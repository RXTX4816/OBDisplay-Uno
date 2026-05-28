// SPDX-License-Identifier: GPL-3.0-or-later
#include "DTCScreen.h"

namespace obd
{
namespace Display
{

// DTC main menu layout (10 cols × 16 rows):
//   Row 0:  DTC Menu
//   Row 2:  [>] Read
//   Row 4:  [ ] Clear
//   Row 6:  [ ] Show
//   Row 14: DTCs: XX    (count; "--" if never read)
//
// DTC show sub-view:
//   Row 0:  DTCs X-Y
//   Row 2:  #01 E:XXXXX
//   Row 3:     S:XXX
//   Row 5:  #02 E:XXXXX
//   Row 6:     S:XXX
//   Row 8:  #03 E:XXXXX
//   Row 9:     S:XXX
//   Row 11: #04 E:XXXXX
//   Row 12:    S:XXX
//   Row 14: U/D:pg Sel:bk

static const uint8_t rowV[4] = {2, 5, 8, 11};
static const uint8_t rowS[4] = {3, 6, 9, 12};

void renderDtcScreen(DisplayManager& dm, uint8_t cursor, bool showActive, uint8_t showPage,
                     int8_t dtcCount, const Model::DTCStore& dtcStore)
{
    char buf[11];

    if (!showActive)
    {
        // ── Main menu ───────────────────────────────────────────────
        dm.print(0, 0, F("DTC Menu"));

        // Three selectable actions with '>' cursor indicator
        dm.print(0, 2, cursor == 0 ? F(">Read") : F(" Read"));
        dm.print(0, 4, cursor == 1 ? F(">Clear") : F(" Clear"));
        dm.print(0, 6, cursor == 2 ? F(">Show") : F(" Show"));

        // DTC count at the bottom
        dm.print(0, 14, F("DTCs:"));
        if (dtcCount < 0)
        {
            dm.print(5, 14, F("--"));
        }
        else
        {
            ltoa((long)dtcCount, buf, 10);
            dm.print(5, 14, buf);
        }
    }
    else
    {
        // ── Show sub-view ─────────────────────────────────────────
        uint8_t base = showPage * 4; // first DTC index on this page (0-based)

        // Header: "DTCs X-Y" showing 1-based range
        uint8_t first = base + 1;
        uint8_t last = static_cast<uint8_t>(base + 4);
        buf[0] = 'D';
        buf[1] = 'T';
        buf[2] = 'C';
        buf[3] = 's';
        buf[4] = ' ';
        ltoa((long)first, buf + 5, 10);
        uint8_t len = 5;
        while (buf[len])
            len++;
        buf[len++] = '-';
        ltoa((long)last, buf + len, 10);
        dm.print(0, 0, buf);

        // Four DTC entries
        for (uint8_t i = 0; i < 4; ++i)
        {
            uint8_t idx = base + i;
            if (idx >= Model::DTCStore::MaxCount)
                break;

            uint16_t err = dtcStore.errorAt(idx);
            uint8_t sts = dtcStore.statusAt(idx);

            // Skip empty slots (0xFFFF = unset)
            if (err == 0xFFFF)
                continue;

            // "#N E:XXXXX" (idx<9) or "#NNE:XXXXX" (idx>=9) — always fits 10 cols
            buf[0] = '#';
            ltoa((long)(idx + 1), buf + 1, 10);
            uint8_t p = 1;
            while (buf[p])
                p++;
            if (idx < 9)
                buf[p++] = ' '; // space only for single-digit index number
            buf[p++] = 'E';
            buf[p++] = ':';
            ltoa((long)err, buf + p, 10);
            dm.print(0, rowV[i], buf);

            // "   S:XXX" — status decimal
            buf[0] = ' ';
            buf[1] = ' ';
            buf[2] = ' ';
            buf[3] = 'S';
            buf[4] = ':';
            ltoa((long)sts, buf + 5, 10);
            dm.print(0, rowS[i], buf);
        }

        dm.print(0, 14, F("U/D:pg Sel:bk"));
    }
}

} // namespace Display
} // namespace obd
