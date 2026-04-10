#pragma once
#include "unit.hpp"

// Spawne `availableGold / cost` unités du type donné pour un camp.
void spawnWave(bool isEnemy, UnitType type, int availableGold);

// Thread 1 : déplace les unités, résout les combats, endommage les bases, écrit le tampon de rendu.
void unitThread();

// Thread 2 : compte les unités du joueur par type et choisit le contre-pick de l'IA.
void aiThread();

// Thread 3 : gère l'horloge de tick, déclenche les spawns de vagues, synchronise la barrière.
void logicThread();