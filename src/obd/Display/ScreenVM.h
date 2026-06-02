// SPDX-License-Identifier: GPL-3.0-or-later
// Minimal PROGMEM byte-stream screen VM.
// Each screen is a static const uint8_t[] script interpreted by runScript().
// Adding a new screen requires only a new PROGMEM script array — no new code.
#pragma once

#include <avr/pgmspace.h>
#include <stdint.h>
#include "../Model/OBDSignals.h"

namespace obd
{
namespace Display
{

struct DebugInfo; // forward — defined in DisplayManager.h
class DisplayManager;

enum ScreenOp : uint8_t
{
    SO_END = 0x00,      // end of script
    SO_LABEL = 0x01,    // [x, y, len, chars...]
    SO_U8 = 0x02,       // [x, y, fid]
    SO_U16 = 0x03,      // [x, y, fid]
    SO_U32 = 0x04,      // [x, y, fid]
    SO_I8 = 0x05,       // [x, y, fid]
    SO_I16 = 0x06,      // [x, y, fid]
    SO_SCALED = 0x07,   // [x, y, fid, width] — ×10 fixed-point, 1 decimal place
    SO_STR = 0x08,      // [x, y, fid]         — null-terminated char*
    SO_BOOL_YN = 0x09,  // [x, y, fid]         — prints "Y" or "N"
    SO_HEX_U8 = 0x0A,   // [x, y, fid]         — prints "0xNN"
    SO_CURSOR = 0x0B,   // [x, y, target, len, chars...] — ">label" or " label"
    SO_MODE_STR = 0x0C, // [x, y, fid]         — ACK/Grp/Sensor from uint8_t kwpMode
    SO_BIN_U8 = 0x0D,   // [x, y, fid]         — prints 8-bit binary "01101001"
};

enum FieldId : uint8_t
{
    // InstrumentSignals
    FLD_VEH_SPEED,
    FLD_ENG_RPM,
    FLD_COOLANT_T,
    FLD_OIL_T,
    FLD_AMB_T,
    FLD_OIL_LVL,
    FLD_OIL_PRES,
    FLD_ODOMETER,
    FLD_FUEL_LVL,
    FLD_FUEL_RES,
    FLD_TIME_ECU,
    // ComputedStats
    FLD_FUEL_100,
    FLD_FUEL_H,
    FLD_ELAPSED_KM,
    FLD_FUEL_BURNED,
    // EngineSignals
    FLD_VOLTAGE,
    FLD_TEMP1,
    FLD_TEMP2,
    FLD_TEMP3,
    FLD_LAMBDA,
    FLD_LAMBDA2,
    FLD_ENG_LOAD,
    FLD_TB_ANGLE,
    FLD_STEER_ANGLE,
    FLD_PRESSURE,
    FLD_ERR_BITS_STR, // char* — bitsAsString, assembled by caller before runScript()
                      // ExperimentalGroup
    FLD_EXP_GRP,
    FLD_EXP_V0,
    FLD_EXP_V1,
    FLD_EXP_V2,
    FLD_EXP_V3,
    FLD_EXP_U0, // char*
    FLD_EXP_U1, // char*
    FLD_EXP_U2, // char*
    FLD_EXP_U3, // char*
    FLD_EXP_K0,
    FLD_EXP_K1,
    FLD_EXP_K2,
    FLD_EXP_K3,
    // DebugInfo
    FLD_DBG_CON,
    FLD_DBG_AVA,
    FLD_DBG_BC,
    FLD_DBG_GRP,
    FLD_DBG_ADDR,
    FLD_DBG_BAUD,
    FLD_DBG_ATT,
    FLD_DBG_RAM,
    // Context
    FLD_KWP_MODE,
};

struct ScreenCtx
{
    const Model::OBDSignals* signals; // nullptr for debug-only screens
    const DebugInfo* debug;           // nullptr for signal-only screens
    uint8_t cursor;
    uint8_t kwpMode;
};

void runScript(const uint8_t* pgm_script, const ScreenCtx& ctx, const DisplayManager& dm);

} // namespace Display
} // namespace obd
