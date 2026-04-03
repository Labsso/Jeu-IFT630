#pragma once
#include "config.hpp"
#include <array>
#include <vector>
#include <algorithm>

// Uniform grid that maps world positions to buckets for fast neighbour queries.
// Each cell covers CELL_W x CELL_H pixels.  Must be rebuilt every tick.
struct SpatialGrid {
    std::array<std::vector<int>, GRID_COLS * GRID_ROWS> cells;

    void clear() { for (auto& c : cells) c.clear(); }

    // Inserts unit index idx at world position (x, y).
    void insert(int idx, float x, float y) {
        int col = std::clamp((int)(x / CELL_W), 0, GRID_COLS - 1);
        int row = std::clamp((int)((y - GAME_TOP) / CELL_H), 0, GRID_ROWS - 1);
        cells[row * GRID_COLS + col].push_back(idx);
    }

    // Fills out with indices from the 3x3 neighbourhood around (x, y).
    void nearby(float x, float y, std::vector<int>& out) const {
        out.clear();
        int col = std::clamp((int)(x / CELL_W), 0, GRID_COLS - 1);
        int row = std::clamp((int)((y - GAME_TOP) / CELL_H), 0, GRID_ROWS - 1);
        for (int dr = -1; dr <= 1; dr++)
            for (int dc = -1; dc <= 1; dc++) {
                int r = row + dr, c = col + dc;
                if (r < 0 || r >= GRID_ROWS || c < 0 || c >= GRID_COLS) continue;
                for (int j : cells[r * GRID_COLS + c]) out.push_back(j);
            }
    }
};