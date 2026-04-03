#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

// Reusable barrier: blocks each caller until all `total` participants have arrived.
// Uses an epoch counter so late arrivals from the previous round don't deadlock.
struct FrameBarrier {
    std::mutex mx;
    std::condition_variable cv;
    int arrived = 0;
    const int total;
    std::atomic<uint64_t> epoch{0};

    explicit FrameBarrier(int n) : total(n) {}

    // Blocks until all threads have called wait(), then releases everyone.
    void wait() {
        std::unique_lock lock(mx);
        uint64_t myEpoch = epoch.load();
        if (++arrived == total) {
            arrived = 0;
            epoch.fetch_add(1);
            cv.notify_all();
        } else {
            cv.wait(lock, [&] { return epoch.load() > myEpoch; });
        }
    }
};