#pragma once

// Minimal Arduino.h shim for native tests. This is intentionally
// tiny and only provides what the model code needs. Do NOT use in
// firmware builds; it's only for [env:native].

#include <stdint.h>
#include <stdlib.h>
#include <string>

// Basic Arduino-style types
using byte = uint8_t;
using String = std::string;

// millis() / delay() stubs
inline unsigned long millis()
{
    // For deterministic unit tests, just return 0 unless overridden
    return 0;
}

inline void delay(unsigned long) {}

// abs overloads as in Arduino
using ::abs;
