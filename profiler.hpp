#pragma once
#include <array>
#include <mutex>

// Rolling 60-sample frame-time average for up to 3 named thread channels.
struct Profiler {
    struct Channel {
        std::array<double, 60> samples{};
        int idx = 0;
        double total = 0.0;

        // Inserts a new ms value, discarding the oldest sample.
        void push(double ms) {
            total -= samples[idx];
            samples[idx] = ms;
            total += ms;
            idx = (idx + 1) % 60;
        }
        double avg() const { return total / 60.0; }
    };

    std::array<Channel, 3> ch;  // ch[0]=unitThread, ch[1]=aiThread, ch[2]=renderThread
    std::mutex mx;

    void record(int id, double ms) { std::lock_guard lock(mx); ch[id].push(ms); }
    double avg(int id)             { std::lock_guard lock(mx); return ch[id].avg(); }
};