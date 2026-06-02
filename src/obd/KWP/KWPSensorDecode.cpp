// SPDX-License-Identifier: GPL-3.0-or-later
#include "KWPSensorDecode.h"
#include <avr/pgmspace.h>

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
    F_BK_CA = 1,
    // 2 was F_ABS_BK_CA — now encoded as F_BK_CA with F_ABS_FLAG set
    F_B = 3,
    F_B_A = 4,
    F_AK_B = 5, // v = a*c1 + b  (F_B_AK merged here: b+a*c1 is identical)
    F_LINEAR = 6,
    F_SPECIAL = 7,
};
// Bit 6 of typeAndC2: absolute-value flag for F_BK_CA (replaces F_ABS_BK_CA).
static constexpr uint8_t F_ABS_FLAG = 0x40;

// c2_idx values (bits[5:4] of typeAndC2) → actual c2 for BK_CA formula types.
static const uint8_t kC2[4] PROGMEM = {0, 100, 127, 128};

struct __attribute__((packed)) KWPEntry
{
    uint8_t typeAndC2; // bits[2:0]=FormulaType, bits[5:4]=c2_idx, bit[6]=F_ABS_FLAG
    int16_t c1_s;      // c1 × 1000 (e.g. 0.2 → 200, 1.421 → 1421)
};

// 57 entries × 3 bytes = 171 bytes  (was 741 bytes with 3 floats each)
// c1_s = c1 × 1000 stored as int16_t.
// k=20,23,31,39,41,48,51,54,56 → F_SPECIAL (non-power-of-2 or large c1, handled in switch).
// k=25,40,42,43,53 → F_LINEAR (handled in computeFormula with hardcoded integer c2/c3).
static const KWPEntry kwp_table[71] PROGMEM = {
    /* 0  unused  */ {F_SPECIAL, 0},
    /* 1  rpm     */ {F_AB_C, 200},                         // 0.2
    /* 2  %%      */ {F_AB_C, 2},                           // 0.002
    /* 3  Deg     */ {F_AB_C, 2},                           // 0.002
    /* 4  ATDC    */ {(2 << 4) | F_ABS_FLAG | F_BK_CA, 10}, // 0.01, c2=127, abs
    /* 5  °C      */ {(1 << 4) | F_BK_CA, 100},             // 0.1,  c2=100
    /* 6  V       */ {F_AB_C, 1},                           // 0.001
    /* 7  km/h    */ {F_AB_C, 10},                          // 0.01
    /* 8  raw     */ {F_AB_C, 100},                         // 0.1
    /* 9  Deg     */ {(2 << 4) | F_BK_CA, 20},              // 0.02, c2=127
    /* 10 WARM    */ {F_SPECIAL, 0},
    /* 11 lambda  */ {F_SPECIAL, 0},
    /* 12 Ohm     */ {F_AB_C, 1},             // 0.001
    /* 13 mm      */ {(2 << 4) | F_BK_CA, 1}, // 0.001, c2=127
    /* 14 bar     */ {F_AB_C, 5},             // 0.005
    /* 15 ms      */ {F_AB_C, 10},            // 0.01
    /* 16 Bin.Bit */ {F_SPECIAL, 0},
    /* 17 chr     */ {F_SPECIAL, 0},
    /* 18 mbar    */ {F_AB_C, 40},     // 0.04
    /* 19 l       */ {F_AB_C, 10},     // 0.01
    /* 20 %%      */ {F_SPECIAL, 0},   // (b-128)*a/128
    /* 21 V       */ {F_AB_C, 1},      // 0.001
    /* 22 ms      */ {F_AB_C, 1},      // 0.001
    /* 23 %%      */ {F_SPECIAL, 0},   // a*b/256
    /* 24 A       */ {F_AB_C, 1},      // 0.001
    /* 25 g/s     */ {F_LINEAR, 1421}, // 1.421, c2/c3 hardcoded in dispatch
    /* 26 C       */ {F_B_A, 0},
    /* 27 °       */ {(3 << 4) | F_ABS_FLAG | F_BK_CA, 10}, // 0.01, c2=128, abs
    /* 28 raw     */ {F_B_A, 0},
    /* 29 Kennfd  */ {F_SPECIAL, 0},
    /* 30 Dk/w    */ {F_AB_C, 83},   // 0.083333 ≈ 83/1000
    /* 31 °C      */ {F_SPECIAL, 0}, // a*b/2560
    /* 32 signed  */ {F_SPECIAL, 0},
    /* 33 %%      */ {F_SPECIAL, 0},
    /* 34 kW      */ {(3 << 4) | F_BK_CA, 10}, // 0.01, c2=128
    /* 35 l/h     */ {F_AB_C, 10},             // 0.01
    /* 36 km      */ {F_SPECIAL, 0},
    /* 37 raw     */ {F_B, 0},
    /* 38 Dk/w    */ {(3 << 4) | F_BK_CA, 1}, // 0.001, c2=128
    /* 39 mg/h    */ {F_SPECIAL, 0},          // a*b/256
    /* 40 A       */ {F_LINEAR, 100},         // 0.1, c2/c3 hardcoded
    /* 41 Ah      */ {F_SPECIAL, 0},          // 255*a+b
    /* 42 Kw      */ {F_LINEAR, 100},         // 0.1, c2/c3 hardcoded
    /* 43 V       */ {F_LINEAR, 100},         // 0.1, c2/c3 hardcoded
    /* 44 h:m     */ {F_SPECIAL, 0},
    /* 45 raw     */ {F_AB_C, 1}, // 0.001
    /* 46 Dk/w    */ {F_SPECIAL, 0},
    /* 47 ms      */ {(3 << 4) | F_BK_CA, 1000}, // 1.0, c2=128
    /* 48 raw     */ {F_SPECIAL, 0},             // 255*a+b
    /* 49 mg/h    */ {F_AB_C, 25},               // 0.025
    /* 50 mbar    */ {F_SPECIAL, 0},
    /* 51 mg/h    */ {F_SPECIAL, 0}, // (b-128)*a/255
    /* 52 Nm      */ {F_SPECIAL, 0},
    /* 53 g/s     */ {F_LINEAR, 1422}, // 1.4222, c2/c3 hardcoded
    /* 54 count   */ {F_SPECIAL, 0},   // 256*a+b
    /* 55 s       */ {F_AB_C, 5},      // a*b/200
    /* 56 WSC     */ {F_SPECIAL, 0},   // 256*a+b
    /* 57 WSC     */ {F_SPECIAL, 0},   // 256*a+b+65536
    /* 58 -       */ {F_SPECIAL, 0},   // 1.0225*b (or 1.0225*(256-b) if b>128)
    /* 59 -       */ {F_SPECIAL, 0},   // (256*a+b)/32768
    /* 60 sec     */ {F_SPECIAL, 0},   // (256*a+b)*0.01
    /* 61 -       */ {F_SPECIAL, 0},   // (b-128)/a
    /* 62 S       */ {F_AB_C, 256},    // 0.256*a*b
    /* 63 txt     */ {F_SPECIAL, 0},
    /* 64 Ohm     */ {F_SPECIAL, 0},           // a+b
    /* 65 mm      */ {(2 << 4) | F_BK_CA, 10}, // 0.01*a*(b-127), c2=127
    /* 66 V       */ {F_SPECIAL, 0},           // a*b/511.12
    /* 67 Deg     */ {F_SPECIAL, 0},           // 640*a+2.5*b
    /* 68 deg/s   */ {F_SPECIAL, 0},           // (256*a+b)/7.365
    /* 69 bar     */ {F_SPECIAL, 0},           // (256*a+b)*0.3254
    /* 70 m/s2    */ {F_SPECIAL, 0},           // (256*a+b)*0.192
};

// ---------------------------------------------------------------------------
// Unit string table — replaces the large switch(k) in processKwpMeasurement.
// kUnitIdx[k] → index into kUnitStrs (0 = empty / unassigned).
// ---------------------------------------------------------------------------
static const char kU_empty[] PROGMEM = "";
static const char kU_rpm[] PROGMEM = "rpm";
static const char kU_pct[] PROGMEM = "%%";
static const char kU_deg[] PROGMEM = "Deg";
static const char kU_atdc[] PROGMEM = "ATDC";
static const char kU_degC[] PROGMEM = "\xB0"
                                      "C";
static const char kU_V[] PROGMEM = "V";
static const char kU_kmh[] PROGMEM = "km/h";
static const char kU_sp[] PROGMEM = " ";
static const char kU_ohm[] PROGMEM = "Ohm";
static const char kU_bar[] PROGMEM = "bar";
static const char kU_ms[] PROGMEM = "ms";
static const char kU_mbar[] PROGMEM = "mbar";
static const char kU_l[] PROGMEM = "l";
static const char kU_A[] PROGMEM = "A";
static const char kU_gs[] PROGMEM = "g/s";
static const char kU_C[] PROGMEM = "C";
static const char kU_degSym[] PROGMEM = "\xB0";
static const char kU_degkw[] PROGMEM = "Degk/w";
static const char kU_kW[] PROGMEM = "kW";
static const char kU_lh[] PROGMEM = "l/h";
static const char kU_km[] PROGMEM = "km";
static const char kU_mgh[] PROGMEM = "mg/h";
static const char kU_Ah[] PROGMEM = "Ah";
static const char kU_Kw[] PROGMEM = "Kw";
static const char kU_Nm[] PROGMEM = "Nm";
static const char kU_count[] PROGMEM = "count";
static const char kU_s[] PROGMEM = "s";
static const char kU_hm[] PROGMEM = "h:m";
static const char kU_WSC[] PROGMEM = "WSC";
static const char kU_sec[] PROGMEM = "sec";
static const char kU_S[] PROGMEM = "S";
static const char kU_mm[] PROGMEM = "mm";
static const char kU_degs[] PROGMEM = "deg/s";
static const char kU_ms2[] PROGMEM = "m/s2";

// clang-format off
static PGM_P const kUnitStrs[] PROGMEM = {
    kU_empty, kU_rpm, kU_pct,  kU_deg,  kU_atdc, kU_degC, kU_V,    kU_kmh,  // 0-7
    kU_sp,    kU_ohm, kU_bar,  kU_ms,   kU_mbar, kU_l,    kU_A,    kU_gs,   // 8-15
    kU_C,     kU_degSym, kU_degkw, kU_kW, kU_lh, kU_km,  kU_mgh,  kU_Ah,   // 16-23
    kU_Kw,    kU_Nm,  kU_count, kU_s,   kU_hm,  kU_WSC,  kU_sec,  kU_S,    // 24-31
    kU_mm,    kU_degs, kU_ms2,                                                // 32-34
};

// One byte per k-value (0–70): index into kUnitStrs[] above.
static const uint8_t kUnitIdx[71] PROGMEM = {
//  0    1    2    3    4    5    6    7    8    9
    0,   1,   2,   3,   4,   5,   6,   7,   8,   3,  //  0- 9
    0,   8,   9,   3,  10,  11,   0,   0,  12,  13,  // 10-19
    2,   6,  11,   2,  14,  15,  16,  17,   8,   0,  // 20-29
   18,   5,   0,   2,  19,  20,  21,   8,  18,  22,  // 30-39
   14,  23,  24,   6,  28,   8,  18,  11,   8,  22,  // 40-49
   12,  22,  25,  15,  26,  27,  29,  29,   0,   0,  // 50-59
   30,   0,  31,   0,   9,  32,   0,   0,  33,  10,  // 60-69
   34,                                                 // 70
};
// clang-format on

// Apply the tabulated formula for measurement type k.
// Returns value ×10 as int32_t (e.g. 1230 = 123.0).
static int32_t computeFormula(uint8_t k, byte a, byte b)
{
    const KWPEntry* e = &kwp_table[k];
    uint8_t raw = pgm_read_byte(&e->typeAndC2);
    FormulaType t = (FormulaType)(raw & 0x07);
    int16_t c1s = (int16_t)pgm_read_word(&e->c1_s);

    // All formulae: v_x10 = (c1_s/1000) * formula_result * 10
    //             = c1_s * formula_result / 100
    switch (t)
    {
        case F_AB_C:
            return (int32_t)c1s * a * b / 100;

        case F_BK_CA:
        {
            int16_t c2 = (int16_t)pgm_read_byte(&kC2[(raw >> 4) & 0x03]);
            int16_t d = (int16_t)b - c2;
            if ((raw & F_ABS_FLAG) && d < 0)
                d = -d;
            return (int32_t)c1s * d * a / 100;
        }

        case F_B:
            return (int32_t)b * 10;

        case F_B_A:
            return (int32_t)((int16_t)b - (int16_t)a) * 10;

        case F_AK_B:
            return (int32_t)c1s * a / 100 + (int32_t)b * 10; // (a*c1+b)×10

        case F_LINEAR:
        {
            // c2 and c3 hardcoded per k, all in integer form.
            // Formula: v×10 = c1s*b/100 + c2_x10*a/1000 + c3_x10
            // k=25:  1.421*b + 0.005494*a        → c1s=1421, c2_x100=5494, c3=0
            // k=40,42: 0.1*b + 25.5*a - 400      → 10*b + 255*a - 4000 (×10)
            // k=43:  0.1*b + 25.5*a              → 10*b + 255*a
            // k=53:  1.4222*b + 0.006*a - 182.04 → 14222*b/1000 + 6*a/1000 - 1820 (×10)
            switch (k)
            {
                case 25:
                    return (int32_t)1421 * b / 100; // c2 term (5494*a/1M) ≈ 0
                case 40:
                case 42:
                    return (int32_t)b * 10 + (int32_t)255 * a - 4000L;
                case 43:
                    return (int32_t)b * 10 + (int32_t)255 * a;
                case 53:
                    return (int32_t)1422 * b / 100 + 6L * a / 100 - 1820L;
                default:
                    return 0;
            }
        }

        default:
            return 0;
    }
}

// Out-of-line helpers so LTO can share one function body for each type.
// Do NOT inline — that defeats the deduplication purpose.
static void setU8(uint8_t& f, bool& u, uint8_t v)
{
    if (f != v)
    {
        f = v;
        u = true;
    }
}
static void setU16(uint16_t& f, bool& u, uint16_t v)
{
    if (f != v)
    {
        f = v;
        u = true;
    }
}
static void setU32(uint32_t& f, bool& u, uint32_t v)
{
    if (f != v)
    {
        f = v;
        u = true;
    }
}
static void setI8(int8_t& f, bool& u, int8_t v)
{
    if (f != v)
    {
        f = v;
        u = true;
    }
}
static void setI16(int16_t& f, bool& u, int16_t v)
{
    if (f != v)
    {
        f = v;
        u = true;
    }
}

void processKwpMeasurement(uint8_t ecuAddr, uint8_t group, int idx, byte k, byte a, byte b,
                           Model::OBDSignals& signals)
{
    int32_t v = 0; // value ×10 fixed-point
    const __FlashStringHelper* units = F("");

    // Dispatch formula via table; F_SPECIAL cases fall through to the switch below.
    if (k < 71 && (FormulaType)(pgm_read_byte(&kwp_table[k].typeAndC2) & 0x0F) != F_SPECIAL)
    {
        v = computeFormula(k, a, b);
    }
    else
    {
        // Special cases: non-tabulated formulas, all integer arithmetic.
        // v is ×10 fixed-point throughout.
        switch (k)
        {
            case 10: // WARM/COLD flag
                v = (int32_t)b * 10;
                units = b ? F("WARM") : F("COLD");
                break;
            case 11: // lambda: 0.0001*a*(b-128)+1.0
                v = (int32_t)a * ((int16_t)b - 128) / 100 + 10;
                break;
            case 16: // Bin. Bits: 256*a + b
            case 17: // chr(a) chr(b) — show as raw 16-bit
                v = ((int32_t)a * 256 + b) * 10;
                break;
            case 20: // %%: (b-128)*a/128
                v = (int32_t)((int16_t)b - 128) * a * 10 / 128;
                break;
            case 23: // %%: a*b/256
            case 39: // mg/h: same coefficient
                v = (int32_t)a * b * 10 / 256;
                break;
            case 29: // Kennfeld: 1=first map, 2=second map
                v = (b < a) ? 10 : 20;
                break;
            case 31: // °C: a*b/2560
                v = (int32_t)a * b * 10 / 2560;
                break;
            case 32: // signed byte: (int8_t)b
                v = (b > 128) ? ((int32_t)b - 256) * 10 : (int32_t)b * 10;
                break;
            case 33: // %%: 100*b/a (if a==0: 100*b)
                v = (a > 0) ? (int32_t)b * 1000 / a : (int32_t)b * 1000;
                break;
            case 36: // km: a*2560+b*10
                v = ((int32_t)a * 2560L + (int32_t)b * 10L) * 10;
                break;
            case 41: // Ah: 255*a+b
            case 48: // raw: same
                v = ((int32_t)255 * a + b) * 10;
                break;
            case 44: // Uhrzeit h:m → total minutes
                v = ((int32_t)a * 60 + b) * 10;
                break;
            case 46: // Dk/w: (a*b-3200)*0.0027
                v = ((int32_t)a * b - 3200L) * 27L / 10000L * 10;
                break;
            case 50: // mbar: (b-128)/(0.01*a) = (b-128)*100/a
                v = (a > 0) ? (int32_t)((int16_t)b - 128) * 1000 / a : 0;
                break;
            case 51: // mg/h: (b-128)*a/255
                v = (int32_t)((int16_t)b - 128) * a * 10 / 255;
                break;
            case 52: // Nm: b*0.02*a - a = a*(0.02*b-1)
                v = (int32_t)a * ((int32_t)b * 20 - 1000L) / 100;
                break;
            case 54: // count: 256*a+b
            case 56: // WSC: same
                v = ((int32_t)256 * a + b) * 10;
                break;
            case 57: // WSC: 256*a+b+65536
                v = ((int32_t)256 * a + b + 65536L) * 10;
                break;
            case 58: // 1.0225*b (or 1.0225*(256-b) if b>128)
            {
                int16_t bv = (b > 128) ? (int16_t)(256 - b) : (int16_t)b;
                v = (int32_t)bv * 10225L / 1000L;
            }
            break;
            case 59: // (256*a+b)/32768
                v = ((int32_t)256 * a + b) * 10 / 32768L;
                break;
            case 60: // (256*a+b)*0.01 sec
                v = ((int32_t)256 * a + b) / 10;
                break;
            case 61: // (b-128)/a (if a==0: b-128)
                v = (a > 0) ? (int32_t)((int16_t)b - 128) * 10 / a
                            : (int32_t)((int16_t)b - 128) * 10;
                break;
            case 64: // a+b Ohm
                v = ((int32_t)a + b) * 10;
                break;
            case 66: // (a*b)/511.12
                v = (int32_t)a * b * 10 / 511;
                break;
            case 67: // 640*a + 2.5*b
                v = (int32_t)a * 6400L + (int32_t)b * 25;
                break;
            case 68: // (256*a+b)/7.365
                v = ((int32_t)256 * a + b) * 200L / 1473L;
                break;
            case 69: // (256*a+b)*0.3254
                v = ((int32_t)256 * a + b) * 3254L / 10000L;
                break;
            case 70: // (256*a+b)*0.192
                v = ((int32_t)256 * a + b) * 1920L / 10000L;
                break;
            default:
                break;
        }
    }

    // Unit string lookup via PROGMEM table (k=10 already set units above).
    if (k != 10 && k < 71)
    {
        uint8_t uidx = pgm_read_byte(&kUnitIdx[k]);
        if (uidx != 0)
            units = reinterpret_cast<__FlashStringHelper*>(pgm_read_word(&kUnitStrs[uidx]));
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
    {
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
    } // end firstChar block

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
                            setU16(signals.instruments.vehicleSpeed,
                                   signals.instruments.vehicleSpeedUpdated, (uint16_t)(v / 10));
                            break;
                        case 1:
                            setU16(signals.instruments.engineRpm,
                                   signals.instruments.engineRpmUpdated, (uint16_t)(v / 10));
                            break;
                        case 2:
                            setU16(signals.instruments.oilPressureMin,
                                   signals.instruments.oilPressureMinUpdated, (uint16_t)(v / 10));
                            break;
                        case 3:
                            setU32(signals.instruments.timeEcu, signals.instruments.timeEcuUpdated,
                                   (uint32_t)(v / 10));
                            break;
                    }
                    break;
                case 2:
                    switch (idx)
                    {
                        case 0:
                            setU32(signals.instruments.odometer,
                                   signals.instruments.odometerUpdated, (uint32_t)(v / 10));
                            break;
                        case 1:
                            setU8(signals.instruments.fuelLevel,
                                  signals.instruments.fuelLevelUpdated, (uint8_t)(v / 10));
                            break;
                        case 2:
                            setU16(signals.instruments.fuelSensorResistance,
                                   signals.instruments.fuelSensorResistanceUpdated,
                                   (uint16_t)(v / 10));
                            break;
                        case 3:
                            setU8(signals.instruments.ambientTemp,
                                  signals.instruments.ambientTempUpdated, (uint8_t)(v / 10));
                            break;
                    }
                    break;
                case 3:
                    switch (idx)
                    {
                        case 0:
                            setU8(signals.instruments.coolantTemp,
                                  signals.instruments.coolantTempUpdated, (uint8_t)(v / 10));
                            break;
                        case 1:
                            setU8(signals.instruments.oilLevelOk,
                                  signals.instruments.oilLevelOkUpdated, (uint8_t)(v / 10));
                            break;
                        case 2:
                            setU8(signals.instruments.oilTemp, signals.instruments.oilTempUpdated,
                                  (uint8_t)(v / 10));
                            break;
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
                            setU16(signals.instruments.engineRpm,
                                   signals.instruments.engineRpmUpdated, (uint16_t)(v / 10));
                            break;
                        case 1:
                            setU8(signals.engine.tempUnknown1, signals.engine.tempUnknown1Updated,
                                  (uint8_t)(v / 10));
                            break;
                        case 2:
                            setI8(signals.engine.lambda, signals.engine.lambdaUpdated,
                                  (int8_t)(v / 10));
                            break;
                        case 3:
                            // Basic-setting requirement bits: 1=condition met
                            setU8(signals.engine.basicSettingBits,
                                  signals.engine.basicSettingBitsUpdated, (uint8_t)(v / 10));
                            break;
                        default:
                            break;
                    }
                    break;
                case 3:
                    switch (idx)
                    {
                        case 1:
                            setU16(signals.engine.pressure, signals.engine.pressureUpdated,
                                   (uint16_t)(v / 10));
                            break;
                        case 2:
                            setI16(signals.engine.tbAngle, signals.engine.tbAngleUpdated,
                                   (int16_t)v);
                            break; // ×10
                        case 3:
                            setI16(signals.engine.steeringAngle,
                                   signals.engine.steeringAngleUpdated, (int16_t)v);
                            break; // ×10
                    }
                    break;
                case 4:
                    switch (idx)
                    {
                        case 1:
                            setU16(signals.engine.voltage, signals.engine.voltageUpdated,
                                   (uint16_t)v);
                            break; // ×10
                        case 2:
                            setU8(signals.engine.tempUnknown2, signals.engine.tempUnknown2Updated,
                                  (uint8_t)(v / 10));
                            break;
                        case 3:
                            setU8(signals.engine.tempUnknown3, signals.engine.tempUnknown3Updated,
                                  (uint8_t)(v / 10));
                            break;
                    }
                    break;
                case 6:
                    switch (idx)
                    {
                        case 1:
                            setU16(signals.engine.engineLoad, signals.engine.engineLoadUpdated,
                                   (uint16_t)(v / 10));
                            break;
                        case 3:
                            setI8(signals.engine.lambda2, signals.engine.lambda2Updated,
                                  (int8_t)(v / 10));
                            break;
                    }
                    break;
                case 100:
                    // OBD readiness bits: 1=not complete/FAIL, 0=complete/PASS
                    if (idx == 0)
                    {
                        // k=16 binary type: v = raw_byte×10, so raw_byte = v/10
                        uint8_t rb = (uint8_t)(v / 10);
                        signals.engine.exhaustGasRecirculationError = (rb >> 7) & 1;
                        signals.engine.oxygenSensorHeatingError = (rb >> 6) & 1;
                        signals.engine.oxygenSensorError = (rb >> 5) & 1;
                        signals.engine.airConditioningError = (rb >> 4) & 1;
                        signals.engine.secondaryAirInjectionError = (rb >> 3) & 1;
                        signals.engine.evaporativeEmissionsError = (rb >> 2) & 1;
                        signals.engine.catalystHeatingError = (rb >> 1) & 1;
                        signals.engine.catalyticConverter = (rb >> 0) & 1;
                        signals.engine.errorBitsUpdated = true;
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
