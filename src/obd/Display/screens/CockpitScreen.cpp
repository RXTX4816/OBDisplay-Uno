// SPDX-License-Identifier: GPL-3.0-or-later
#include "CockpitScreen.h"
#include "../ScreenVM.h"
#include <avr/pgmspace.h>
#include <string.h>

// Portrait layout (64×128 px), 2× pixel-doubled font (12 px/char, 14 px/row).
// Row step = 16 px (14 px glyph + 2 px gap between rows).
//
// ADDR_INSTRUMENTS (0x17), page 0 — 6 rows (108 px used of 128):
//   y=  0   speed (km/h)          e.g. "130"
//   y= 16   RPM                   e.g. "2200"
//   [+7 px gap]
//   y= 39   oil temperature        e.g. "99 O"  / "-WARN-" if >=100
//   y= 55   coolant temperature    e.g. "99 C"  / "-WARN-" if >=100
//   [+7 px gap]
//   y= 78   fuel level (L)         e.g. "33 L"
//   y= 94   ambient temperature    e.g. "20AIR"
//
// ADDR_INSTRUMENTS (0x17), page 1 — second dashboard, 6 rows (96 px):
//   y=  0   speed       e.g. "130"
//   y= 16   oil temp    e.g. "95 O"  / "-WARN-" if >=100
//   y= 32   coolant     e.g. "88 C"  / "-WARN-" if >=100
//   y= 48   km remain   e.g. "450K"  / "---" if no data
//   y= 64   L/100km     e.g. "8.3L"
//   y= 80   fuel level  e.g. "33 F"
//
// ADDR_ENGINE (0x01), page 0 — 8 rows (exactly 126 px of 128):
//   y=  0   speed         e.g. "120"
//   y= 16   RPM           e.g. "1200"
//   y= 32   oil temp      e.g. "99 O"  / "-WARN-" if >=100
//   y= 48   coolant temp  e.g. "99 C"  / "-WARN-" if >=100
//   y= 64   engine load   e.g. "20%"
//   y= 80   throttle body e.g. "5.5T"
//   y= 96   battery volt  e.g. "12V"
//   y=112   lambda (%)    e.g. "5%"
//
// ADDR_ENGINE (0x01), page 1 — OBD readiness bits (small font).
// ADDR_ENGINE (0x01), page 2 — basic-setting status bits (small font).
// ADDR_ENGINE (0x01), page 3 — engine diagnostics ScreenVM script (small font).

namespace obd
{
namespace Display
{

using namespace Model;

// clang-format off

// 0x01 page 3 — engine diagnostics
static const uint8_t PROGMEM kCockpit01S3Script[] = {
    SO_LABEL, 0, 0, 2, 'V', ':',             SO_SCALED, 2, 0, FLD_VOLTAGE,   5,
    SO_LABEL, 0, 1, 3, 'C', 'o', ':',        SO_U8,     4, 1, FLD_TEMP2,
    SO_LABEL, 0, 2, 3, 'I', 'A', ':',        SO_U8,     4, 2, FLD_TEMP3,
    SO_LABEL, 0, 3, 3, 'L', 'd', ':',        SO_U16,    4, 3, FLD_ENG_LOAD,
    SO_LABEL, 0, 4, 3, 'm', 'b', ':',        SO_U16,    3, 4, FLD_PRESSURE,
    SO_LABEL, 0, 5, 3, 'L', '1', ':',        SO_I8,     3, 5, FLD_LAMBDA,
    SO_LABEL, 0, 6, 3, 'L', '2', ':',        SO_I8,     3, 6, FLD_LAMBDA2,
    SO_END
};

// Readiness screen labels — 5 chars each (bit 7→0)
static const char PROGMEM kReadinessLabels[8][6] = {
    "EGR  ", "O2Htr", "O2Sns", "A/C  ", "2Air ", "Evap ", "CtHtr", "Cat  "
};

// Basic-setting status labels — 8 chars each (bit 7→0)
static const char PROGMEM kBasicSettingLabels[8][9] = {
    "CoolWarm", "RPM<2000", "TBclosed", "LambdaOK",
    "Idle    ", "A/Coff  ", "Cat>300 ", "NoFault "
};

// Big-font severity labels for the warning flash overlay (4 chars each, index = maxLevel-1).
static const char PROGMEM kSevFlash[3][5] = {"ALRT", "CAUT", "CRIT"};

// Pre-formatted warning lines: 4-char severity prefix + 6-char name = 10 chars each.
// Bit order is HIGH→MED→LOW so iterating forward prints most-severe first.
static const char PROGMEM kWarnLines[WARN_COUNT][10] = {
    "!!! OIL PR", // 0 WARN_OIL_PRES  HIGH
    "!!! OIL HT", // 1 WARN_OIL_HOT   HIGH
    "!!! COOLNT", // 2 WARN_COOL_HOT  HIGH
    "!!  OIL LV", // 3 WARN_OIL_LVL   MED
    "!!  LO VLT", // 4 WARN_LOW_VOLT  MED
    "!!  FL CRT", // 5 WARN_FUEL_CRIT MED
    "!!  V COLD", // 6 WARN_VERY_COLD MED
    "!   HILOAD", // 7 WARN_HIGH_LOAD LOW
    "!   FL LOW", // 8 WARN_FUEL_LOW  LOW
    "!   CLD EG", // 9 WARN_COLD_ENG  LOW
};

// clang-format on

void initCockpitScreen(DisplayManager& /*dm*/, uint8_t /*screen*/, uint8_t /*addrSelected*/) {}

// Temperatures only have room for 2 digits + 3-char label at 2x font.
// If the value reaches 100+ the label would overflow the screen, so
// warn the driver instead — a 3-digit temp is dangerous anyway.
static void printBigTemp(const DisplayManager& dm, uint8_t x, uint8_t y, uint8_t val,
                         const char* label)
{
    if (val >= 100)
        dm.printBig(x, y, "-WARN-");
    else
        dm.printBigWithLabel(x, y, val, label);
}

// ── 0x17 page 0: existing big dashboard ──────────────────────────────────────
static void renderCockpit17Big(const DisplayManager& dm, const OBDSignals& s)
{
    dm.printBig(0, 0, s.instruments.vehicleSpeed);
    dm.printBig(0, 16, s.instruments.engineRpm);
    printBigTemp(dm, 0, 39, s.instruments.oilTemp, " O");
    printBigTemp(dm, 0, 55, s.instruments.coolantTemp, " C");
    dm.printBigWithLabel(0, 78, s.instruments.fuelLevel, " L");
    dm.printBigWithLabel(0, 94, s.instruments.ambientTemp, "AIR");
}

// ── 0x01 page 0: existing big dashboard ──────────────────────────────────────
static void renderCockpit01Big(const DisplayManager& dm, const OBDSignals& s)
{
    dm.printBig(0, 0, s.instruments.vehicleSpeed);
    dm.printBig(0, 16, s.instruments.engineRpm);
    printBigTemp(dm, 0, 32, s.instruments.oilTemp, " O");
    printBigTemp(dm, 0, 48, s.instruments.coolantTemp, " C");
    dm.printBig(0, 64, s.engine.engineLoad, '%');
    dm.printBigScaled10(0, 80, s.engine.tbAngle, 'T');
    dm.printBigVoltage(0, 96, s.engine.voltage);
    dm.printBig(0, 112, (int16_t)s.engine.lambda, '%');
}

// ── 0x01 page 1: OBD readiness bits ─────────────────────────────────────────
static void renderReadinessScreen(const DisplayManager& dm, const OBDSignals& s)
{
    if (!s.engine.errorBitsUpdated)
    {
        dm.print(0, 0, F("Readiness"));
        dm.print(0, 1, F("not read"));
        return;
    }
    // Bits in group 100 value 1: 1=FAIL, 0=PASS
    const bool bits[8] = {
        s.engine.exhaustGasRecirculationError,
        s.engine.oxygenSensorHeatingError,
        s.engine.oxygenSensorError,
        s.engine.airConditioningError,
        s.engine.secondaryAirInjectionError,
        s.engine.evaporativeEmissionsError,
        s.engine.catalystHeatingError,
        s.engine.catalyticConverter,
    };
    for (uint8_t i = 0; i < 8; ++i)
    {
        char label[6];
        memcpy_P(label, kReadinessLabels[i], 5);
        label[5] = '\0';
        dm.print(0, i, label);
        dm.print(6, i, bits[i] ? F("FAIL") : F("PASS"));
    }
}

// ── 0x01 page 2: basic-setting requirement bits ───────────────────────────────
static void renderBasicSettingScreen(const DisplayManager& dm, const OBDSignals& s)
{
    if (!s.engine.basicSettingBitsUpdated)
    {
        dm.print(0, 0, F("BasicSet"));
        dm.print(0, 1, F("not read"));
        return;
    }
    uint8_t b = s.engine.basicSettingBits;
    for (uint8_t i = 0; i < 8; ++i)
    {
        char label[9];
        memcpy_P(label, kBasicSettingLabels[i], 8);
        label[8] = '\0';
        dm.print(0, i, label);
        // Bit 7 is first label, bit 0 is last
        dm.print(9, i, (b & (0x80u >> i)) ? F("Y") : F("N"));
    }
}

// ── 0x17 page 1: second big dashboard ────────────────────────────────────────
static void renderSecondDashboard17(const DisplayManager& dm, const OBDSignals& s)
{
    dm.printBig(0, 0, s.instruments.vehicleSpeed);
    printBigTemp(dm, 0, 16, s.instruments.oilTemp, " O");
    printBigTemp(dm, 0, 32, s.instruments.coolantTemp, " C");

    if (s.computed.fuelPer100km > 0)
        dm.printBigWithLabel(0, 48, s.computed.kmRemaining, "K");
    else
        dm.printBig(0, 48, "---");

    dm.printBigScaled10(0, 64, (int16_t)s.computed.fuelPer100km, 'L');
    dm.printBigWithLabel(0, 80, s.instruments.fuelLevel, " F");
}

// ── Warning flash overlay (shown when a new warning fires) ────────────────────
// Layout (64×128 px portrait):
//   y=  0  severity label in big font, centered  ("CRIT" / "CAUT" / "ALRT")
//   y= 48  first 5 chars of kWarnLines entry, centered
//   y= 64  last  5 chars of kWarnLines entry, centered
//   y=112  severity label again at the very bottom
// flashPage selects which active warning to show; caller increments each "on" frame.
void renderWarningFlash(const DisplayManager& dm, const WarningState& w, uint8_t flashPage)
{
    if (w.bits == 0)
        return;

    // Severity label — 4 chars × 12 px = 48 px; center at x = (64-48)/2 = 8
    char sev[5];
    sev[4] = '\0';
    uint8_t sevIdx = (w.maxLevel >= 3) ? 2u : (w.maxLevel == 2) ? 1u : 0u;
    memcpy_P(sev, kSevFlash[sevIdx], 4);
    dm.printBig(8, 0, sev);
    dm.printBig(8, 112, sev);

    // Find the flashPage-th active warning (wrapping)
    uint8_t activeCount = 0;
    for (uint8_t i = 0; i < WARN_COUNT; ++i)
        if (w.bits & ((uint16_t)1u << i))
            ++activeCount;

    uint8_t target = flashPage % activeCount;
    uint8_t found = 0;
    uint8_t warnIdx = 0;
    for (uint8_t i = 0; i < WARN_COUNT; ++i)
    {
        if (!(w.bits & ((uint16_t)1u << i)))
            continue;
        if (found == target)
        {
            warnIdx = i;
            break;
        }
        ++found;
    }

    // kWarnLines entry is 10 chars; split into 2 rows of 5.
    // 5 chars × 12 px = 60 px; center at x = (64-60)/2 = 2
    // 2 rows × 16 px = 32 px; vertical center: (128-32)/2 = 48
    char half[6];
    half[5] = '\0';
    memcpy_P(half, kWarnLines[warnIdx], 5);
    dm.printBig(2, 48, half);
    memcpy_P(half, kWarnLines[warnIdx] + 5, 5);
    dm.printBig(2, 64, half);
}

// ── Main dispatch ─────────────────────────────────────────────────────────────
void renderCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected,
                         const OBDSignals& signals, bool forceUpdate)
{
    if (addrSelected == 0x01)
    {
        switch (screen)
        {
            case 0:
                renderCockpit01Big(dm, signals);
                break;
            case 1:
                renderReadinessScreen(dm, signals);
                break;
            case 2:
                renderBasicSettingScreen(dm, signals);
                break;
            case 3:
            {
                ScreenCtx ctx{&signals, nullptr, 0, 0};
                runScript(kCockpit01S3Script, ctx, dm);
                break;
            }
            default:
                renderCockpit01Big(dm, signals);
                break;
        }
    }
    else if (addrSelected == 0x17)
    {
        switch (screen)
        {
            case 0:
                renderCockpit17Big(dm, signals);
                break;
            case 1:
                renderSecondDashboard17(dm, signals);
                break;
            default:
                renderCockpit17Big(dm, signals);
                break;
        }
    }
    else
    {
        char buf[4];
        dm.print(0, 0, F("Addr 0x"));
        ltoa((long)addrSelected, buf, 16);
        dm.print(7, 0, buf);
        dm.print(0, 1, F("no data"));
    }

    (void)forceUpdate;
}

} // namespace Display
} // namespace obd
