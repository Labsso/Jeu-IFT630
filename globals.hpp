#pragma once
#include "unit_pool.hpp"
#include "render.hpp"
#include "wave_system.hpp"
#include "profiler.hpp"
#include "barrier.hpp"
#include "spatial_grid.hpp"
#include <atomic>
#include <mutex>

// All global singletons shared across threads.
// Defined in globals.cpp; declared extern here so every TU can access them.

extern UnitPool      pool;
extern DoubleBuffer  renderDB;
extern WaveSystem    waves;
extern Profiler      profiler;
extern FrameBarrier  barrier;   // 3 participants: unitThread, aiThread, logicThread
extern SpatialGrid   grid;
extern std::mutex    gridMutex;

extern std::atomic<bool> running;
extern std::atomic<int>  gold;
extern std::atomic<int>  playerBaseHp;
extern std::atomic<int>  enemyBaseHp;