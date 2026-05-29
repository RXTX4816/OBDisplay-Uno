// SPDX-License-Identifier: GPL-3.0-or-later
#include "ScreenVM.h"
#include "DisplayManager.h"

namespace obd
{
namespace Display
{

static int32_t getFieldInt(FieldId fid, const ScreenCtx& ctx)
{
    const Model::OBDSignals* s = ctx.signals;
    const DebugInfo* d = ctx.debug;
    switch (fid)
    {
        // InstrumentSignals
        case FLD_VEH_SPEED:
            return s ? (int32_t)s->instruments.vehicleSpeed : 0;
        case FLD_ENG_RPM:
            return s ? (int32_t)s->instruments.engineRpm : 0;
        case FLD_COOLANT_T:
            return s ? (int32_t)s->instruments.coolantTemp : 0;
        case FLD_OIL_T:
            return s ? (int32_t)s->instruments.oilTemp : 0;
        case FLD_AMB_T:
            return s ? (int32_t)s->instruments.ambientTemp : 0;
        case FLD_OIL_LVL:
            return s ? (int32_t)s->instruments.oilLevelOk : 0;
        case FLD_OIL_PRES:
            return s ? (int32_t)s->instruments.oilPressureMin : 0;
        case FLD_ODOMETER:
            return s ? (int32_t)s->instruments.odometer : 0;
        case FLD_FUEL_LVL:
            return s ? (int32_t)s->instruments.fuelLevel : 0;
        case FLD_FUEL_RES:
            return s ? (int32_t)s->instruments.fuelSensorResistance : 0;
        case FLD_TIME_ECU:
            return s ? (int32_t)s->instruments.timeEcu : 0;
        // ComputedStats
        case FLD_FUEL_100:
            return s ? (int32_t)s->computed.fuelPer100km : 0;
        case FLD_FUEL_H:
            return s ? (int32_t)s->computed.fuelPerHour : 0;
        case FLD_ELAPSED_KM:
            return s ? (int32_t)s->computed.elapsedKmSinceStart : 0;
        case FLD_FUEL_BURNED:
            return s ? (int32_t)s->computed.fuelBurnedSinceStart : 0;
        // EngineSignals
        case FLD_VOLTAGE:
            return s ? (int32_t)s->engine.voltage : 0;
        case FLD_TEMP1:
            return s ? (int32_t)s->engine.tempUnknown1 : 0;
        case FLD_TEMP2:
            return s ? (int32_t)s->engine.tempUnknown2 : 0;
        case FLD_TEMP3:
            return s ? (int32_t)s->engine.tempUnknown3 : 0;
        case FLD_LAMBDA:
            return s ? (int32_t)s->engine.lambda : 0;
        case FLD_LAMBDA2:
            return s ? (int32_t)s->engine.lambda2 : 0;
        case FLD_ENG_LOAD:
            return s ? (int32_t)s->engine.engineLoad : 0;
        case FLD_TB_ANGLE:
            return s ? (int32_t)s->engine.tbAngle : 0;
        case FLD_STEER_ANGLE:
            return s ? (int32_t)s->engine.steeringAngle : 0;
        case FLD_PRESSURE:
            return s ? (int32_t)s->engine.pressure : 0;
        // ExperimentalGroup
        case FLD_EXP_GRP:
            return s ? (int32_t)s->experimental.groupCurrent : 0;
        case FLD_EXP_V0:
            return s ? s->experimental.v[0] : 0;
        case FLD_EXP_V1:
            return s ? s->experimental.v[1] : 0;
        case FLD_EXP_V2:
            return s ? s->experimental.v[2] : 0;
        case FLD_EXP_V3:
            return s ? s->experimental.v[3] : 0;
        // DebugInfo
        case FLD_DBG_CON:
            return d ? (int32_t)d->serialCon : 0;
        case FLD_DBG_AVA:
            return d ? (int32_t)d->serialAva : 0;
        case FLD_DBG_BC:
            return d ? (int32_t)d->blockCtr : 0;
        case FLD_DBG_GRP:
            return d ? (int32_t)d->group : 0;
        case FLD_DBG_ADDR:
            return d ? (int32_t)d->addr : 0;
        case FLD_DBG_BAUD:
            return d ? (int32_t)d->baud : 0;
        case FLD_DBG_ATT:
            return d ? (int32_t)d->attempts : 0;
        case FLD_DBG_SIM:
            return d ? (int32_t)d->sim : 0;
        case FLD_DBG_RAM:
            return d ? (int32_t)d->freeRam : 0;
        // Context
        case FLD_KWP_MODE:
            return (int32_t)ctx.kwpMode;
        default:
            return 0;
    }
}

static const char* getFieldStr(FieldId fid, const ScreenCtx& ctx)
{
    const Model::OBDSignals* s = ctx.signals;
    switch (fid)
    {
        case FLD_ERR_BITS_STR:
            return s ? s->engine.bitsAsString : "";
        case FLD_EXP_U0:
            return s ? s->experimental.unit[0] : "";
        case FLD_EXP_U1:
            return s ? s->experimental.unit[1] : "";
        case FLD_EXP_U2:
            return s ? s->experimental.unit[2] : "";
        case FLD_EXP_U3:
            return s ? s->experimental.unit[3] : "";
        default:
            return "";
    }
}

void runScript(const uint8_t* p, const ScreenCtx& ctx, const DisplayManager& dm)
{
    uint8_t op;
    while ((op = pgm_read_byte(p++)) != SO_END)
    {
        uint8_t x = pgm_read_byte(p++);
        uint8_t y = pgm_read_byte(p++);
        uint8_t fid = 0;
        switch (op)
        {
            case SO_LABEL:
            {
                uint8_t len = pgm_read_byte(p++);
                char buf[15];
                for (uint8_t i = 0; i < len; ++i)
                    buf[i] = (char)pgm_read_byte(p++);
                buf[len] = '\0';
                dm.print(x, y, buf);
                break;
            }
            case SO_U8:
            case SO_U16:
            case SO_U32:
            case SO_I8:
            case SO_I16:
                fid = pgm_read_byte(p++);
                dm.print(x, y, getFieldInt((FieldId)fid, ctx));
                break;

            case SO_SCALED:
            {
                fid = pgm_read_byte(p++);
                uint8_t w = pgm_read_byte(p++);
                dm.print(x, y, getFieldInt((FieldId)fid, ctx), 1, w);
                break;
            }
            case SO_STR:
                fid = pgm_read_byte(p++);
                dm.print(x, y, getFieldStr((FieldId)fid, ctx));
                break;

            case SO_BOOL_YN:
                fid = pgm_read_byte(p++);
                dm.print(x, y, getFieldInt((FieldId)fid, ctx) ? F("Y") : F("N"));
                break;

            case SO_HEX_U8:
            {
                fid = pgm_read_byte(p++);
                char buf[5];
                buf[0] = '0';
                buf[1] = 'x';
                ltoa(getFieldInt((FieldId)fid, ctx), buf + 2, 16);
                dm.print(x, y, buf);
                break;
            }
            case SO_CURSOR:
            {
                uint8_t target = pgm_read_byte(p++);
                uint8_t len = pgm_read_byte(p++);
                char buf[15];
                buf[0] = (ctx.cursor == target) ? '>' : ' ';
                for (uint8_t i = 0; i < len; ++i)
                    buf[i + 1] = (char)pgm_read_byte(p++);
                buf[len + 1] = '\0';
                dm.print(x, y, buf);
                break;
            }
            case SO_MODE_STR:
                p++; // consume fid (always kwpMode, already in ctx)
                switch (ctx.kwpMode)
                {
                    case 0:
                        dm.print(x, y, F("ACK"));
                        break;
                    case 2:
                        dm.print(x, y, F("Grp"));
                        break;
                    default:
                        dm.print(x, y, F("Sensor"));
                        break;
                }
                break;

            default:
                break;
        }
    }
}

} // namespace Display
} // namespace obd
