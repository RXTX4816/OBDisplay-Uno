#include "CockpitScreen.h"
#include "ScreenHelpers.h"

// OLED layout: 128x64, System5x7 font (6px wide), 20 cols x 8 rows.
//
// ADDR_INSTRUMENTS (0x17) — all data on one screen:
//   Row 0: SPD:xxx  RPM:xxxx
//   Row 1: CLT:xxx  OIL:xxx
//   Row 2: OLV:x OPR:x AMB:xx
//   Row 3: ODO:xxxxxxx
//   Row 4: FUL:xx  FSR:xxxxx
//   Row 5: TIME:xxxxxxx
//   Row 6: L/100:xxxxx L/h:xx
//   Row 7: km:xxxxx  L:xxxxx
//
// ADDR_ENGINE (0x01) — two screens (screen 0: core, screen 1: extended):
//   Screen 0 row 0: RPM:xxxx  V:xxx.x
//   Screen 0 row 1: T1:xxx T2:xxx T3:xxx
//   Screen 0 row 2: LAM:xxx LAM2:xxx
//   Screen 0 row 3: LOAD:xxx
//   Screen 1 row 0: TBa:xxxx STa:xxxx
//   Screen 1 row 1: mbar:xxxx bits:xxxxxxxx

namespace obd
{
namespace Display
{

void initCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected)
{
    switch (addrSelected)
    {
        case 0x17: // ADDR_INSTRUMENTS — single screen
            dm.print(0, 0, F("SPD:    RPM:    "));
            dm.print(0, 1, F("CLT:    OIL:    "));
            dm.print(0, 2, F("OLV:  OPR:  AMB:"));
            dm.print(0, 3, F("ODO:            "));
            dm.print(0, 4, F("FUL:   FSR:     "));
            dm.print(0, 5, F("TIME:           "));
            dm.print(0, 6, F("L/100:     L/h: "));
            dm.print(0, 7, F("km:      L:     "));
            break;
        case 0x01: // ADDR_ENGINE
            switch (screen)
            {
                case 0:
                    dm.print(0, 0, F("RPM:     V:     "));
                    dm.print(0, 1, F("T1:  T2:  T3:   "));
                    dm.print(0, 2, F("LAM:   LAM2:    "));
                    dm.print(0, 3, F("LOAD:           "));
                    break;
                case 1:
                    dm.print(0, 0, F("TBa:     STa:   "));
                    dm.print(0, 1, F("mbar:    bits:  "));
                    break;
                default:
                    dm.print(0, 0, F("Screen "));
                    dm.print(7, 0, String(screen));
                    dm.print(0, 1, F("not supported!"));
                    break;
            }
            break;
        default:
            dm.print(0, 0, F("Addr 0x"));
            dm.print(7, 0, String(addrSelected, HEX));
            dm.print(0, 1, F("not supported!"));
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
            printField(dm, 4, 0, ins.vehicleSpeed, 3, const_cast<bool&>(ins.vehicleSpeedUpdated),
                       forceUpdate);
            printField(dm, 12, 0, ins.engineRpm, 4, const_cast<bool&>(ins.engineRpmUpdated),
                       forceUpdate);
            printField(dm, 4, 1, ins.coolantTemp, 3, const_cast<bool&>(ins.coolantTempUpdated),
                       forceUpdate);
            printField(dm, 12, 1, ins.oilTemp, 3, const_cast<bool&>(ins.oilTempUpdated),
                       forceUpdate);
            printField(dm, 4, 2, ins.oilLevelOk, 1, const_cast<bool&>(ins.oilLevelOkUpdated),
                       forceUpdate);
            printField(dm, 9, 2, ins.oilPressureMin, 1,
                       const_cast<bool&>(ins.oilPressureMinUpdated), forceUpdate);
            printField(dm, 14, 2, ins.ambientTemp, 2, const_cast<bool&>(ins.ambientTempUpdated),
                       forceUpdate);
            printField(dm, 4, 3, ins.odometer, 10, const_cast<bool&>(ins.odometerUpdated),
                       forceUpdate);
            printField(dm, 4, 4, ins.fuelLevel, 2, const_cast<bool&>(ins.fuelLevelUpdated),
                       forceUpdate);
            printField(dm, 11, 4, ins.fuelSensorResistance, 5,
                       const_cast<bool&>(ins.fuelSensorResistanceUpdated), forceUpdate);
            printField(dm, 5, 5, ins.timeEcu, 9, const_cast<bool&>(ins.timeEcuUpdated),
                       forceUpdate);
            printFieldFloat(dm, 6, 6, c.fuelPer100km, 5, const_cast<bool&>(c.fuelPer100kmUpdated),
                            forceUpdate);
            printFieldFloat(dm, 16, 6, c.fuelPerHour, 4, const_cast<bool&>(c.fuelPerHourUpdated),
                            forceUpdate);
            printField(dm, 3, 7, c.elapsedKmSinceStart, 5,
                       const_cast<bool&>(c.elapsedKmSinceStartUpdated), forceUpdate);
            printFieldFloat(dm, 11, 7, c.fuelBurnedSinceStart, 5,
                            const_cast<bool&>(c.fuelBurnedSinceStartUpdated), forceUpdate);
            break;
        }
        case 0x01:
        {
            const EngineSignals& e = signals.engine;
            const InstrumentSignals& ins = signals.instruments;
            switch (screen)
            {
                case 0:
                    printField(dm, 4, 0, ins.engineRpm, 4, const_cast<bool&>(ins.engineRpmUpdated),
                               forceUpdate);
                    printFieldFloat(dm, 10, 0, e.voltage, 5, const_cast<bool&>(e.voltageUpdated),
                                    forceUpdate);
                    printField(dm, 3, 1, e.tempUnknown1, 3,
                               const_cast<bool&>(e.tempUnknown1Updated), forceUpdate);
                    printField(dm, 7, 1, e.tempUnknown2, 3,
                               const_cast<bool&>(e.tempUnknown2Updated), forceUpdate);
                    printField(dm, 13, 1, e.tempUnknown3, 3,
                               const_cast<bool&>(e.tempUnknown3Updated), forceUpdate);
                    printField(dm, 4, 2, e.lambda, 3, const_cast<bool&>(e.lambdaUpdated),
                               forceUpdate);
                    printField(dm, 13, 2, e.lambda2, 3, const_cast<bool&>(e.lambda2Updated),
                               forceUpdate);
                    printField(dm, 5, 3, e.engineLoad, 3, const_cast<bool&>(e.engineLoadUpdated),
                               forceUpdate);
                    break;
                case 1:
                {
                    printFieldFloat(dm, 4, 0, e.tbAngle, 5, const_cast<bool&>(e.tbAngleUpdated),
                                    forceUpdate);
                    printFieldFloat(dm, 13, 0, e.steeringAngle, 5,
                                    const_cast<bool&>(e.steeringAngleUpdated), forceUpdate);
                    printField(dm, 5, 1, e.pressure, 4, const_cast<bool&>(e.pressureUpdated),
                               forceUpdate);
                    EngineSignals& em = const_cast<EngineSignals&>(e);
                    if (em.errorBitsUpdated || forceUpdate)
                    {
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
                    printFieldStr(dm, 14, 1, em.bitsAsString, 8,
                                  const_cast<bool&>(em.errorBitsUpdated), forceUpdate);
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
