#include "threads.hpp"
#include "globals.hpp"
#include <cmath>
#include <algorithm>
#include <thread>
#include <chrono>
#include <vector>

// Répartit les unités en groupe autour d'un centre Y aléatoire.
void spawnWave(bool isEnemy, UnitType type, int availableGold) {
    Unit tmp; tmp.type = type;
    int cost = tmp.cost();
    if (cost <= 0) return;
    int count = availableGold / cost;

    constexpr float margin    = 40.0f;
    constexpr float groupSpread = 30.0f;  // rayon du groupe en Y
    constexpr float xSpread     = 20.0f;  // décalage X aléatoire dans le groupe
    float arenaTop = GAME_TOP + margin;
    float arenaBot = GAME_BOT - margin;

    // Centre Y aléatoire dans l'arène
    float centerY = arenaTop + ((float)rand() / (float)RAND_MAX) * (arenaBot - arenaTop);

    float baseX = isEnemy ? 752.0f : 48.0f;

    for (int i = 0; i < count; i++) {
        float offsetY = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * groupSpread;
        float offsetX = (((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f) * xSpread;
        float spawnY  = std::max(arenaTop, std::min(arenaBot, centerY + offsetY));

        Unit u;
        u.type    = type;
        u.isEnemy = isEnemy;
        u.laneY   = spawnY;
        u.x       = baseX + offsetX;
        u.y       = spawnY;
        pool.alloc(u);
    }
}

// Thread 1 — s'exécute à chaque tick de barrière.
// Étapes : reconstruire la grille spatiale → calculer déplacements/attaques/dégâts → vider le tampon de rendu.
void unitThread() {
    std::vector<int> nearby;
    while (running) {
        barrier.wait();
        if (!running) break;
        auto t0 = Clock::now();

        pool.tickAtkTimers();
        auto snap = pool.snapshot();

        // Reconstruire la grille à partir du snapshot courant pour que les requêtes nearby() soient cohérentes.
        {
            std::lock_guard lock(gridMutex);
            grid.clear();
            for (int i = 0; i < MAX_UNITS; i++)
                if (snap[i].alive) grid.insert(i, snap[i].x, snap[i].y);
        }

        std::vector<std::pair<int, std::pair<float, float>>> moves;
        std::vector<std::pair<int, int>> dmg;
        std::vector<std::pair<int, std::pair<UnitState, int>>> states;

        for (int i = 0; i < MAX_UNITS; i++) {
            const auto& u = snap[i];
            if (!u.alive) continue;

            {
                std::lock_guard lock(gridMutex);
                grid.nearby(u.x, u.y, nearby);
            }

            // Poussée verticale douce pour éviter que les unités se superposent.
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

            // Scan complet pour trouver l'ennemi le plus proche (AGGRO_RANGE > taille d'une cellule de grille).
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
                    // En contact : attaquer si le délai de recharge le permet.
                    if (snap[i].atkTimer == 0) {
                        states.push_back({i, {UnitState::Attacking, bestTarget}});
                        // Les triangles ignorent l'armure ; tous les autres sont réduits par l'armure de la cible.
                        int finalDmg = (u.type == UnitType::Triangle)
                                       ? u.damage()
                                       : std::max(0, u.damage() - tgt.armor());
                        if (finalDmg > 0) {
                            dmg.push_back({bestTarget, finalDmg});
                            // Tuer une unité ennemie rapporte 3 pièces d'or.
                            if (tgt.isEnemy && tgt.hp - finalDmg <= 0)
                                gold.fetch_add(3);
                        }
                    }
                    float ny = std::clamp(u.y + pushY, GAME_TOP + 10.0f, GAME_BOT - 10.0f);
                    if (fabsf(pushY) > 0.01f)
                        moves.push_back({i, {u.x, ny}});
                } else {
                    // Pas encore à portée : se diriger vers l'ennemi tout en appliquant la poussée.
                    float len = dist > 0.0f ? dist : 1.0f;
                    float nx  = u.x + (dx / len) * u.speed();
                    float ny  = std::clamp(u.y + (dy / len) * u.speed() + pushY,
                                           GAME_TOP + 10.0f, GAME_BOT - 10.0f);
                    moves.push_back({i, {nx, ny}});
                    states.push_back({i, {UnitState::Marching, bestTarget}});
                }
            } else {
                // Aucun ennemi visible : marcher droit vers la base adverse.
                float dir = u.isEnemy ? -1.0f : 1.0f;
                float nx  = u.x + dir * u.speed();
                if ((u.isEnemy && nx <= 45.0f) || (!u.isEnemy && nx >= 755.0f)) {
                    // Base atteinte : infliger 5 dégâts et retirer l'unité.
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

        // Écrire les positions mises à jour dans le tampon de rendu arrière, puis permuter.
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

// Thread 2 — compte les unités du joueur et choisit le type qui contre la majorité.
void aiThread() {
    while (running) {
        barrier.wait();
        if (!running) break;
        auto t0 = Clock::now();

        auto snap = pool.snapshot();
        int pCircles = 0, pSquares = 0, pTriangles = 0;
        for (int i = 0; i < MAX_UNITS; i++) {
            if (!snap[i].alive || snap[i].isEnemy) continue;
            if (snap[i].type == UnitType::Circle)   pCircles++;
            if (snap[i].type == UnitType::Square)   pSquares++;
            if (snap[i].type == UnitType::Triangle) pTriangles++;
        }

        // Contre-pick : majorité de carrés → envoyer des triangles (percent l'armure) ;
        //               majorité de triangles → envoyer des cercles ;
        //               sinon → majorité de cercle → envoyer carrés (armure).
        UnitType pick;
        if (pSquares >= pCircles && pSquares >= pTriangles)
            pick = UnitType::Triangle;
        else if (pTriangles >= pCircles)
            pick = UnitType::Circle;
        else
            pick = UnitType::Square;

        { std::lock_guard lock(waves.mx); waves.aiSelected = pick; }

        profiler.record(1, Ms(Clock::now() - t0).count());
    }
}

// Thread 3 — cadence les ticks, attend à la barrière chaque cycle, déclenche les vagues.
void logicThread() {
    using namespace std::chrono;
    auto nextTick = Clock::now();

    while (running) {
        auto t0 = Clock::now();
        waves.update(TICK_MS / 1000.0f);

        if (waves.consumeSpawn()) {
            int waveNum  = waves.getWave();
            int waveGold = GOLD_PER_WAVE + (waveNum - 1) * 10;  // +10 or par vague
            int g = waveGold + gold.load();
            gold.store(0);
            UnitType pt, at;
            { std::lock_guard lock(waves.mx); pt = waves.playerSelected; at = waves.aiSelected; }
            spawnWave(false, pt, g);         // le joueur reçoit le bonus d'or accumulé
            spawnWave(true,  at, waveGold);  // l'IA reçoit toujours l'or de base de la vague
        }

        profiler.record(3, Ms(Clock::now() - t0).count());
        barrier.wait();
        if (!running) break;

        nextTick += milliseconds(TICK_MS);
        std::this_thread::sleep_until(nextTick);
    }
}