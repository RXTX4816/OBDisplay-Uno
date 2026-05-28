// SPDX-License-Identifier: GPL-3.0-or-later
#include "KWPSensorDecode.h"
#include <avr/pgmspace.h>
#include <string.h> // memcpy_P

namespace obd
{
namespace KWP
{

// ---------------------------------------------------------------------------
// Formula table (PROGMEM)
//
// Stores the computation type + up to 3 float coefficients for each KWP
// measurement type ID (k=0..56). Replaces the per-case float constants that
// were previously embedded in 50+ separate code paths.
//
// To further reduce flash once you know which type IDs your ECU actually
// emits: change unused entries to {F_SPECIAL,...} so their formula code is
// never reached — compiler/LTO will eliminate unreachable branches.
//
// Formula families:
//   F_AB_C      v = c1 * a * b
//   F_BK_CA     v = (b - c2) * c1 * a          c2 = K offset
//   F_ABS_BK_CA v = |b - c2| * c1 * a          c2 = K offset
//   F_B         v = (float)b
//   F_B_A       v = (float)(b - a)
//   F_AK_B      v = a * c1 + b                 c1 = K (e.g. 256)
//   F_B_AK      v = b + a * c1                 c1 = K (e.g. 255)
//   F_LINEAR    v = b*c1 + a*c2 + c3
//   F_SPECIAL   handled inline in computeSpecial()
// ---------------------------------------------------------------------------

enum FormulaType : uint8_t
{
    F_AB_C = 0,
    F_BK_CA,
    F_ABS_BK_CA,
    F_B,
    F_B_A,
    F_AK_B,
    F_B_AK,
    F_LINEAR,
    F_SPECIAL,
};

struct __attribute__((packed)) KWPEntry
{
    FormulaType type;
    float c1;
    float c2;
    float c3;
};

static const KWPEntry kwp_table[57] PROGMEM = {
    /* 0  unused */ {F_SPECIAL, 0, 0, 0},
    /* 1  rpm    */ {F_AB_C, 0.2f, 0, 0},
    /* 2  %%     */ {F_AB_C, 0.002f, 0, 0},
    /* 3  Deg    */ {F_AB_C, 0.002f, 0, 0},
    /* 4  ATDC   */ {F_ABS_BK_CA, 0.01f, 127, 0},
    /* 5  °C     */ {F_BK_CA, 0.1f, 100, 0},
    /* 6  V      */ {F_AB_C, 0.001f, 0, 0},
    /* 7  km/h   */ {F_AB_C, 0.01f, 0, 0},
    /* 8  raw    */ {F_AB_C, 0.1f, 0, 0},
    /* 9  Deg    */ {F_BK_CA, 0.02f, 127, 0},
    /* 10 WARM   */ {F_SPECIAL, 0, 0, 0},
    /* 11 lambda */ {F_SPECIAL, 0, 0, 0},
    /* 12 Ohm    */ {F_AB_C, 0.001f, 0, 0},
    /* 13 mm     */ {F_BK_CA, 0.001f, 127, 0},
    /* 14 bar    */ {F_AB_C, 0.005f, 0, 0},
    /* 15 ms     */ {F_AB_C, 0.01f, 0, 0},
    /* 16 unused */ {F_SPECIAL, 0, 0, 0},
    /* 17 unused */ {F_SPECIAL, 0, 0, 0},
    /* 18 mbar   */ {F_AB_C, 0.04f, 0, 0},
    /* 19 l      */ {F_AB_C, 0.01f, 0, 0},
    /* 20 %%     */ {F_BK_CA, 0.0078125f, 128, 0},
    /* 21 V      */ {F_AB_C, 0.001f, 0, 0},
    /* 22 ms     */ {F_AB_C, 0.001f, 0, 0},
    /* 23 %%     */ {F_AB_C, 0.00390625f, 0, 0},
    /* 24 A      */ {F_AB_C, 0.001f, 0, 0},
    /* 25 g/s    */ {F_LINEAR, 1.421f, 0.005494f, 0},
    /* 26 C      */ {F_B_A, 0, 0, 0},
    /* 27 °      */ {F_ABS_BK_CA, 0.01f, 128, 0},
    /* 28 raw    */ {F_B_A, 0, 0, 0},
    /* 29 unused */ {F_SPECIAL, 0, 0, 0},
    /* 30 Dk/w   */ {F_AB_C, 0.083333f, 0, 0},
    /* 31 °C     */ {F_AB_C, 0.000390625f, 0, 0},
    /* 32 unused */ {F_SPECIAL, 0, 0, 0},
    /* 33 %%     */ {F_SPECIAL, 0, 0, 0},
    /* 34 kW     */ {F_BK_CA, 0.01f, 128, 0},
    /* 35 l/h    */ {F_AB_C, 0.01f, 0, 0},
    /* 36 km     */ {F_SPECIAL, 0, 0, 0},
    /* 37 raw    */ {F_B, 0, 0, 0},
    /* 38 Dk/w   */ {F_BK_CA, 0.001f, 128, 0},
    /* 39 mg/h   */ {F_AB_C, 0.00390625f, 0, 0},
    /* 40 A      */ {F_LINEAR, 0.1f, 25.5f, -400.0f},
    /* 41 Ah     */ {F_B_AK, 255.0f, 0, 0},
    /* 42 Kw     */ {F_LINEAR, 0.1f, 25.5f, -400.0f},
    /* 43 V      */ {F_LINEAR, 0.1f, 25.5f, 0},
    /* 44 unused */ {F_SPECIAL, 0, 0, 0},
    /* 45 raw    */ {F_AB_C, 0.001f, 0, 0},
    /* 46 Dk/w   */ {F_SPECIAL, 0, 0, 0},
    /* 47 ms     */ {F_BK_CA, 1.0f, 128, 0},
    /* 48 raw    */ {F_B_AK, 255.0f, 0, 0},
    /* 49 mg/h   */ {F_AB_C, 0.025f, 0, 0},
    /* 50 mbar   */ {F_SPECIAL, 0, 0, 0},
    /* 51 mg/h   */ {F_BK_CA, 0.003922f, 128, 0},
    /* 52 Nm     */ {F_SPECIAL, 0, 0, 0},
    /* 53 g/s    */ {F_LINEAR, 1.4222f, 0.006f, -182.04f},
    /* 54 count  */ {F_AK_B, 256.0f, 0, 0},
    /* 55 s      */ {F_AB_C, 0.005f, 0, 0},
    /* 56 raw    */ {F_AK_B, 256.0f, 0, 0},
};

// Read a float from a packed PROGMEM address (handles unaligned access safely).
static float pgmFloat(const float* addr)
{
    float f;
    memcpy_P(&f, addr, sizeof(float));
    return f;
}

// Apply the tabulated formula for measurement type k.
static float computeFormula(uint8_t k, byte a, byte b)
{
    const KWPEntry* e = &kwp_table[k];
    switch ((FormulaType)pgm_read_byte(&e->type))
    {
        case F_AB_C:
            return pgmFloat(&e->c1) * (float)a * (float)b;

        case F_BK_CA:
            return ((float)b - pgmFloat(&e->c2)) * pgmFloat(&e->c1) * (float)a;

        case F_ABS_BK_CA:
        {
            float d = (float)b - pgmFloat(&e->c2);
            if (d < 0)
                d = -d;
            return d * pgmFloat(&e->c1) * (float)a;
        }

        case F_B:
            return (float)b;

        case F_B_A:
            return (float)b - (float)a;

        case F_AK_B:
            return (float)a * pgmFloat(&e->c1) + (float)b;

        case F_B_AK:
            return (float)b + (float)a * pgmFloat(&e->c1);

        case F_LINEAR:
            return (float)b * pgmFloat(&e->c1) + (float)a * pgmFloat(&e->c2) + pgmFloat(&e->c3);

        default:
            return 0;
    }
}

void processKwpMeasurement(uint8_t ecuAddr, uint8_t group, int idx, byte k, byte a, byte b,
                           Model::OBDSignals& signals)
{
    float v = 0;
    const __FlashStringHelper* units = F("");

    // Dispatch formula via table; F_SPECIAL cases fall through to the switch below.
    if (k < 57 && (FormulaType)pgm_read_byte(&kwp_table[k].type) != F_SPECIAL)
    {
        v = computeFormula(k, a, b);
    }
    else
    {
        // Residual switch: 8 cases that don't fit a simple formula family.
        switch (k)
        {
            case 10:
                v = b;
                units = b ? F("WARM") : F("COLD"); // unit handled here; skip unit switch below
                break;
            case 11:
                v = 0.0001f * (float)a * ((float)b - 128.0f) + 1.0f;
                break;
            case 33:
                v = 100.0f * (float)b / (float)a;
                break;
            case 36:
                v = (float)(((uint32_t)a) * 2560UL + ((uint32_t)b) * 10UL);
                break;
            case 46:
                v = ((float)a * (float)b - 3200.0f) * 0.0027f;
                break;
            case 50:
                v = ((float)b - 128.0f) / (0.01f * (float)a);
                break;
            case 52:
                v = (float)b * 0.02f * (float)a - (float)a;
                break;
            default:
                break;
        }
    }

    // Unit string assignment (k=10 already set units above).
    if (k != 10)
    {
        switch (k)
        {
            case 1:
                units = F("rpm");
                break;
            case 2:
            case 20:
            case 23:
            case 33:
                units = F("%%");
                break;
            case 3:
            case 9:
            case 13:
                units = F("Deg");
                break;
            case 4:
                units = F("ATDC");
                break;
            case 5:
            case 31:
                units = F("\xB0"
                          "C");
                break;
            case 6:
            case 21:
            case 43:
                units = F("V");
                break;
            case 7:
                units = F("km/h");
                break;
            case 8:
            case 11:
            case 28:
            case 37:
            case 45:
            case 48:
            case 56:
                units = F(" ");
                break;
            case 12:
                units = F("Ohm");
                break;
            case 14:
                units = F("bar");
                break;
            case 15:
            case 22:
            case 47:
                units = F("ms");
                break;
            case 18:
            case 50:
                units = F("mbar");
                break;
            case 19:
                units = F("l");
                break;
            case 24:
            case 40:
                units = F("A");
                break;
            case 25:
            case 53:
                units = F("g/s");
                break;
            case 26:
                units = F("C");
                break;
            case 27:
                units = F("\xB0");
                break;
            case 30:
            case 38:
            case 46:
                units = F("Degk/w");
                break;
            case 34:
                units = F("kW");
                break;
            case 35:
                units = F("l/h");
                break;
            case 36:
                units = F("km");
                break;
            case 39:
            case 49:
            case 51:
                units = F("mg/h");
                break;
            case 41:
                units = F("Ah");
                break;
            case 42:
                units = F("Kw");
                break;
            case 52:
                units = F("Nm");
                break;
            case 54:
                units = F("count");
                break;
            case 55:
                units = F("s");
                break;
            default:
                break;
        }
    }

    // Update experimental group arrays — only for the group currently being displayed,
    // so the screen always shows data from the selected group rather than the last
    // group read (which would always be group 3 in ReadSensors mode).
    if (group != signals.experimental.groupCurrent)
        goto signal_mapping;

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

signal_mapping:
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
