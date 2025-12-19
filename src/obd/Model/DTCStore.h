#pragma once

#include <Arduino.h>

namespace obd {
namespace Model {

class DTCStore {
public:
    static constexpr uint8_t MaxCount = 16;

    DTCStore();

    void reset();
    void resetRandom();

    // Returns the fixed capacity of the internal arrays (not the number of active DTCs).
    // This mirrors the original behavior and is used by display code to iterate slots.
    uint8_t capacity() const { return MaxCount; }

    uint16_t errorAt(uint8_t idx) const { return dtcErrors_[idx]; }
    uint8_t statusAt(uint8_t idx) const { return dtcStatus_[idx]; }

    void set(uint8_t idx, uint16_t error, uint8_t status);

private:
    uint16_t dtcErrors_[MaxCount];
    uint8_t dtcStatus_[MaxCount];
};

} // namespace Model
} // namespace obd
