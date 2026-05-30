// SPDX-License-Identifier: GPL-3.0-or-later
#include "CockpitScreen.h"
#include "../ScreenVM.h"

// Portrait layout (64×128 px), 2× pixel-doubled font (12 px/char, 14 px/row).
// Row step = 16 px (14 px glyph + 2 px gap between rows).
//
// ADDR_INSTRUMENTS (0x17), screen 0 — 6 rows (108 px used of 128):
//   y=  0   speed (km/h)          e.g. "130"
//   y= 16   RPM                   e.g. "2200"
//   [+7 px gap]
//   y= 39   oil temperature        e.g. "99 O"  / "-WARN-" if >=100
//   y= 55   coolant temperature    e.g. "99 C"  / "-WARN-" if >=100
//   [+7 px gap]
//   y= 78   fuel level (L)         e.g. "33 L"
//   y= 94   ambient temperature    e.g. "20AIR"
//
// ADDR_ENGINE (0x01), screen 0 — 8 rows (exactly 126 px of 128):
//   y=  0   speed (km/h)          e.g. "120"
//   y= 16   RPM                   e.g. "1200"
//   y= 32   oil temperature        e.g. "99 O"  / "-WARN-" if >=100
//   y= 48   coolant temperature    e.g. "99 C"  / "-WARN-" if >=100
//   y= 64   engine load (%)        e.g. "20%"
//   y= 80   throttle body angle    e.g. "5.5T"
//   y= 96   battery voltage        e.g. "12V"
//   y=112   lambda (%)             e.g. "5%"
//
// ADDR_ENGINE (0x01), screen 1 — error bits (small font, unchanged).

namespace obd
{
namespace Display
{

// clang-format off
static const uint8_t PROGMEM kCockpit01S1Script[] = {
    SO_LABEL,   0, 0, 4, 'T','B','a',':',  SO_SCALED, 4, 0, FLD_TB_ANGLE,    5,
    SO_LABEL,   0, 1, 4, 'S','T','a',':',  SO_SCALED, 4, 1, FLD_STEER_ANGLE, 5,
    SO_LABEL,   0, 2, 3, 'm','b', ':',     SO_U16,    3, 2, FLD_PRESSURE,
    SO_LABEL,   0, 3, 5, 'b','i','t','s',':',
    SO_STR,     0, 4, FLD_ERR_BITS_STR,
    SO_END
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

static void renderCockpit17Big(const DisplayManager& dm, const Model::OBDSignals& s)
{
    dm.printBig(0, 0, s.instruments.vehicleSpeed);
    dm.printBig(0, 16, s.instruments.engineRpm);
    printBigTemp(dm, 0, 39, s.instruments.oilTemp, " O");
    printBigTemp(dm, 0, 55, s.instruments.coolantTemp, " C");
    dm.printBigWithLabel(0, 78, s.instruments.fuelLevel, " L");
    dm.printBigWithLabel(0, 94, s.instruments.ambientTemp, "AIR");
}

static void renderCockpit01Big(const DisplayManager& dm, const Model::OBDSignals& s)
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

void renderCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected,
                         const Model::OBDSignals& signals, bool forceUpdate)
{
    using namespace Model;

    if (addrSelected == 0x01 && screen == 1)
    {
        if (signals.engine.errorBitsUpdated || forceUpdate)
        {
            EngineSignals& em = const_cast<EngineSignals&>(signals.engine);
            em.bitsAsString[0] = em.exhaustGasRecirculationError ? '1' : '0';
            em.bitsAsString[1] = em.oxygenSensorHeatingError ? '1' : '0';
            em.bitsAsString[2] = em.oxygenSensorError ? '1' : '0';
            em.bitsAsString[3] = em.airConditioningError ? '1' : '0';
            em.bitsAsString[4] = em.secondaryAirInjectionError ? '1' : '0';
            em.bitsAsString[5] = em.evaporativeEmissionsError ? '1' : '0';
            em.bitsAsString[6] = em.catalystHeatingError ? '1' : '0';
            em.bitsAsString[7] = em.catalyticConverter ? '1' : '0';
            em.bitsAsString[8] = '\0';
        }
        ScreenCtx ctx{&signals, nullptr, 0, 0};
        runScript(kCockpit01S1Script, ctx, dm);
        return;
    }

    if (addrSelected == 0x17)
    {
        renderCockpit17Big(dm, signals);
    }
    else if (addrSelected == 0x01)
    {
        renderCockpit01Big(dm, signals);
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
