#pragma once
#include <array>
#include <atomic>
#include <mutex>
#include <raylib.h>
#include "config.hpp"
#include "unit.hpp"

// Une entrée dans le tampon de rendu : tout ce dont le thread de dessin a besoin pour une unité.
struct RenderEntry { float x, y, r; Color color; int hp, maxHp; UnitType type; };

struct RenderBuffer { std::array<RenderEntry, MAX_UNITS> e; int size = 0; };

// Tampon ping-pong : unitThread écrit à l'arrière, le thread de rendu lit à l'avant.
// La permutation est la seule opération nécessitant un verrou.
struct DoubleBuffer {
    RenderBuffer bufs[2];
    std::atomic<int> front{0};
    std::mutex swapMx;

    RenderBuffer& getFront() { return bufs[front.load()]; }
    RenderBuffer& getBack()  { return bufs[1 - front.load()]; }
    void swap() { std::lock_guard lock(swapMx); front.store(1 - front.load()); }
};

// Dessine la forme d'une unité + barre de HP à partir d'un RenderEntry.
void drawUnit(const RenderEntry& e);

// Dessine une petite icône d'unité utilisée dans les boutons de sélection du HUD.
void drawUnitIcon(float cx, float cy, float r, UnitType t, Color c);