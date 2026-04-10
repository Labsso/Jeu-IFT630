#pragma once
#include <array>
#include <mutex>

// Moyenne glissante sur 60 échantillons du temps de frame pour jusqu'à 3 canaux de thread nommés.
struct Profiler {
    struct Channel {
        std::array<double, 60> samples{};
        int idx = 0;
        double total = 0.0;

        // Insère une nouvelle valeur en ms, en écartant l'échantillon le plus ancien.
        void push(double ms) {
            total -= samples[idx];
            samples[idx] = ms;
            total += ms;
            idx = (idx + 1) % 60;
        }
        double avg() const { return total / 60.0; }
    };

    std::array<Channel, 3> ch;  // ch[0]=unitThread, ch[1]=aiThread, ch[2]=threadDeRendu
    std::mutex mx;

    void record(int id, double ms) { std::lock_guard lock(mx); ch[id].push(ms); }
    double avg(int id)             { std::lock_guard lock(mx); return ch[id].avg(); }
};