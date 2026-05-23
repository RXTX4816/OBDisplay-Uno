#pragma once

#include <Arduino.h>
#include "../Model/OBDSignals.h"

namespace obd
{
namespace KWP
{

// Decode one KWP-1281 measurement block entry and write results into signals.
// Covers all 56 k-type formulas (VW/Audi table, proven with Golf Mk4).
// Also updates the experimental group arrays and maps to named signal fields.
void processKwpMeasurement(uint8_t ecuAddr, uint8_t group, int idx, byte k, byte a, byte b,
                           Model::OBDSignals& signals);

} // namespace KWP
} // namespace obd
