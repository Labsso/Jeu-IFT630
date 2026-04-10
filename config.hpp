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
constexpr float GAME_TOP      = 80.0f;   // y où la zone de jeu commence (sous le HUD)
constexpr float GAME_BOT      = 560.0f;  // y où la zone de jeu se termine (au-dessus de la barre de statut)
constexpr float GAME_H        = GAME_BOT - GAME_TOP;
constexpr float AGGRO_RANGE   = 350.0f;  // les unités dans ce rayon se voient mutuellement
constexpr int   WAVE_DURATION = 10;      // secondes entre les spawns
constexpr int   GOLD_PER_WAVE = 50;      // or de base donné à chaque vague
constexpr int   TICK_MS       = 16;      // période d'un tick logique (~60 ticks/s)