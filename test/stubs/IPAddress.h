#pragma once
#include <cstdint>
#include <cstdio>

#include "WString.h"

struct IPAddress {
    uint8_t _address[4];

    IPAddress() { _address[0] = _address[1] = _address[2] = _address[3] = 0; }
    IPAddress(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
        _address[0] = a;
        _address[1] = b;
        _address[2] = c;
        _address[3] = d;
    }
    /* Mirrors Arduino's uint32_t constructor (memcpy of 4 bytes). */
    explicit IPAddress(uint32_t addr) {
        _address[0] = addr & 0xFF;
        _address[1] = (addr >> 8) & 0xFF;
        _address[2] = (addr >> 16) & 0xFF;
        _address[3] = (addr >> 24) & 0xFF;
    }

    bool operator==(const IPAddress& o) const {
        return _address[0] == o._address[0] && _address[1] == o._address[1] &&
               _address[2] == o._address[2] && _address[3] == o._address[3];
    }
    bool operator!=(const IPAddress& o) const { return !(*this == o); }

    String toString() const {
        String s;
        snprintf(s.buf, sizeof(s.buf), "%d.%d.%d.%d", _address[0], _address[1], _address[2],
                 _address[3]);
        return s;
    }
};
