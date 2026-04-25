#pragma once
#include <cstdint>

/* Minimal stub so net.h compiles in the native test env (no Arduino SDK). */
struct IPAddress {
    uint8_t _address[4];
    IPAddress() { _address[0] = _address[1] = _address[2] = _address[3] = 0; }
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        _address[0] = a;
        _address[1] = b;
        _address[2] = c;
        _address[3] = d;
    }
};
