#pragma once
#include "unit.hpp"

// Spawns `availableGold / cost` units of the given type for one side.
void spawnWave(bool isEnemy, UnitType type, int availableGold);

// Thread 1: moves units, resolves combat, damages bases, writes render buffer.
void unitThread();

// Thread 2: counts player units per type and picks the AI counter-pick.
void aiThread();

// Thread 3: owns the tick clock, triggers wave spawns, synchronises the barrier.
void logicThread();