// SPDX-License-Identifier: GPL-3.0-or-later
#include "CockpitScreen.h"
#include "../ScreenVM.h"
#include "../../../Config.h"
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
// ADDR_INSTRUMENTS (0x17), page 1 — second dashboard, 7 rows (112 px):
//   y=  0   speed       e.g. "130"
//   y= 16   oil temp    e.g. "95 O"  / "-WARN-" if >=100
//   y= 32   coolant     e.g. "88 C"  / "-WARN-" if >=100
//   y= 48   km remain   e.g. "450K"  / "---" if no data
//   y= 64   L/100km     e.g. "8.3L"
//   y= 80   fuel level  e.g. "33 F"
//   y= 96   oil level   e.g. "50 %"  (0–255 → 0–100%)
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

// Combined warning names: bytes [0..4] = word1, [5..9] = word2, both null-padded.
// Single array lets both renderWarningFlash and renderWarningSummaryPage use one
// memcpy_P per entry instead of two.
// clang-format off
static const char PROGMEM kWarnNames[WARN_COUNT][10] = {
    {'O','I','L', 0,  0,  'P','R','E','S', 0 }, // 0  OIL  PRES  HIGH
    {'O','I','L', 0,  0,  'H','O','T', 0,  0 }, // 1  OIL  HOT   HIGH
    {'C','O','O','L', 0,  'H','O','T', 0,  0 }, // 2  COOL HOT   HIGH
    {'O','I','L', 0,  0,  'L','V','L', 0,  0 }, // 3  OIL  LVL   HIGH (<20%)
    {'L','O','W', 0,  0,  'V','O','L','T', 0 }, // 4  LOW  VOLT  MED
    {'F','U','E','L', 0,  'C','R','I','T', 0 }, // 5  FUEL CRIT  MED
    {'V','E','R','Y', 0,  'C','O','L','D', 0 }, // 6  VERY COLD  MED
    {'H','I','G','H', 0,  'L','O','A','D', 0 }, // 7  HIGH LOAD  LOW
    {'F','U','E','L', 0,  'L','O','W', 0,  0 }, // 8  FUEL LOW   LOW
    {'C','O','L','D', 0,  'E','N','G', 0,  0 }, // 9  COLD ENG   LOW
    {'O','I','L', 0,  0,  'L','V','L', 0,  0 }, // 10 OIL  LVL   LOW (<45%)
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
    dm.printBigWithLabel(0, 78, s.instruments.fuelLevelSmoothX8 >> 3, " L");
    dm.printBigWithLabel(0, 94, s.instruments.ambientTemp, "AIR");
}

// ── 0x01 page 0: big dashboard (fixed to use actual 0x01 fields) ─────────────
// Row layout (128 px):  speed RPM coolant load tbAngle voltage lambda intakeAir
static void renderCockpit01Big(const DisplayManager& dm, const OBDSignals& s)
{
    dm.printBig(0, 0, s.instruments.vehicleSpeed);            // group 5 idx 2 [fixed]
    dm.printBig(0, 16, s.instruments.engineRpm);              // group 1 idx 0
    printBigTemp(dm, 0, 32, s.engine.tempUnknown2, " C");     // group 4 idx 2: coolant
    dm.printBig(0, 48, (int16_t)s.engine.engineLoad, '%');    // group 5/6 idx 1 [fixed]
    dm.printBigScaled10(0, 64, s.engine.tbAngle, 'T');        // group 3 idx 2
    dm.printBigVoltage(0, 80, s.engine.voltage);              // group 4 idx 1 [fixed]
    dm.printBig(0, 96, (int16_t)s.engine.lambda, '%');        // group 1 idx 2
    dm.printBig(0, 112, (int16_t)s.engine.tempUnknown3, 'I'); // group 4 idx 3: intake air
}

// Shared helper for both bit-field screens (readiness + basic-setting).
// labelTable: PROGMEM array, labelWidth bytes per entry (no null needed).
// xVal: column for the pass/fail indicator. errorBits: 1=FAIL/0=PASS vs 1=Y/0=N.
static void renderBitField(const DisplayManager& dm, const void* labelTable, uint8_t labelWidth,
                           uint8_t xVal, uint8_t bits, bool errorBits)
{
    char label[9];
    for (uint8_t i = 0; i < 8; ++i)
    {
        memcpy_P(label, (PGM_P)labelTable + (uint16_t)i * (labelWidth + 1u), labelWidth);
        label[labelWidth] = '\0';
        dm.print(0, i, label);
        bool bit = (bits >> (7 - i)) & 1;
        dm.print(xVal, i, errorBits ? (bit ? F("FAIL") : F("PASS")) : (bit ? F("Y") : F("N")));
    }
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
    const uint8_t bits =
        ((uint8_t)s.engine.exhaustGasRecirculationError << 7) |
        ((uint8_t)s.engine.oxygenSensorHeatingError << 6) |
        ((uint8_t)s.engine.oxygenSensorError << 5) | ((uint8_t)s.engine.airConditioningError << 4) |
        ((uint8_t)s.engine.secondaryAirInjectionError << 3) |
        ((uint8_t)s.engine.evaporativeEmissionsError << 2) |
        ((uint8_t)s.engine.catalystHeatingError << 1) | ((uint8_t)s.engine.catalyticConverter << 0);
    renderBitField(dm, kReadinessLabels, 5, 6, bits, true);
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
    renderBitField(dm, kBasicSettingLabels, 8, 9, s.engine.basicSettingBits, false);
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
    dm.printBigWithLabel(0, 80, s.instruments.fuelLevelSmoothX8 >> 3, " F");
    dm.printBigWithLabel(0, 96, ((uint16_t)s.instruments.oilLevelOk * 100u) / 255u, " %");
}

static void drawBarGauge(const DisplayManager& dm, uint8_t barX, uint8_t val, uint8_t maxVal,
                         uint8_t tickY, const char* label);

// ── 0x01 page 4: trip computer ───────────────────────────────────────────────
// kmRemaining is non-zero only if fuel level was stored via 0x17 Settings→Fuel.
static void renderTripPage01(const DisplayManager& dm, const OBDSignals& s)
{
    // L/100km ×10 → display as X.X
    if (s.computed.fuelPer100km > 0)
        dm.printBigScaled10(0, 0, (int16_t)s.computed.fuelPer100km, 'L');
    else
        dm.printBig(0, 0, "---L");

    // L/hr ×10 → display as X.X
    if (s.computed.fuelPerHour > 0)
        dm.printBigScaled10(0, 16, (int16_t)s.computed.fuelPerHour, 'H');
    else
        dm.printBig(0, 16, "---H");

    // km remaining (0 = no fuel start set)
    if (s.computed.kmRemaining > 0)
        dm.printBigWithLabel(0, 32, s.computed.kmRemaining, "K");
    else
        dm.printBig(0, 32, "---K");

    // L burned this session
    dm.printBigWithLabel(0, 48, s.computed.fuelBurnedSinceStart, " B");
}

// ── 0x01 page 5: 4-bar gauges ─────────────────────────────────────────────────
// Bars: coolant °C | engine load % | lambda % (offset +15) | voltage ×10
// Lambda bar: maps -15..+15% → 0..30 range, tick at centre (0%)
static void renderBarsPage01(const DisplayManager& dm, const OBDSignals& s)
{
    // coolant: group 4, 0–120 °C, tick at 90 °C → fillH=84, tickY=28
    drawBarGauge(dm, 2, s.engine.tempUnknown2, 120, 28, "C");
    // engine load: 0–100 %, tick at 80 % → tickY = 112 - (112*80/100) = 22
    drawBarGauge(dm, 18, (uint8_t)s.engine.engineLoad, 100, 22, "L");
    // lambda: -15 to +15 % mapped to 0–30, tick at 0 % (centre) → tickY=56
    int8_t lam = s.engine.lambda;
    uint8_t lamBar = (lam < -15) ? 0u : (lam > 15) ? 30u : (uint8_t)(lam + 15);
    drawBarGauge(dm, 34, lamBar, 30, 56, "%");
    // voltage ×10: range 100–160 (10.0–16.0V), tick at 120 (12.0V)
    // scale: val-100, max=60; tick: 120-100=20 → tickY = 112 - 112*20/60 = 112-37 = 75
    uint16_t vraw = s.engine.voltage;
    uint8_t vbar = (vraw < 100u) ? 0u : (vraw > 160u) ? 60u : (uint8_t)(vraw - 100u);
    drawBarGauge(dm, 50, vbar, 60, 75, "V");
}

// ── 0x17 page 2: 4-bar gauge ─────────────────────────────────────────────────
//
// Layout (64×128 px portrait):
//   4 zones × 16 px each.  Bar: 12 px wide, 2 px left margin in zone.
//   Bar area: y = 0–111 (112 px), fills from bottom.
//   Tick line: 2 px tall at midpoint.  Label (big font): y = 114.
//
//   Bar 0 — coolant °C  x= 2  range 0–120  tick@90 → y=28
//   Bar 1 — oil °C      x=18  range 0–120  tick@90 → y=28
//   Bar 2 — oil level % x=34  range 0–100  tick@50 → y=56
//   Bar 3 — fuel L      x=50  range 0–FUEL_TANK_MAX_LITERS  tick@50% → y=56
static void drawBarGauge(const DisplayManager& dm, uint8_t barX, uint8_t val, uint8_t maxVal,
                         uint8_t tickY, const char* label)
{
    static constexpr uint8_t kBarAreaH = 112;
    static constexpr uint8_t kBarW = 12;
    static constexpr uint8_t kLabelY = 114;

    if (val > maxVal)
        val = maxVal;
    uint8_t fillH = (uint8_t)((uint16_t)val * kBarAreaH / maxVal);
    uint8_t fillTop = kBarAreaH - fillH;

    if (fillH > 0)
        dm.drawBar(barX, fillTop, kBarW, fillH);

    // Tick: clear notch when inside fill, filled line when in empty area.
    if (fillTop <= tickY)
        dm.drawBarClear(barX, tickY, kBarW, 2);
    else
        dm.drawBar(barX, tickY, kBarW, 2);

    dm.printBig(barX, kLabelY, label);
}

static void renderBarsPage17(const DisplayManager& dm, const OBDSignals& s)
{
    // coolant: 0–120 °C, tick at 90 °C (fillH=84 → tickY=28)
    drawBarGauge(dm, 2, s.instruments.coolantTemp, 120, 28, "C");
    // oil temp: 0–120 °C, tick at 90 °C
    drawBarGauge(dm, 18, s.instruments.oilTemp, 120, 28, "O");
    // oil level: raw 0–255 mapped to 0–100 %, tick at 50 % (tickY=56)
    uint8_t oilPct = (uint8_t)(((uint16_t)s.instruments.oilLevelOk * 100u) / 255u);
    drawBarGauge(dm, 34, oilPct, 100, 56, "L");
    // fuel: smoothed litres 0–FUEL_TANK_MAX_LITERS, tick at 50 % (tickY=56)
    uint16_t smoothL = s.instruments.fuelLevelSmoothX8 >> 3u;
    uint8_t fuelL = smoothL > FUEL_TANK_MAX_LITERS ? FUEL_TANK_MAX_LITERS : (uint8_t)smoothL;
    drawBarGauge(dm, 50, fuelL, FUEL_TANK_MAX_LITERS, 56, "F");
}

// ── Warning summary page (last cockpit page on both ECUs) ─────────────────────
// Small font (6×8 px); lists every active warning as "WORD1 WORD2", or "ALL OK".
static void renderWarningSummaryPage(const DisplayManager& dm, const WarningState& w)
{
    dm.print(0, 0, F("WARN"));
    if (w.bits == 0)
    {
        dm.print(0, 1, F("ALL OK"));
        return;
    }
    uint8_t row = 1;
    char words[11];
    words[10] = '\0';
    for (uint8_t i = 0; i < WARN_COUNT; ++i)
    {
        if (!(w.bits & ((uint16_t)1u << i)))
            continue;
        memcpy_P(words, kWarnNames[i], 10);
        dm.print(0, row, words);
        dm.print(5, row, words + 5);
        ++row;
    }
}

// ── Warning flash overlay (shown when a new warning fires) ────────────────────
// Layout (64×128 px portrait):
//   y=  0  severity label in big font, centered  ("CRIT" / "CAUT" / "ALRT")
//   y= 48  word 1 of warning name, centered
//   y= 64  word 2 of warning name, centered
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

    // Find the flashPage-th active warning (wrapping); single pass with decrement.
    uint8_t cnt = 0;
    for (uint8_t i = 0; i < WARN_COUNT; ++i)
        cnt += (uint8_t)((w.bits >> i) & 1u);

    uint8_t rem = flashPage % cnt;
    uint8_t warnIdx = 0;
    for (uint8_t i = 0; i < WARN_COUNT; ++i)
    {
        if (!(w.bits & ((uint16_t)1u << i)))
            continue;
        if (rem-- == 0)
        {
            warnIdx = i;
            break;
        }
    }

    // Each word is centered: x = (64 - len*12) / 2  (12 px per char at 2× font)
    char word[6];
    word[5] = '\0';
    memcpy_P(word, kWarnNames[warnIdx], 5);
    dm.printBig((64u - (uint8_t)(strlen(word) * 12u)) >> 1u, 48, word);
    memcpy_P(word, kWarnNames[warnIdx] + 5, 5);
    dm.printBig((64u - (uint8_t)(strlen(word) * 12u)) >> 1u, 64, word);
}

// ── Main dispatch ─────────────────────────────────────────────────────────────
void renderCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected,
                         const OBDSignals& signals, bool forceUpdate)
{
    if (addrSelected == 0x01)
    {
        ScreenCtx ctx{&signals, nullptr, 0, 0};
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
                runScript(kCockpit01S3Script, ctx, dm);
                break;
            case 4:
                renderTripPage01(dm, signals);
                break;
            case 5:
                renderBarsPage01(dm, signals);
                break;
            case 6:
                renderWarningSummaryPage(dm, signals.warnings);
                break;
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
            case 2:
                renderBarsPage17(dm, signals);
                break;
            case 3:
                renderWarningSummaryPage(dm, signals.warnings);
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
