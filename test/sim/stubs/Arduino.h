#pragma once
// Host (PC) stand-in for the few Arduino functions the game logic uses, so
// src/game/*.cpp compiles unchanged with a desktop C++ compiler.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifndef PI
#define PI 3.14159265358979f
#endif

inline long random(long hi) { return hi > 0 ? std::rand() % hi : 0; }
inline long random(long lo, long hi) { return hi > lo ? lo + std::rand() % (hi - lo) : lo; }

// Serial.printf is silent unless the simulation turns it on (--verbose).
struct SerialStub {
    bool verbose = false;
    template <typename... A>
    void printf(const char* fmt, A... args) {
        if (verbose) std::printf(fmt, args...);
    }
    void println(const char* s) { if (verbose) std::puts(s); }
};
extern SerialStub Serial;
