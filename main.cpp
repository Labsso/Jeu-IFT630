// Miragine War
// Architecture: 3 worker threads (unitThread, aiThread, logicThread) synchronised
// by a barrier each tick (~60 Hz).  The main thread renders freely at 60 FPS and
// is NOT part of the barrier — it reads the double-buffered render data lock-free.

#include <raylib.h>
#include <thread>
#include <algorithm>
#include "globals.hpp"
#include "threads.hpp"
#include "render.hpp"

int main() {
    InitWindow(SCREEN_W, SCREEN_H, "Miragine War");
    SetTargetFPS(60);
    srand((unsigned)time(nullptr));

    // Spawn the first wave before starting threads so the pool isn't empty.
    spawnWave(false, UnitType::Circle, GOLD_PER_WAVE);
    spawnWave(true,  UnitType::Circle, GOLD_PER_WAVE);

    std::thread tUnits(unitThread);
    std::thread tAI(aiThread);
    std::thread tLogic(logicThread);

    while (!WindowShouldClose()) {
        auto t0 = Clock::now();

        bool gameOver = (playerBaseHp.load() <= 0 || enemyBaseHp.load() <= 0);

        // Input: keys 1/2/3 select the unit type spawned next wave.
        if (!gameOver) {
            if (IsKeyPressed(KEY_ONE))   { std::lock_guard lock(waves.mx); waves.playerSelected = UnitType::Circle;   }
            if (IsKeyPressed(KEY_TWO))   { std::lock_guard lock(waves.mx); waves.playerSelected = UnitType::Triangle; }
            if (IsKeyPressed(KEY_THREE)) { std::lock_guard lock(waves.mx); waves.playerSelected = UnitType::Square;   }
        }

        BeginDrawing();
        ClearBackground({15, 15, 25, 255});

        // Play area background + centre line
        DrawRectangle(0, (int)GAME_TOP, SCREEN_W, (int)GAME_H, {25, 28, 40, 255});
        DrawLine(SCREEN_W / 2, (int)GAME_TOP, SCREEN_W / 2, (int)GAME_BOT, {60, 60, 80, 100});

        // Left base (player) — filled proportionally to remaining HP
        int php = std::max(0, playerBaseHp.load());
        int ehp = std::max(0, enemyBaseHp.load());
        DrawRectangle(0, (int)GAME_TOP, 35, (int)GAME_H, {15, 15, 45, 255});
        DrawRectangle(2, (int)(GAME_TOP + GAME_H * (1.0f - php / 100.0f)), 31, (int)(GAME_H * php / 100.0f), BLUE);
        DrawRectangleLines(0, (int)GAME_TOP, 35, (int)GAME_H, DARKBLUE);
        DrawText(TextFormat("%d", php), 4, (int)GAME_TOP + 5, 12, LIGHTGRAY);

        // Right base (enemy)
        DrawRectangle(765, (int)GAME_TOP, 35, (int)GAME_H, {45, 15, 15, 255});
        DrawRectangle(767, (int)(GAME_TOP + GAME_H * (1.0f - ehp / 100.0f)), 31, (int)(GAME_H * ehp / 100.0f), RED);
        DrawRectangleLines(765, (int)GAME_TOP, 35, (int)GAME_H, MAROON);
        DrawText(TextFormat("%d", ehp), 770, (int)GAME_TOP + 5, 12, LIGHTGRAY);

        // Units — read from the front buffer, no lock needed (double-buffered).
        const auto& front = renderDB.getFront();
        for (int i = 0; i < front.size; i++) drawUnit(front.e[i]);

        // Top HUD
        DrawRectangle(0, 0, SCREEN_W, (int)GAME_TOP, {10, 10, 18, 245});

        // Wave timer bar
        float wTimer   = waves.getTimer();
        int   wNum     = waves.getWave();
        DrawText(TextFormat("Vague %d", wNum), SCREEN_W / 2 - 38, 6, 20, WHITE);
        float timerPct = wTimer / WAVE_DURATION;
        DrawRectangle(SCREEN_W / 2 - 80, 32, 160, 10, {40, 40, 60, 255});
        DrawRectangle(SCREEN_W / 2 - 80, 32, (int)(160 * timerPct), 10,
                      timerPct > 0.4f ? GREEN : (timerPct > 0.15f ? YELLOW : RED));
        DrawRectangleLines(SCREEN_W / 2 - 80, 32, 160, 10, GRAY);
        DrawText(TextFormat("%.1fs", wTimer), SCREEN_W / 2 - 12, 31, 11, LIGHTGRAY);

        // Gold display
        DrawText(TextFormat("Gold: %d", gold.load()), 10, 8, 18, GOLD);
        int nextWaveGold = GOLD_PER_WAVE + (waves.getWave() - 1) * 10;
        DrawText(TextFormat("+%d prochaine vague", nextWaveGold), 10, 30, 12, {200, 160, 0, 180});

        // Unit selection buttons
        UnitType sel;
        { std::lock_guard lock(waves.mx); sel = waves.playerSelected; }
        struct Btn { UnitType t; const char* label; const char* stats; Color col; };
        Btn btns[3] = {
            {UnitType::Circle,   "[1] Cercle",   "10g  HP:12  DMG:3", BLUE},
            {UnitType::Triangle, "[2] Triangle", "20g  HP:6   DMG:5", SKYBLUE},
            {UnitType::Square,   "[3] Carre",    "30g  HP:25  ARM:2", DARKBLUE},
        };
        for (int b = 0; b < 3; b++) {
            int bx     = 150 + b * 170;
            bool active = sel == btns[b].t;
            DrawRectangle(bx, 4, 158, 70, active ? Color{35, 55, 95, 255} : Color{18, 18, 32, 255});
            DrawRectangleLines(bx, 4, 158, 70, active ? SKYBLUE : Color{55, 55, 75, 255});
            drawUnitIcon(bx + 18.0f, 38.0f, 14.0f, btns[b].t, btns[b].col);
            DrawText(btns[b].label, bx + 36, 12, 14, active ? WHITE : LIGHTGRAY);
            DrawText(btns[b].stats, bx + 36, 32, 11, active ? LIGHTGRAY : Color{100, 100, 120, 255});
            if (active) DrawText("SELECTIONNE", bx + 36, 52, 10, SKYBLUE);
        }

        DrawText(TextFormat("%d unites", pool.aliveCount.load()), 658, 8, 13, LIGHTGRAY);

        // Bottom status bar: per-thread frame times + AI selection
        DrawRectangle(0, (int)GAME_BOT, SCREEN_W, SCREEN_H - (int)GAME_BOT, {10, 10, 18, 220});
        DrawText(TextFormat("unitThread: %.2fms", profiler.avg(0)),  10, (int)GAME_BOT + 10, 12, SKYBLUE);
        DrawText(TextFormat("aiThread:   %.2fms", profiler.avg(1)), 220, (int)GAME_BOT + 10, 12, ORANGE);
        DrawText(TextFormat("FPS: %d", GetFPS()),                   420, (int)GAME_BOT + 10, 12, GREEN);

        UnitType aiSel;
        { std::lock_guard lock(waves.mx); aiSel = waves.aiSelected; }
        const char* aiSelName = aiSel == UnitType::Triangle ? "Triangle"
                              : (aiSel == UnitType::Square  ? "Carre" : "Cercle");
        DrawText(TextFormat("IA joue: %s", aiSelName), 545, (int)GAME_BOT + 10, 12, RED);

        // Game-over overlay
        if (gameOver) {
            DrawRectangle(200, 220, 400, 100, Fade(BLACK, 0.88f));
            DrawRectangleLines(200, 220, 400, 100, GRAY);
            if (playerBaseHp.load() <= 0)
                DrawText("DEFAITE",    285, 245, 44, RED);
            else
                DrawText("VICTOIRE !", 255, 245, 44, GREEN);
            DrawText("Fermer la fenetre pour quitter", 228, 300, 15, LIGHTGRAY);
        }

        profiler.record(2, Ms(Clock::now() - t0).count());
        EndDrawing();
    }

    // Signal threads to stop, then unblock the barrier in case they're waiting.
    running = false;
    barrier.wait();
    tUnits.join();
    tAI.join();
    tLogic.join();
    CloseWindow();
    return 0;
}