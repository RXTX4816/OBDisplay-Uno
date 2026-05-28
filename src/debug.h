// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Build-time binary debug logging.
// Enable: add -D OBD_DEBUG to build_flags (see [env:uno_debug] in platformio.ini).
// In production builds, all macros expand to nothing — zero flash cost.
// Decode output with: python tools/dbg_monitor.py --port /dev/ttyUSB0
//
// Wire format: 0xAA <code8> <val_hi8> <val_lo8> 0x55  (5 bytes per event)

// KWP protocol events (0x01–0x0F)
#define DBG_KWP_CONNECT 0x01       // connecting; val = baud / 100
#define DBG_KWP_5BAUD_START 0x02   // 5-baud init started
#define DBG_KWP_5BAUD_DONE 0x03    // 5-baud init done
#define DBG_KWP_SYNC_WAIT 0x04     // waiting for sync bytes
#define DBG_KWP_SYNC_FAIL 0x05     // sync bytes receive failed
#define DBG_KWP_SYNC_MISMATCH 0x06 // sync bytes mismatch; val = first byte received
#define DBG_KWP_SYNC_OK 0x07       // sync OK; val = first sync byte (should be 0x55)
#define DBG_KWP_BLOCKS_READ 0x08   // reading device data blocks
#define DBG_KWP_BLOCKS_FAIL 0x09   // device data read failed
#define DBG_KWP_TIMEOUT 0x0A       // receiveBlock_ timeout; val = bytes received so far
#define DBG_KWP_COMPLEMENT 0x0B    // complement mismatch; val = byte index
#define DBG_KWP_KEEPALIVE_TX 0x0C  // keepAlive send ACK failed
#define DBG_KWP_KEEPALIVE_RX 0x0D  // keepAlive receive ACK failed

// Display driver events (0x10–0x1F)
#define DBG_DISP_INIT 0x10      // Display::begin() start
#define DBG_DISP_WIRE_OK 0x11   // Wire initialized at 100 kHz
#define DBG_DISP_OFF 0x12       // sending display OFF command
#define DBG_DISP_SEQ 0x13       // sending init sequence
#define DBG_DISP_INIT_DONE 0x14 // init complete
#define DBG_DISP_CLEAR 0x15     // clearing display buffer
#define DBG_DISP_READY 0x16     // display ready

// Controller / startup events (0x20–0x2F)
#define DBG_CTRL_STEP 0x20 // startup step; val = step number

#ifdef OBD_DEBUG
#include <Arduino.h>
inline void _dbg(uint8_t code, uint16_t val = 0)
{
    Serial.write(0xAA);
    Serial.write(code);
    Serial.write(static_cast<uint8_t>(val >> 8));
    Serial.write(static_cast<uint8_t>(val & 0xFF));
    Serial.write(0x55);
}
#define DBG(code) _dbg(code)
#define DBGV(code, val) _dbg(code, static_cast<uint16_t>(val))
#else
#define DBG(code)
#define DBGV(code, val)
#endif
