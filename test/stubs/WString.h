#pragma once
#include <cstdio>

struct String {
    char buf[64];
    String() { buf[0] = 0; }
    explicit String(const char* s) { snprintf(buf, sizeof(buf), "%s", s); }
    const char* c_str() const { return buf; }
};
