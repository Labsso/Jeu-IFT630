#pragma once
#include "config.hpp"
#include "unit.hpp"
#include <array>
#include <shared_mutex>
#include <atomic>
#include <mutex>
#include <vector>

// Pool d'unités à taille fixe avec verrouillage lecteur-écrivain.
// Les écrivains prennent un unique_lock ; les lecteurs prennent un shared_lock.
struct UnitPool {
    std::array<Unit, MAX_UNITS> data{};
    mutable std::shared_mutex   rwMutex;
    std::atomic<int>            aliveCount{0};

    // Trouve le premier emplacement libre, l'initialise et retourne son index.
    // Retourne -1 si le pool est plein.
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

    // Retourne une copie du tableau entier ; ok à lire sans verrou ensuite.
    std::array<Unit, MAX_UNITS> snapshot() const {
        std::shared_lock lock(rwMutex);
        return data;
    }

    // Décrémente atkTimer pour chaque unité vivante (appelé une fois par tick).
    void tickAtkTimers() {
        std::unique_lock lock(rwMutex);
        for (auto& u : data)
            if (u.alive && u.atkTimer > 0) u.atkTimer--;
    }

    // Applique un lot de mises à jour de positions calculées par unitThread.
    void applyMoves(const std::vector<std::pair<int, std::pair<float, float>>>& moves) {
        std::unique_lock lock(rwMutex);
        for (auto& [idx, pos] : moves) {
            if (!data[idx].alive) continue;
            data[idx].x = pos.first;
            data[idx].y = pos.second;
        }
    }

    // Applique un lot de valeurs de dégâts ; tue les unités dont les HP atteignent 0.
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

    // Applique un lot de paires (état, cible) et réinitialise atkTimer lors du passage en état Attacking.
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