#include "threads.hpp"
#include "globals.hpp"
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <vector>

// Distributes units evenly across the arena height, one per lane slice.
void spawnWave(bool isEnemy, UnitType type, int availableGold) {
    Unit tmp; tmp.type = type;
    int cost = tmp.cost();
    if (cost <= 0) return;
    int count = availableGold / cost;

    constexpr float margin   = 40.0f;
    float arenaTop = GAME_TOP + margin;
    float arenaBot = GAME_BOT - margin;
    float arenaH   = arenaBot - arenaTop;

    for (int i = 0; i < count; i++) {
        float laneY = count > 1
            ? arenaTop + arenaH * ((float)i / (float)(count - 1))
            : arenaTop + arenaH * 0.5f;

        Unit u;
        u.type    = type;
        u.isEnemy = isEnemy;
        u.laneY   = laneY;
        u.x       = isEnemy ? 752.0f : 48.0f;  // spawn near own base edge
        u.y       = laneY;
        pool.alloc(u);
    }
}

// Thread 1 — runs every barrier tick.
// Steps: rebuild spatial grid → compute moves/attacks/damage → flush render buffer.
void unitThread() {
    std::vector<int> nearby;
    while (running) {
        auto t0 = Clock::now();
        barrier.wait();
        if (!running) break;

        pool.tickAtkTimers();
        auto snap = pool.snapshot();

        // Rebuild the grid from the current snapshot so nearby() queries are consistent.
        {
            std::lock_guard lock(gridMutex);
            grid.clear();
            for (int i = 0; i < MAX_UNITS; i++)
                if (snap[i].alive) grid.insert(i, snap[i].x, snap[i].y);
        }

        std::vector<std::pair<int, std::pair<float, float>>>   moves;
        std::vector<std::pair<int, int>>                       dmg;
        std::vector<std::pair<int, std::pair<UnitState, int>>> states;

        for (int i = 0; i < MAX_UNITS; i++) {
            const auto& u = snap[i];
            if (!u.alive) continue;

            {
                std::lock_guard lock(gridMutex);
                grid.nearby(u.x, u.y, nearby);
            }

            // Soft vertical push so units don't stack on each other.
            float pushY = 0.0f;
            for (int j : nearby) {
                if (j == i) continue;
                const auto& v = snap[j];
                if (!v.alive) continue;
                float dx = u.x - v.x, dy = u.y - v.y;
                float d    = sqrtf(dx * dx + dy * dy);
                float minD = u.radius() + v.radius();
                if (d < minD && d > 0.0f)
                    pushY += (dy / d) * (minD - d) * 0.4f;
            }

            // Full-range scan for the nearest enemy (AGGRO_RANGE > grid cell size).
            int   bestTarget = -1;
            float bestDist   = AGGRO_RANGE;
            for (int j = 0; j < MAX_UNITS; j++) {
                const auto& v = snap[j];
                if (!v.alive || v.isEnemy == u.isEnemy) continue;
                float dx = u.x - v.x, dy = u.y - v.y;
                float d  = sqrtf(dx * dx + dy * dy);
                if (d < bestDist) { bestDist = d; bestTarget = j; }
            }

            if (bestTarget != -1) {
                const auto& tgt  = snap[bestTarget];
                float dx         = tgt.x - u.x, dy = tgt.y - u.y;
                float dist       = sqrtf(dx * dx + dy * dy);
                float contactDist = u.radius() + tgt.radius() + 2.0f;

                if (dist <= contactDist) {
                    // In contact: attack if cooldown allows.
                    if (snap[i].atkTimer == 0) {
                        states.push_back({i, {UnitState::Attacking, bestTarget}});
                        // Triangles ignore armor; all others are reduced by target armor.
                        int finalDmg = (u.type == UnitType::Triangle)
                                       ? u.damage()
                                       : std::max(0, u.damage() - tgt.armor());
                        if (finalDmg > 0) {
                            dmg.push_back({bestTarget, finalDmg});
                            // Killing an enemy unit earns 3 gold.
                            if (tgt.isEnemy && tgt.hp - finalDmg <= 0)
                                gold.fetch_add(3);
                        }
                    }
                    float ny = std::clamp(u.y + pushY, GAME_TOP + 10.0f, GAME_BOT - 10.0f);
                    if (fabsf(pushY) > 0.01f)
                        moves.push_back({i, {u.x, ny}});
                } else {
                    // Not yet in range: steer toward enemy while applying push.
                    float len = dist > 0.0f ? dist : 1.0f;
                    float nx  = u.x + (dx / len) * u.speed();
                    float ny  = std::clamp(u.y + (dy / len) * u.speed() + pushY,
                                           GAME_TOP + 10.0f, GAME_BOT - 10.0f);
                    moves.push_back({i, {nx, ny}});
                    states.push_back({i, {UnitState::Marching, bestTarget}});
                }
            } else {
                // No enemy visible: march straight toward the opposing base.
                float dir = u.isEnemy ? -1.0f : 1.0f;
                float nx  = u.x + dir * u.speed();
                if ((u.isEnemy && nx <= 45.0f) || (!u.isEnemy && nx >= 755.0f)) {
                    // Reached the base: deal 5 damage and remove the unit.
                    if (u.isEnemy) playerBaseHp.fetch_sub(5);
                    else           enemyBaseHp.fetch_sub(5);
                    dmg.push_back({i, 9999});
                } else {
                    float ny = std::clamp(u.y + pushY, GAME_TOP + 10.0f, GAME_BOT - 10.0f);
                    moves.push_back({i, {nx, ny}});
                    states.push_back({i, {UnitState::Marching, -1}});
                }
            }
        }

        pool.applyStates(states);
        pool.applyMoves(moves);
        pool.applyDamage(dmg);

        // Write updated positions into the back render buffer then swap.
        auto& back = renderDB.getBack();
        back.size  = 0;
        auto snap2 = pool.snapshot();
        for (int i = 0; i < MAX_UNITS; i++) {
            const auto& u = snap2[i];
            if (!u.alive) continue;
            Color c;
            switch (u.type) {
                case UnitType::Circle:   c = u.isEnemy ? RED    : BLUE;     break;
                case UnitType::Triangle: c = u.isEnemy ? ORANGE : SKYBLUE;  break;
                case UnitType::Square:   c = u.isEnemy ? MAROON : DARKBLUE; break;
            }
            back.e[back.size++] = {u.x, u.y, u.radius(), c, u.hp, u.maxHp, u.type};
        }
        renderDB.swap();

        profiler.record(0, Ms(Clock::now() - t0).count());
    }
}

// Thread 2 — counts player units and picks the type that counters the majority.
void aiThread() {
    while (running) {
        auto t0 = Clock::now();
        barrier.wait();
        if (!running) break;

        auto snap = pool.snapshot();
        int pCircles = 0, pSquares = 0, pTriangles = 0;
        for (int i = 0; i < MAX_UNITS; i++) {
            if (!snap[i].alive || snap[i].isEnemy) continue;
            if (snap[i].type == UnitType::Circle)   pCircles++;
            if (snap[i].type == UnitType::Square)   pSquares++;
            if (snap[i].type == UnitType::Triangle) pTriangles++;
        }

        // Counter-pick: most squares → send triangles (pierce armor);
        //               most triangles → send squares (high HP absorbs);
        //               otherwise → mirror with circles.
        UnitType pick;
        if (pSquares >= pCircles && pSquares >= pTriangles)
            pick = UnitType::Triangle;
        else if (pTriangles >= pCircles)
            pick = UnitType::Square;
        else
            pick = UnitType::Circle;

        { std::lock_guard lock(waves.mx); waves.aiSelected = pick; }

        profiler.record(1, Ms(Clock::now() - t0).count());
    }
}

// Thread 3 — drives the tick rate, waits at the barrier each cycle, spawns waves.
void logicThread() {
    using namespace std::chrono;
    auto nextTick = Clock::now();

    while (running) {
        waves.update(TICK_MS / 1000.0f);

        if (waves.consumeSpawn()) {
            int waveNum  = waves.getWave();
            int waveGold = GOLD_PER_WAVE + (waveNum - 1) * 10;  // +10 gold per wave
            int g = waveGold + gold.load();
            gold.store(0);
            UnitType pt, at;
            { std::lock_guard lock(waves.mx); pt = waves.playerSelected; at = waves.aiSelected; }
            spawnWave(false, pt, g);         // player side gets banked gold bonus
            spawnWave(true,  at, waveGold);  // AI always gets the base wave gold
        }

        barrier.wait();
        if (!running) break;

        nextTick += milliseconds(TICK_MS);
        std::this_thread::sleep_until(nextTick);
    }
}