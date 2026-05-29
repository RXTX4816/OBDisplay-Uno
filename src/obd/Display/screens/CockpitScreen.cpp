// SPDX-License-Identifier: GPL-3.0-or-later
#include "CockpitScreen.h"
#include "../ScreenVM.h"

// Portrait layout (10 cols x 16 rows):
//
// ADDR_INSTRUMENTS (0x17), screen 0 — all 15 signals:
//   Row 0:  SPD:XXX     Row 7:  FUL:XX
//   Row 1:  RPM:XXXX    Row 8:  FSR:XXXXX
//   Row 2:  CLT:XXX     Row 9:  TM:XXXXXXX
//   Row 3:  OIL:XXX     Row 10: L100:X.X
//   Row 4:  AMB:XXX     Row 11: L/h:X.X
//   Row 5:  OL:X OP:X   Row 12: km:XXXXX
//   Row 6:  ODO:XXXXXX  Row 13: L:XX
//
// ADDR_ENGINE (0x01):
//   Screen 0: 8 signals stacked vertically
//   Screen 1: 4 signals + 8-bit error string

namespace obd
{
namespace Display
{

// clang-format off
static const uint8_t PROGMEM kCockpit17Script[] = {
    SO_LABEL,  0,  0, 4, 'S','P','D',':',   SO_U16,    4,  0, FLD_VEH_SPEED,
    SO_LABEL,  0,  1, 4, 'R','P','M',':',   SO_U16,    4,  1, FLD_ENG_RPM,
    SO_LABEL,  0,  2, 4, 'C','L','T',':',   SO_U8,     4,  2, FLD_COOLANT_T,
    SO_LABEL,  0,  3, 4, 'O','I','L',':',   SO_U8,     4,  3, FLD_OIL_T,
    SO_LABEL,  0,  4, 4, 'A','M','B',':',   SO_U8,     4,  4, FLD_AMB_T,
    // Row 5: OL:X OP:X — two bool fields on one row
    SO_LABEL,  0,  5, 3, 'O','L',':',       SO_U8,     3,  5, FLD_OIL_LVL,
    SO_LABEL,  5,  5, 3, 'O','P',':',       SO_U8,     8,  5, FLD_OIL_PRES,
    SO_LABEL,  0,  6, 4, 'O','D','O',':',   SO_U32,    4,  6, FLD_ODOMETER,
    SO_LABEL,  0,  7, 4, 'F','U','L',':',   SO_U8,     4,  7, FLD_FUEL_LVL,
    SO_LABEL,  0,  8, 4, 'F','S','R',':',   SO_U16,    4,  8, FLD_FUEL_RES,
    SO_LABEL,  0,  9, 3, 'T','M', ':',      SO_U32,    3,  9, FLD_TIME_ECU,
    SO_LABEL,  0, 10, 5, 'L','1','0','0',':', SO_SCALED, 5, 10, FLD_FUEL_100, 5,
    SO_LABEL,  0, 11, 4, 'L','/','h',':',   SO_SCALED, 4, 11, FLD_FUEL_H,   4,
    SO_LABEL,  0, 12, 3, 'k','m', ':',      SO_U16,    3, 12, FLD_ELAPSED_KM,
    SO_LABEL,  0, 13, 2, 'L', ':',          SO_U8,     2, 13, FLD_FUEL_BURNED,
    SO_END
};

static const uint8_t PROGMEM kCockpit01S0Script[] = {
    SO_LABEL,  0, 0, 4, 'R','P','M',':',   SO_U16,    4, 0, FLD_ENG_RPM,
    SO_LABEL,  0, 1, 2, 'V', ':',          SO_SCALED, 2, 1, FLD_VOLTAGE,  5,
    SO_LABEL,  0, 2, 3, 'T','1',':',       SO_U8,     3, 2, FLD_TEMP1,
    SO_LABEL,  0, 3, 3, 'T','2',':',       SO_U8,     3, 3, FLD_TEMP2,
    SO_LABEL,  0, 4, 3, 'T','3',':',       SO_U8,     3, 4, FLD_TEMP3,
    SO_LABEL,  0, 5, 4, 'L','A','M',':',   SO_I8,     4, 5, FLD_LAMBDA,
    SO_LABEL,  0, 6, 5, 'L','A','M','2',':', SO_I8,   5, 6, FLD_LAMBDA2,
    SO_LABEL,  0, 7, 3, 'L','D',':',       SO_U16,    3, 7, FLD_ENG_LOAD,
    SO_END
};

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

void renderCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected,
                         const Model::OBDSignals& signals, bool forceUpdate)
{
    using namespace Model;

    // Pre-assemble error bits string for screen 0x01/1 (const_cast is safe: bitsAsString
    // is a mutable cache field, not logically const).
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
    }

    ScreenCtx ctx{&signals, nullptr, 0, 0};
    const uint8_t* script = nullptr;

    if (addrSelected == 0x17)
    {
        script = kCockpit17Script;
    }
    else if (addrSelected == 0x01)
    {
        script = (screen == 0) ? kCockpit01S0Script : kCockpit01S1Script;
    }

    if (script)
    {
        runScript(script, ctx, dm);
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
