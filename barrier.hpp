#pragma once
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdint>

// Barrière réutilisable : bloque chaque appelant jusqu'à ce que tous les `total` participants soient arrivés.
// Utilise un compteur d'époque pour que les arrivées tardives du tour précédent ne provoquent pas d'interblocage.
struct FrameBarrier {
    std::mutex mx;
    std::condition_variable cv;
    int arrived = 0;
    const int total;
    std::atomic<uint64_t> epoch{0};

    explicit FrameBarrier(int n) : total(n) {}

    // Bloque jusqu'à ce que tous les threads aient appelé wait(), puis libère tout le monde.
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