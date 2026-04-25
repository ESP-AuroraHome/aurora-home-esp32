#pragma once
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>

#include "WString.h"

inline void delay(uint32_t) {}

struct SerialClass {
    static void begin(uint32_t) {}
    /* NOLINTNEXTLINE(cert-err33-c) */
    static void printf(const char* fmt, ...) __attribute__((format(printf, 1, 2))) {
        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
};
inline SerialClass Serial;

template <typename T>
T min(T a, T b) {
    return a < b ? a : b;
}
