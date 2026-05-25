#include "CockpitScreen.h"
#include "ScreenHelpers.h"

// Landscape layout: 128x64, System5x7 font -> 21 cols x 8 rows.
// Max safe: col + width <= 21.
//
// ADDR_INSTRUMENTS (0x17) -- all 15 signals, screen 0 only:
//   Row 0: SPD:XXX    RPM:XXXX        col 0/4(3)  col 9/13(4)
//   Row 1: CLT:XXX    OIL:XXX         col 0/4(3)  col 9/13(3)
//   Row 2: AMB:XXX    OLV:X  OPR:X    col 0/4(3)  col 9/13(1)  col 15/19(1)
//   Row 3: ODO:XXXXXX                 col 0/4(6)
//   Row 4: FUL:XX     FSR:XXXXX       col 0/4(2)  col 9/13(5)
//   Row 5: TM:XXXXXXX                 col 0/3(7)
//   Row 6: L100:X.X   L/h:X.X         col 0/5(5f) col 11/15(4f)
//   Row 7: km:XXXXX   L:X.X           col 0/3(5)  col 9/11(5f)
//
// ADDR_ENGINE (0x01) -- screen 0: core signals, screen 1: extended:
//   Screen 0 row 0: RPM:XXXX           col 0/4(4)
//   Screen 0 row 1: V:X.X              col 0/2(5f)
//   Screen 0 row 2: T1:XXX T2:XXX T3:XXX  col 0/3(3) col 7/10(3) col 14/17(3)
//   Screen 0 row 3: LAM:XXX  LAM2:XXX  col 0/4(3)  col 9/14(3)
//   Screen 0 row 4: LOAD:XXX           col 0/5(3)
//   Screen 1 row 0: TBa:X.X            col 0/4(5f)
//   Screen 1 row 1: STa:X.X            col 0/4(5f)
//   Screen 1 row 2: mb:XXXX            col 0/3(4)
//   Screen 1 row 3: bits:
//   Screen 1 row 4: XXXXXXXX

namespace obd
{
namespace Display
{

void initCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected)
{
    switch (addrSelected)
    {
        case 0x17:
            dm.print(0,  0, F("SPD:"));
            dm.print(9,  0, F("RPM:"));
            dm.print(0,  1, F("CLT:"));
            dm.print(9,  1, F("OIL:"));
            dm.print(0,  2, F("AMB:"));
            dm.print(9,  2, F("OLV:"));
            dm.print(15, 2, F("OPR:"));
            dm.print(0,  3, F("ODO:"));
            dm.print(0,  4, F("FUL:"));
            dm.print(9,  4, F("FSR:"));
            dm.print(0,  5, F("TM:"));
            dm.print(0,  6, F("L100:"));
            dm.print(11, 6, F("L/h:"));
            dm.print(0,  7, F("km:"));
            dm.print(9,  7, F("L:"));
            break;
        case 0x01:
            switch (screen)
            {
                case 0:
                    dm.print(0,  0, F("RPM:"));
                    dm.print(0,  1, F("V:"));
                    dm.print(0,  2, F("T1:"));
                    dm.print(7,  2, F("T2:"));
                    dm.print(14, 2, F("T3:"));
                    dm.print(0,  3, F("LAM:"));
                    dm.print(9,  3, F("LAM2:"));
                    dm.print(0,  4, F("LOAD:"));
                    break;
                case 1:
                    dm.print(0, 0, F("TBa:"));
                    dm.print(0, 1, F("STa:"));
                    dm.print(0, 2, F("mb:"));
                    dm.print(0, 3, F("bits:"));
                    break;
                default:
                    dm.print(0, 0, F("Screen"));
                    dm.print(7, 0, String(screen));
                    dm.print(0, 1, F("no data"));
                    break;
            }
            break;
        default:
            dm.print(0, 0, F("Addr 0x"));
            dm.print(7, 0, String(addrSelected, HEX));
            dm.print(0, 1, F("no data"));
            break;
    }
}

void renderCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected,
                         const Model::OBDSignals& signals, bool forceUpdate)
{
    using namespace Model;

    switch (addrSelected)
    {
        case 0x17:
        {
            const InstrumentSignals& ins = signals.instruments;
            const ComputedStats& c = signals.computed;
            (void)screen;
            printField(dm, 4,  0, ins.vehicleSpeed,          3, const_cast<bool&>(ins.vehicleSpeedUpdated),         forceUpdate);
            printField(dm, 13, 0, ins.engineRpm,              4, const_cast<bool&>(ins.engineRpmUpdated),            forceUpdate);
            printField(dm, 4,  1, ins.coolantTemp,            3, const_cast<bool&>(ins.coolantTempUpdated),          forceUpdate);
            printField(dm, 13, 1, ins.oilTemp,                3, const_cast<bool&>(ins.oilTempUpdated),              forceUpdate);
            printField(dm, 4,  2, ins.ambientTemp,            3, const_cast<bool&>(ins.ambientTempUpdated),          forceUpdate);
            printField(dm, 13, 2, ins.oilLevelOk,             1, const_cast<bool&>(ins.oilLevelOkUpdated),           forceUpdate);
            printField(dm, 19, 2, ins.oilPressureMin,         1, const_cast<bool&>(ins.oilPressureMinUpdated),       forceUpdate);
            printField(dm, 4,  3, ins.odometer,               6, const_cast<bool&>(ins.odometerUpdated),             forceUpdate);
            printField(dm, 4,  4, ins.fuelLevel,              2, const_cast<bool&>(ins.fuelLevelUpdated),            forceUpdate);
            printField(dm, 13, 4, ins.fuelSensorResistance,   5, const_cast<bool&>(ins.fuelSensorResistanceUpdated), forceUpdate);
            printField(dm, 3,  5, ins.timeEcu,                7, const_cast<bool&>(ins.timeEcuUpdated),              forceUpdate);
            printFieldFloat(dm, 5,  6, c.fuelPer100km,        5, const_cast<bool&>(c.fuelPer100kmUpdated),           forceUpdate);
            printFieldFloat(dm, 15, 6, c.fuelPerHour,         4, const_cast<bool&>(c.fuelPerHourUpdated),            forceUpdate);
            printField(dm, 3,  7, c.elapsedKmSinceStart,      5, const_cast<bool&>(c.elapsedKmSinceStartUpdated),    forceUpdate);
            printFieldFloat(dm, 11, 7, c.fuelBurnedSinceStart,5, const_cast<bool&>(c.fuelBurnedSinceStartUpdated),   forceUpdate);
            break;
        }
        case 0x01:
        {
            const EngineSignals& e = signals.engine;
            const InstrumentSignals& ins = signals.instruments;
            switch (screen)
            {
                case 0:
                    printField(dm, 4,  0, ins.engineRpm,    4, const_cast<bool&>(ins.engineRpmUpdated),    forceUpdate);
                    printFieldFloat(dm, 2, 1, e.voltage,    5, const_cast<bool&>(e.voltageUpdated),         forceUpdate);
                    printField(dm, 3,  2, e.tempUnknown1,   3, const_cast<bool&>(e.tempUnknown1Updated),   forceUpdate);
                    printField(dm, 10, 2, e.tempUnknown2,   3, const_cast<bool&>(e.tempUnknown2Updated),   forceUpdate);
                    printField(dm, 17, 2, e.tempUnknown3,   3, const_cast<bool&>(e.tempUnknown3Updated),   forceUpdate);
                    printField(dm, 4,  3, e.lambda,         3, const_cast<bool&>(e.lambdaUpdated),         forceUpdate);
                    printField(dm, 14, 3, e.lambda2,        3, const_cast<bool&>(e.lambda2Updated),        forceUpdate);
                    printField(dm, 5,  4, e.engineLoad,     3, const_cast<bool&>(e.engineLoadUpdated),     forceUpdate);
                    break;
                case 1:
                {
                    printFieldFloat(dm, 4, 0, e.tbAngle,       5, const_cast<bool&>(e.tbAngleUpdated),      forceUpdate);
                    printFieldFloat(dm, 4, 1, e.steeringAngle, 5, const_cast<bool&>(e.steeringAngleUpdated),forceUpdate);
                    printField(dm, 3, 2, e.pressure,           4, const_cast<bool&>(e.pressureUpdated),     forceUpdate);
                    EngineSignals& em = const_cast<EngineSignals&>(e);
                    if (em.errorBitsUpdated || forceUpdate)
                    {
                        em.bitsAsString[0] = em.exhaustGasRecirculationError   ? '1' : '0';
                        em.bitsAsString[1] = em.oxygenSensorHeatingError       ? '1' : '0';
                        em.bitsAsString[2] = em.oxygenSensorError              ? '1' : '0';
                        em.bitsAsString[3] = em.airConditioningError           ? '1' : '0';
                        em.bitsAsString[4] = em.secondaryAirInjectionError     ? '1' : '0';
                        em.bitsAsString[5] = em.evaporativeEmissionsError      ? '1' : '0';
                        em.bitsAsString[6] = em.catalystHeatingError           ? '1' : '0';
                        em.bitsAsString[7] = em.catalyticConverter             ? '1' : '0';
                        em.bitsAsString[8] = '\0';
                    }
                    printFieldStr(dm, 0, 4, em.bitsAsString, 8, const_cast<bool&>(em.errorBitsUpdated), forceUpdate);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        default:
            break;
    }
}

} // namespace Display
} // namespace obd
