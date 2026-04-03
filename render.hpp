#pragma once
#include <array>
#include <atomic>
#include <mutex>
#include <raylib.h>
#include "config.hpp"
#include "unit.hpp"

// One entry in the render buffer: everything the draw thread needs for one unit.
struct RenderEntry { float x, y, r; Color color; int hp, maxHp; UnitType type; };

struct RenderBuffer { std::array<RenderEntry, MAX_UNITS> e; int size = 0; };

// Ping-pong buffer: unitThread writes to back, render thread reads from front.
// Swap is the only operation that needs a lock.
struct DoubleBuffer {
    RenderBuffer bufs[2];
    std::atomic<int> front{0};
    std::mutex swapMx;

    RenderBuffer& getFront() { return bufs[front.load()]; }
    RenderBuffer& getBack()  { return bufs[1 - front.load()]; }
    void swap() { std::lock_guard lock(swapMx); front.store(1 - front.load()); }
};

// Draws a unit shape + HP bar from a RenderEntry.
void drawUnit(const RenderEntry& e);

// Draws a small unit icon used in the HUD selection buttons.
void drawUnitIcon(float cx, float cy, float r, UnitType t, Color c);