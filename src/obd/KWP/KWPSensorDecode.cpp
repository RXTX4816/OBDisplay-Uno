// SPDX-License-Identifier: GPL-3.0-or-later
#include "KWPSensorDecode.h"

namespace obd
{
namespace KWP
{

void processKwpMeasurement(uint8_t ecuAddr, uint8_t group, int idx, byte k, byte a, byte b,
                           Model::OBDSignals& signals)
{
    float v = 0;
    const __FlashStringHelper* units = F("");

    // Full VW/Audi KWP-1281 measurement value type table.
    // Formulas from the classic obdisplay.cpp (proven with Golf Mk4).
    switch (k)
    {
        case 1:
            v = 0.2f * a * b;
            units = F("rpm");
            break;
        case 2:
            v = a * 0.002f * b;
            units = F("%%");
            break;
        case 3:
            v = 0.002f * a * b;
            units = F("Deg");
            break;
        case 4:
            v = abs(b - 127) * 0.01f * a;
            units = F("ATDC");
            break;
        case 5:
            v = a * (b - 100) * 0.1f;
            units = F("\xB0C");
            break;
        case 6:
            v = 0.001f * a * b;
            units = F("V");
            break;
        case 7:
            v = 0.01f * a * b;
            units = F("km/h");
            break;
        case 8:
            v = 0.1f * a * b;
            units = F(" ");
            break;
        case 9:
            v = (b - 127) * 0.02f * a;
            units = F("Deg");
            break;
        case 10:
            v = b;
            units = b ? F("WARM") : F("COLD");
            break;
        case 11:
            v = 0.0001f * a * (b - 128) + 1;
            units = F(" ");
            break;
        case 12:
            v = 0.001f * a * b;
            units = F("Ohm");
            break;
        case 13:
            v = (b - 127) * 0.001f * a;
            units = F("mm");
            break;
        case 14:
            v = 0.005f * a * b;
            units = F("bar");
            break;
        case 15:
            v = 0.01f * a * b;
            units = F("ms");
            break;
        case 18:
            v = 0.04f * a * b;
            units = F("mbar");
            break;
        case 19:
            v = a * b * 0.01f;
            units = F("l");
            break;
        case 20:
            v = a * (b - 128) / 128.0f;
            units = F("%%");
            break;
        case 21:
            v = 0.001f * a * b;
            units = F("V");
            break;
        case 22:
            v = 0.001f * a * b;
            units = F("ms");
            break;
        case 23:
            v = b / 256.0f * a;
            units = F("%%");
            break;
        case 24:
            v = 0.001f * a * b;
            units = F("A");
            break;
        case 25:
            v = (b * 1.421f) + (a / 182.0f);
            units = F("g/s");
            break;
        case 26:
            v = float(b - a);
            units = F("C");
            break;
        case 27:
            v = abs(b - 128) * 0.01f * a;
            units = F("\xB0");
            break;
        case 28:
            v = float(b - a);
            units = F(" ");
            break;
        case 30:
            v = b / 12.0f * a;
            units = F("Degk/w");
            break;
        case 31:
            v = b / 2560.0f * a;
            units = F("\xB0C");
            break;
        case 33:
            v = 100.0f * b / a;
            units = F("%%");
            break;
        case 34:
            v = (b - 128) * 0.01f * a;
            units = F("kW");
            break;
        case 35:
            v = 0.01f * a * b;
            units = F("l/h");
            break;
        case 36:
            v = ((uint32_t)a) * 2560UL + ((uint32_t)b) * 10UL;
            units = F("km");
            break;
        case 37:
            v = b;
            units = F(" ");
            break;
        case 38:
            v = (b - 128) * 0.001f * a;
            units = F("Degk/w");
            break;
        case 39:
            v = b / 256.0f * a;
            units = F("mg/h");
            break;
        case 40:
            v = b * 0.1f + (25.5f * a) - 400.0f;
            units = F("A");
            break;
        case 41:
            v = b + a * 255.0f;
            units = F("Ah");
            break;
        case 42:
            v = b * 0.1f + (25.5f * a) - 400.0f;
            units = F("Kw");
            break;
        case 43:
            v = b * 0.1f + (25.5f * a);
            units = F("V");
            break;
        case 45:
            v = 0.1f * a * b / 100.0f;
            units = F(" ");
            break;
        case 46:
            v = (a * b - 3200.0f) * 0.0027f;
            units = F("Degk/w");
            break;
        case 47:
            v = (b - 128) * (float)a;
            units = F("ms");
            break;
        case 48:
            v = b + a * 255.0f;
            units = F(" ");
            break;
        case 49:
            v = (b / 4.0f) * a * 0.1f;
            units = F("mg/h");
            break;
        case 50:
            v = (b - 128.0f) / (0.01f * a);
            units = F("mbar");
            break;
        case 51:
            v = ((b - 128.0f) / 255.0f) * a;
            units = F("mg/h");
            break;
        case 52:
            v = b * 0.02f * a - a;
            units = F("Nm");
            break;
        case 53:
            v = (b - 128) * 1.4222f + 0.006f * a;
            units = F("g/s");
            break;
        case 54:
            v = a * 256.0f + b;
            units = F("count");
            break;
        case 55:
            v = a * b / 200.0f;
            units = F("s");
            break;
        case 56:
            v = a * 256.0f + b;
            units = F(" ");
            break;
        default:
            break;
    }

    // Update experimental group arrays
    if (signals.experimental.k[idx] != k)
    {
        signals.experimental.k[idx] = k;
        signals.experimental.kUpdated = true;
    }
    if (signals.experimental.v[idx] != v)
    {
        signals.experimental.v[idx] = v;
        signals.experimental.vUpdated = true;
    }
    char firstChar = pgm_read_byte(reinterpret_cast<const char*>(units));
    if (signals.experimental.unit[idx][0] != firstChar)
    {
        uint8_t j = 0;
        for (; j < obd::Model::ExperimentalGroup::UnitWidth; ++j)
        {
            char c = pgm_read_byte(
                reinterpret_cast<const char*>(reinterpret_cast<uintptr_t>(units) + j));
            if (c == '\0')
                break;
            signals.experimental.unit[idx][j] = c;
        }
        if (j <= obd::Model::ExperimentalGroup::UnitWidth)
        {
            signals.experimental.unit[idx][j] = '\0';
        }
        for (++j; j < obd::Model::ExperimentalGroup::UnitWidth + 1; ++j)
        {
            signals.experimental.unit[idx][j] = '\0';
        }
        signals.experimental.unitUpdated = true;
    }

    // Map decoded value into named signal fields
    switch (ecuAddr)
    {
        case 0x17:
        { // ADDR_INSTRUMENTS
            switch (group)
            {
                case 1:
                    switch (idx)
                    {
                        case 0:
                        {
                            uint16_t value = (uint16_t)v;
                            if (signals.instruments.vehicleSpeed != value)
                            {
                                signals.instruments.vehicleSpeed = value;
                                signals.instruments.vehicleSpeedUpdated = true;
                            }
                            break;
                        }
                        case 1:
                        {
                            uint16_t value = (uint16_t)v;
                            if (signals.instruments.engineRpm != value)
                            {
                                signals.instruments.engineRpm = value;
                                signals.instruments.engineRpmUpdated = true;
                            }
                            break;
                        }
                        case 2:
                        {
                            uint16_t value = (uint16_t)v;
                            if (signals.instruments.oilPressureMin != value)
                            {
                                signals.instruments.oilPressureMin = value;
                                signals.instruments.oilPressureMinUpdated = true;
                            }
                            break;
                        }
                        case 3:
                        {
                            uint32_t value = (uint32_t)v;
                            if (signals.instruments.timeEcu != value)
                            {
                                signals.instruments.timeEcu = value;
                                signals.instruments.timeEcuUpdated = true;
                            }
                            break;
                        }
                    }
                    break;
                case 2:
                    switch (idx)
                    {
                        case 0:
                        {
                            uint32_t value = (uint32_t)v;
                            if (signals.instruments.odometer != value)
                            {
                                signals.instruments.odometer = value;
                                signals.instruments.odometerUpdated = true;
                            }
                            break;
                        }
                        case 1:
                        {
                            uint8_t value = (uint8_t)v;
                            if (signals.instruments.fuelLevel != value)
                            {
                                signals.instruments.fuelLevel = value;
                                signals.instruments.fuelLevelUpdated = true;
                            }
                            break;
                        }
                        case 2:
                        {
                            uint16_t value = (uint16_t)v;
                            if (signals.instruments.fuelSensorResistance != value)
                            {
                                signals.instruments.fuelSensorResistance = value;
                                signals.instruments.fuelSensorResistanceUpdated = true;
                            }
                            break;
                        }
                        case 3:
                        {
                            uint8_t value = (uint8_t)v;
                            if (signals.instruments.ambientTemp != value)
                            {
                                signals.instruments.ambientTemp = value;
                                signals.instruments.ambientTempUpdated = true;
                            }
                            break;
                        }
                    }
                    break;
                case 3:
                    switch (idx)
                    {
                        case 0:
                        {
                            uint8_t value = (uint8_t)v;
                            if (signals.instruments.coolantTemp != value)
                            {
                                signals.instruments.coolantTemp = value;
                                signals.instruments.coolantTempUpdated = true;
                            }
                            break;
                        }
                        case 1:
                        {
                            uint8_t value = (uint8_t)v;
                            if (signals.instruments.oilLevelOk != value)
                            {
                                signals.instruments.oilLevelOk = value;
                                signals.instruments.oilLevelOkUpdated = true;
                            }
                            break;
                        }
                        case 2:
                        {
                            uint8_t value = (uint8_t)v;
                            if (signals.instruments.oilTemp != value)
                            {
                                signals.instruments.oilTemp = value;
                                signals.instruments.oilTempUpdated = true;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        case 0x01:
        { // ADDR_ENGINE
            switch (group)
            {
                case 1:
                    switch (idx)
                    {
                        case 0:
                        {
                            uint16_t value = (uint16_t)v;
                            if (signals.instruments.engineRpm != value)
                            {
                                signals.instruments.engineRpm = value;
                                signals.instruments.engineRpmUpdated = true;
                            }
                            break;
                        }
                        case 1:
                        {
                            uint8_t value = (uint8_t)v;
                            if (signals.engine.tempUnknown1 != value)
                            {
                                signals.engine.tempUnknown1 = value;
                                signals.engine.tempUnknown1Updated = true;
                            }
                            break;
                        }
                        case 2:
                        {
                            int8_t value = (int8_t)v;
                            if (signals.engine.lambda != value)
                            {
                                signals.engine.lambda = value;
                                signals.engine.lambdaUpdated = true;
                            }
                            break;
                        }
                        default:
                            break;
                    }
                    break;
                case 3:
                    switch (idx)
                    {
                        case 1:
                        {
                            uint16_t value = (uint16_t)v;
                            if (signals.engine.pressure != value)
                            {
                                signals.engine.pressure = value;
                                signals.engine.pressureUpdated = true;
                            }
                            break;
                        }
                        case 2:
                        {
                            float value = v;
                            if (signals.engine.tbAngle != value)
                            {
                                signals.engine.tbAngle = value;
                                signals.engine.tbAngleUpdated = true;
                            }
                            break;
                        }
                        case 3:
                        {
                            float value = v;
                            if (signals.engine.steeringAngle != value)
                            {
                                signals.engine.steeringAngle = value;
                                signals.engine.steeringAngleUpdated = true;
                            }
                            break;
                        }
                    }
                    break;
                case 4:
                    switch (idx)
                    {
                        case 1:
                        {
                            float value = v;
                            if (signals.engine.voltage != value)
                            {
                                signals.engine.voltage = value;
                                signals.engine.voltageUpdated = true;
                            }
                            break;
                        }
                        case 2:
                        {
                            uint8_t value = (uint8_t)v;
                            if (signals.engine.tempUnknown2 != value)
                            {
                                signals.engine.tempUnknown2 = value;
                                signals.engine.tempUnknown2Updated = true;
                            }
                            break;
                        }
                        case 3:
                        {
                            uint8_t value = (uint8_t)v;
                            if (signals.engine.tempUnknown3 != value)
                            {
                                signals.engine.tempUnknown3 = value;
                                signals.engine.tempUnknown3Updated = true;
                            }
                            break;
                        }
                    }
                    break;
                case 6:
                    switch (idx)
                    {
                        case 1:
                        {
                            uint16_t value = (uint16_t)v;
                            if (signals.engine.engineLoad != value)
                            {
                                signals.engine.engineLoad = value;
                                signals.engine.engineLoadUpdated = true;
                            }
                            break;
                        }
                        case 3:
                        {
                            int8_t value = (int8_t)v;
                            if (signals.engine.lambda2 != value)
                            {
                                signals.engine.lambda2 = value;
                                signals.engine.lambda2Updated = true;
                            }
                            break;
                        }
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}

} // namespace KWP
} // namespace obd
