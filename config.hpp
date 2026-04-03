#pragma once
#include <chrono>

using Clock = std::chrono::steady_clock;
using Ms    = std::chrono::duration<double, std::milli>;

constexpr int   MAX_UNITS     = 512;
constexpr int   GRID_COLS     = 16;
constexpr int   GRID_ROWS     = 12;
constexpr float CELL_W        = 800.0f / GRID_COLS;
constexpr float CELL_H        = 560.0f / GRID_ROWS;
constexpr int   SCREEN_W      = 800;
constexpr int   SCREEN_H      = 640;
constexpr float GAME_TOP      = 80.0f;   // y where the play area starts (below HUD)
constexpr float GAME_BOT      = 560.0f;  // y where the play area ends (above status bar)
constexpr float GAME_H        = GAME_BOT - GAME_TOP;
constexpr float AGGRO_RANGE   = 350.0f;  // units within this radius see each other
constexpr int   WAVE_DURATION = 10;      // seconds between spawns
constexpr int   GOLD_PER_WAVE = 50;      // base gold given each wave
constexpr int   TICK_MS       = 16;      // logic tick period (~60 ticks/s)