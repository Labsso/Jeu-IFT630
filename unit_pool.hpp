#pragma once
#include "config.hpp"
#include "unit.hpp"
#include <array>
#include <shared_mutex>
#include <atomic>
#include <vector>

// Fixed-size pool of units with reader-writer locking.
// Writers take a unique_lock; readers (snapshot) take a shared_lock.
struct UnitPool {
    std::array<Unit, MAX_UNITS> data{};
    mutable std::shared_mutex   rwMutex;
    std::atomic<int>            aliveCount{0};

    // Finds the first free slot, initialises it, and returns its index.
    // Returns -1 if the pool is full.
    int alloc(Unit u) {
        u.alive    = true;
        u.hp = u.maxHp = u.initHp();
        u.atkTimer = 0;
        u.state    = UnitState::Marching;
        u.target   = -1;
        std::unique_lock lock(rwMutex);
        for (int i = 0; i < MAX_UNITS; i++) {
            if (!data[i].alive) {
                data[i] = u;
                aliveCount.fetch_add(1);
                return i;
            }
        }
        return -1;
    }

    // Returns a copy of the entire array; safe to read without the lock afterwards.
    std::array<Unit, MAX_UNITS> snapshot() const {
        std::shared_lock lock(rwMutex);
        return data;
    }

    // Decrements atkTimer for every living unit (called once per tick).
    void tickAtkTimers() {
        std::unique_lock lock(rwMutex);
        for (auto& u : data)
            if (u.alive && u.atkTimer > 0) u.atkTimer--;
    }

    // Applies a batch of position updates computed by unitThread.
    void applyMoves(const std::vector<std::pair<int, std::pair<float, float>>>& moves) {
        std::unique_lock lock(rwMutex);
        for (auto& [idx, pos] : moves) {
            if (!data[idx].alive) continue;
            data[idx].x = pos.first;
            data[idx].y = pos.second;
        }
    }

    // Applies a batch of damage values; kills units whose hp reaches 0.
    void applyDamage(const std::vector<std::pair<int, int>>& patches) {
        std::unique_lock lock(rwMutex);
        for (auto& [idx, d] : patches) {
            if (!data[idx].alive) continue;
            data[idx].hp -= d;
            if (data[idx].hp <= 0) {
                data[idx].alive = false;
                aliveCount.fetch_sub(1);
            }
        }
    }

    // Applies a batch of (state, target) pairs and resets atkTimer when entering Attacking.
    void applyStates(const std::vector<std::pair<int, std::pair<UnitState, int>>>& states) {
        std::unique_lock lock(rwMutex);
        for (auto& [idx, sv] : states) {
            if (!data[idx].alive) continue;
            data[idx].state  = sv.first;
            data[idx].target = sv.second;
            if (sv.first == UnitState::Attacking && data[idx].atkTimer == 0)
                data[idx].atkTimer = data[idx].atkCooldownMax();
        }
    }
};