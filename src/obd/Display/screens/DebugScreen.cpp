// SPDX-License-Identifier: GPL-3.0-or-later
#include "DebugScreen.h"

namespace obd
{
namespace Display
{

// Debug layout (10 cols × 16 rows):
//   Row 0:  Con: 1        (NewSoftwareSerial isListening)
//   Row 1:  Ava: 0        (bytes available in RX buffer)
//   Row 2:  BC: 42        (KWP block counter)
//   Row 3:  KWP: Sensor   (mode name)
//   Row 4:  Grp: 3        (current KWP group)
//   Row 5:  Adr: 0x17     (ECU address)
//   Row 6:  Baud:9600     (baud rate)
//   Row 7:  Att: 0        (connection attempts)
//   Row 8:  Sim: N        (simulation mode active)
//   Row 9:  RAM: 834      (estimated free RAM bytes)

void initDebugScreen(DisplayManager& /*dm*/) {}

void renderDebugScreen(DisplayManager& dm, const DebugInfo& di, int kwpModeInt)
{
#ifdef OBD_EXPERIMENTAL_SCREENS
    char buf[7];

    dm.print(0, 0, F("Con:"));
    dm.print(4, 0, (int32_t)di.serialCon);

    dm.print(0, 1, F("Ava:"));
    dm.print(4, 1, (int32_t)di.serialAva);

    dm.print(0, 2, F("BC:"));
    dm.print(3, 2, (int32_t)di.blockCtr);

    dm.print(0, 3, F("KWP:"));
    switch (kwpModeInt)
    {
        case 0:
            dm.print(4, 3, F("ACK"));
            break;
        case 2:
            dm.print(4, 3, F("Grp"));
            break;
        default:
            dm.print(4, 3, F("Sensor"));
            break;
    }

    dm.print(0, 4, F("Grp:"));
    dm.print(4, 4, (int32_t)di.group);

    // Address as "0xNN"
    dm.print(0, 5, F("Adr:"));
    buf[0] = '0';
    buf[1] = 'x';
    ltoa((long)di.addr, buf + 2, 16);
    dm.print(4, 5, buf);

    dm.print(0, 6, F("Baud:"));
    dm.print(5, 6, (int32_t)di.baud);

    dm.print(0, 7, F("Att:"));
    dm.print(4, 7, (int32_t)di.attempts);

    dm.print(0, 8, F("Sim:"));
    dm.print(4, 8, di.sim ? F("Y") : F("N"));

    dm.print(0, 9, F("RAM:"));
    dm.print(4, 9, (int32_t)di.freeRam);

    (void)buf;
#else
    (void)dm;
    (void)di;
    (void)kwpModeInt;
#endif
}

} // namespace Display
} // namespace obd
