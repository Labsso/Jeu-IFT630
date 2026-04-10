#pragma once
#include "unit.hpp"
#include <mutex>

// Décompte un minuteur par vague et enregistre les sélections de type d'unité pour les deux camps.
struct WaveSystem {
    std::mutex mx;
    int   waveNumber    = 1;
    float timer         = 10.0f;  // secondes avant le prochain spawn
    bool  spawnPending  = false;
    UnitType playerSelected = UnitType::Circle;
    UnitType aiSelected     = UnitType::Circle;

    // Avance le minuteur de dt secondes ; active spawnPending quand il atteint 0.
    void update(float dt) {
        std::lock_guard lock(mx);
        timer -= dt;
        if (timer <= 0.0f) {
            timer = 10.0f;
            waveNumber++;
            spawnPending = true;
        }
    }

    // Retourne true (et efface le drapeau) si un spawn a été déclenché ce tick.
    bool consumeSpawn() {
        std::lock_guard lock(mx);
        if (!spawnPending) return false;
        spawnPending = false;
        return true;
    }

    float getTimer() { std::lock_guard lock(mx); return timer; }
    int   getWave()  { std::lock_guard lock(mx); return waveNumber; }
};