// SPDX-License-Identifier: GPL-3.0-or-later
#include "CockpitScreen.h"
#include "ScreenHelpers.h"

// Portrait layout (10 cols x 16 rows):
//
// ADDR_INSTRUMENTS (0x17), screen 0 — all 15 signals:
//   Row 0:  SPD:XXX     Row 7:  FUL:XX
//   Row 1:  RPM:XXXX    Row 8:  FSR:XXXXX
//   Row 2:  CLT:XXX     Row 9:  TM:XXXXXXX
//   Row 3:  OIL:XXX     Row 10: L100:X.X
//   Row 4:  AMB:XXX     Row 11: L/h:X.X
//   Row 5:  OL:X OP:X   Row 12: km:XXXXX
//   Row 6:  ODO:XXXXXX  Row 13: L:X.X
//
// ADDR_ENGINE (0x01):
//   Screen 0: 8 signals stacked vertically
//   Screen 1: 4 signals + 8-bit error string

namespace obd
{
namespace Display
{

void initCockpitScreen(DisplayManager& /*dm*/, uint8_t /*screen*/, uint8_t /*addrSelected*/)
{
    // No-op: all rendering (labels + values) is done in renderCockpitScreen.
}

void renderCockpitScreen(DisplayManager& dm, uint8_t screen, uint8_t addrSelected,
                         const Model::OBDSignals& signals, bool forceUpdate)
{
    using namespace Model;

    char buf[11]; // Buffer for formatted strings (max 10 chars + null)

    switch (addrSelected)
    {
        case 0x17:
        {
            // Instruments cluster (address 0x17)
            const InstrumentSignals& ins = signals.instruments;
            const ComputedStats& c = signals.computed;
            (void)screen;

            // Row 0: SPD:XXX
            buf[0] = 'S';
            buf[1] = 'P';
            buf[2] = 'D';
            buf[3] = ':';
            ltoa(ins.vehicleSpeed, buf + 4, 10);
            dm.print(0, 0, buf);

            // Row 1: RPM:XXXX
            buf[0] = 'R';
            buf[1] = 'P';
            buf[2] = 'M';
            buf[3] = ':';
            ltoa(ins.engineRpm, buf + 4, 10);
            dm.print(0, 1, buf);

            // Row 2: CLT:XXX
            buf[0] = 'C';
            buf[1] = 'L';
            buf[2] = 'T';
            buf[3] = ':';
            ltoa(ins.coolantTemp, buf + 4, 10);
            dm.print(0, 2, buf);

            // Row 3: OIL:XXX
            buf[0] = 'O';
            buf[1] = 'I';
            buf[2] = 'L';
            buf[3] = ':';
            ltoa(ins.oilTemp, buf + 4, 10);
            dm.print(0, 3, buf);

            // Row 4: AMB:XXX
            buf[0] = 'A';
            buf[1] = 'M';
            buf[2] = 'B';
            buf[3] = ':';
            ltoa(ins.ambientTemp, buf + 4, 10);
            dm.print(0, 4, buf);

            // Row 5: OL:X OP:X (abbreviated oil level + oil pressure)
            buf[0] = 'O';
            buf[1] = 'L';
            buf[2] = ':';
            buf[3] = '0' + ins.oilLevelOk;
            buf[4] = ' ';
            buf[5] = 'O';
            buf[6] = 'P';
            buf[7] = ':';
            buf[8] = '0' + ins.oilPressureMin;
            buf[9] = '\0';
            dm.print(0, 5, buf);

            // Row 6: ODO:XXXXXX
            buf[0] = 'O';
            buf[1] = 'D';
            buf[2] = 'O';
            buf[3] = ':';
            ltoa(ins.odometer, buf + 4, 10);
            dm.print(0, 6, buf);

            // Row 7: FUL:XX
            buf[0] = 'F';
            buf[1] = 'U';
            buf[2] = 'L';
            buf[3] = ':';
            ltoa(ins.fuelLevel, buf + 4, 10);
            dm.print(0, 7, buf);

            // Row 8: FSR:XXXXX
            buf[0] = 'F';
            buf[1] = 'S';
            buf[2] = 'R';
            buf[3] = ':';
            ltoa(ins.fuelSensorResistance, buf + 4, 10);
            dm.print(0, 8, buf);

            // Row 9: TM:XXXXXXX
            buf[0] = 'T';
            buf[1] = 'M';
            buf[2] = ':';
            ltoa(ins.timeEcu, buf + 3, 10);
            dm.print(0, 9, buf);

            // Row 10: L100:X.X (fuel consumption per 100km, ×10 fixed-point)
            dm.print(0, 10, F("L100:"));
            dm.print(5, 10, (int32_t)c.fuelPer100km, 1, 5);

            // Row 11: L/h:X.X (fuel consumption per hour, ×10 fixed-point)
            dm.print(0, 11, F("L/h:"));
            dm.print(4, 11, (int32_t)c.fuelPerHour, 1, 4);

            // Row 12: km:XXXXX (distance since start)
            buf[0] = 'k';
            buf[1] = 'm';
            buf[2] = ':';
            ltoa(c.elapsedKmSinceStart, buf + 3, 10);
            dm.print(0, 12, buf);

            // Row 13: L:XX (fuel burned since start, integer litres)
            dm.print(0, 13, F("L:"));
            dm.print(2, 13, (int32_t)c.fuelBurnedSinceStart);

            break;
        }

        case 0x01:
        {
            // Engine controller (address 0x01)
            const EngineSignals& e = signals.engine;
            const InstrumentSignals& ins = signals.instruments;

            switch (screen)
            {
                case 0:
                {
                    // Row 0: RPM:XXXX
                    buf[0] = 'R';
                    buf[1] = 'P';
                    buf[2] = 'M';
                    buf[3] = ':';
                    ltoa(ins.engineRpm, buf + 4, 10);
                    dm.print(0, 0, buf);

                    // Row 1: V:X.X (voltage ×10 fixed-point)
                    dm.print(0, 1, F("V:"));
                    dm.print(2, 1, (int32_t)e.voltage, 1, 5);

                    // Row 2: T1:XXX
                    buf[0] = 'T';
                    buf[1] = '1';
                    buf[2] = ':';
                    ltoa(e.tempUnknown1, buf + 3, 10);
                    dm.print(0, 2, buf);

                    // Row 3: T2:XXX
                    buf[0] = 'T';
                    buf[1] = '2';
                    buf[2] = ':';
                    ltoa(e.tempUnknown2, buf + 3, 10);
                    dm.print(0, 3, buf);

                    // Row 4: T3:XXX
                    buf[0] = 'T';
                    buf[1] = '3';
                    buf[2] = ':';
                    ltoa(e.tempUnknown3, buf + 3, 10);
                    dm.print(0, 4, buf);

                    // Row 5: LAM:XXX
                    buf[0] = 'L';
                    buf[1] = 'A';
                    buf[2] = 'M';
                    buf[3] = ':';
                    ltoa(e.lambda, buf + 4, 10);
                    dm.print(0, 5, buf);

                    // Row 6: LAM2:XXX
                    buf[0] = 'L';
                    buf[1] = 'A';
                    buf[2] = 'M';
                    buf[3] = '2';
                    buf[4] = ':';
                    ltoa(e.lambda2, buf + 5, 10);
                    dm.print(0, 6, buf);

                    // Row 7: LD:XXX (LOAD)
                    buf[0] = 'L';
                    buf[1] = 'D';
                    buf[2] = ':';
                    ltoa(e.engineLoad, buf + 3, 10);
                    dm.print(0, 7, buf);

                    break;
                }

                case 1:
                {
                    // Row 0: TBa:X.X (×10 fixed-point)
                    dm.print(0, 0, F("TBa:"));
                    dm.print(4, 0, (int32_t)e.tbAngle, 1, 5);

                    // Row 1: STa:X.X (×10 fixed-point)
                    dm.print(0, 1, F("STa:"));
                    dm.print(4, 1, (int32_t)e.steeringAngle, 1, 5);

                    // Row 2: mb:XXXX
                    buf[0] = 'm';
                    buf[1] = 'b';
                    buf[2] = ':';
                    ltoa(e.pressure, buf + 3, 10);
                    dm.print(0, 2, buf);

                    // Row 3: bits: (label)
                    dm.print(0, 3, F("bits:"));

                    // Row 4: error bit string (8 bits)
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
                    dm.print(0, 4, em.bitsAsString);

                    break;
                }

                default:
                {
                    // Unknown screen fallback
                    dm.print(0, 0, F("Screen"));
                    ltoa((long)screen, buf, 10);
                    dm.print(7, 0, buf);
                    dm.print(0, 1, F("no data"));
                    break;
                }
            }
            break;
        }

        default:
        {
            // Unknown address fallback
            dm.print(0, 0, F("Addr 0x"));
            ltoa((long)addrSelected, buf, 16);
            dm.print(7, 0, buf);
            dm.print(0, 1, F("no data"));
            break;
        }
    }
}

} // namespace Display
} // namespace obd
