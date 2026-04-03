#pragma once
#include "unit.hpp"
#include <mutex>

// Counts down a per-wave timer and records unit-type selections for both sides.
struct WaveSystem {
    std::mutex mx;
    int   waveNumber    = 1;
    float timer         = 10.0f;  // seconds until next spawn
    bool  spawnPending  = false;
    UnitType playerSelected = UnitType::Circle;
    UnitType aiSelected     = UnitType::Circle;

    // Advances the timer by dt seconds; sets spawnPending when it hits 0.
    void update(float dt) {
        std::lock_guard lock(mx);
        timer -= dt;
        if (timer <= 0.0f) {
            timer = 10.0f;
            waveNumber++;
            spawnPending = true;
        }
    }

    // Returns true (and clears the flag) if a spawn was triggered this tick.
    bool consumeSpawn() {
        std::lock_guard lock(mx);
        if (!spawnPending) return false;
        spawnPending = false;
        return true;
    }

    float getTimer() { std::lock_guard lock(mx); return timer; }
    int   getWave()  { std::lock_guard lock(mx); return waveNumber; }
};