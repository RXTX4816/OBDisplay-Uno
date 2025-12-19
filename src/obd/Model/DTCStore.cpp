#include "DTCStore.h"

namespace obd {
namespace Model {

DTCStore::DTCStore()
{
    reset();
}

void DTCStore::reset()
{
    for (uint8_t i = 0; i < MaxCount; ++i) {
        dtcErrors_[i] = 0xFFFF;
        dtcStatus_[i] = 0xFF;
    }
}

void DTCStore::resetRandom()
{
    for (uint8_t i = 0; i < MaxCount; ++i) {
        dtcErrors_[i] = (uint16_t)(i * 1000);
        dtcStatus_[i] = (uint8_t)(i * 10);
    }
}

void DTCStore::set(uint8_t idx, uint16_t error, uint8_t status)
{
    if (idx >= MaxCount) return;
    dtcErrors_[idx] = error;
    dtcStatus_[idx] = status;
}

} // namespace Model
} // namespace obd
