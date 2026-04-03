#include "globals.hpp"

UnitPool      pool;
DoubleBuffer  renderDB;
WaveSystem    waves;
Profiler      profiler;
FrameBarrier  barrier(3);
SpatialGrid   grid;
std::mutex    gridMutex;

std::atomic<bool> running{true};
std::atomic<int>  gold{0};
std::atomic<int>  playerBaseHp{100};
std::atomic<int>  enemyBaseHp{100};