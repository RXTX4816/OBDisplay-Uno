#pragma once

// Minimal avr/pgmspace.h shim for native tests: on the host, "flash" is plain
// memory, so PROGMEM vanishes and pgm_read_* dereferences directly.
// Do NOT use in firmware builds; it's only for [env:native].

#include <stdint.h>
#include <string.h>

#define PROGMEM
#define PGM_P const char*

namespace native_pgm
{
template <typename T> inline T readWord(const T* p)
{
    return *p;
}
// Tables of flash string pointers (PGM_P const[]): AVR pointers are 16-bit, so
// firmware casts the uint16_t result back to a pointer. Host pointers are 64-bit,
// so hand back the whole address instead of truncating it.
template <typename T> inline uintptr_t readWord(T* const* p)
{
    return reinterpret_cast<uintptr_t>(*p);
}
} // namespace native_pgm

#define pgm_read_byte(p) (*reinterpret_cast<const uint8_t*>(p))
#define pgm_read_word(p) (native_pgm::readWord(p))
#define memcpy_P memcpy
#define strlen_P strlen
