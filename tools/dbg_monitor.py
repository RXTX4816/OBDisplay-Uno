#!/usr/bin/env python3
"""
OBDisplay-Uno debug frame decoder.

Reads binary debug frames from the Arduino serial port and prints human-readable output.
Firmware must be built with -D OBD_DEBUG (use: pio run -e uno_debug).

Frame format: 0xAA <code> <val_hi> <val_lo> 0x55  (5 bytes)

Usage:
    python tools/dbg_monitor.py --port /dev/ttyUSB0
    python tools/dbg_monitor.py --port COM3 --baud 115200
    python tools/dbg_monitor.py --port /dev/ttyUSB0 --raw   # also show unknown bytes as hex
"""

import argparse
import sys
import time
from datetime import datetime

try:
    import serial
except ImportError:
    print("pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

# ---------------------------------------------------------------------------
# Event code table — keep in sync with src/debug.h
# ---------------------------------------------------------------------------
EVENTS = {
    # KWP protocol (0x01–0x0F)
    0x01: ("KWP_CONNECT",        "Connecting; baud = val*100"),
    0x02: ("KWP_5BAUD_START",    "5-baud init started"),
    0x03: ("KWP_5BAUD_DONE",     "5-baud init done"),
    0x04: ("KWP_SYNC_WAIT",      "Waiting for sync bytes"),
    0x05: ("KWP_SYNC_FAIL",      "Sync bytes receive FAILED"),
    0x06: ("KWP_SYNC_MISMATCH",  "Sync bytes mismatch; first byte = 0x{val:02X}"),
    0x07: ("KWP_SYNC_OK",        "Sync OK; first byte = 0x{val:02X} (expect 0x55)"),
    0x08: ("KWP_BLOCKS_READ",    "Reading device data blocks"),
    0x09: ("KWP_BLOCKS_FAIL",    "Device data read FAILED"),
    0x0A: ("KWP_TIMEOUT",        "receiveBlock_ TIMEOUT; received {val} bytes"),
    0x0B: ("KWP_COMPLEMENT",     "Complement mismatch at byte index {val}"),
    0x0C: ("KWP_KEEPALIVE_TX",   "Keep-alive send ACK FAILED"),
    0x0D: ("KWP_KEEPALIVE_RX",   "Keep-alive receive ACK FAILED"),

    # Display driver (0x10–0x1F)
    0x10: ("DISP_INIT",          "Display::begin() starting"),
    0x11: ("DISP_WIRE_OK",       "Wire initialized at 100 kHz"),
    0x12: ("DISP_OFF",           "Sending display OFF command"),
    0x13: ("DISP_SEQ",           "Sending init sequence"),
    0x14: ("DISP_INIT_DONE",     "Init complete"),
    0x15: ("DISP_CLEAR",         "Clearing display buffer"),
    0x16: ("DISP_READY",         "Display cleared and ready"),

    # Controller / startup (0x20–0x2F)
    0x20: ("CTRL_STEP",          "Startup step {val}"),
}

FRAME_START = 0xAA
FRAME_END   = 0x55
FRAME_LEN   = 5  # AA code hi lo 55


def fmt_event(code: int, val: int) -> str:
    if code not in EVENTS:
        return f"UNKNOWN(0x{code:02X}) val={val}"
    name, template = EVENTS[code]
    try:
        desc = template.format(val=val)
    except KeyError:
        desc = template
    return f"{name:<22}  {desc}"


def decode_stream(port: str, baud: int, show_raw: bool) -> None:
    print(f"Opening {port} at {baud} baud ... (Ctrl-C to stop)\n")
    with serial.Serial(port, baud, timeout=1) as ser:
        buf = bytearray()
        while True:
            chunk = ser.read(64)
            if not chunk:
                continue
            buf.extend(chunk)

            while len(buf) >= FRAME_LEN:
                # Find the next start byte
                start = buf.find(FRAME_START)
                if start == -1:
                    if show_raw and buf:
                        print(f"  RAW: {buf.hex()}")
                    buf.clear()
                    break
                if start > 0:
                    if show_raw:
                        print(f"  RAW: {buf[:start].hex()}")
                    del buf[:start]

                if len(buf) < FRAME_LEN:
                    break

                # Check end byte
                if buf[4] != FRAME_END:
                    if show_raw:
                        print(f"  BAD_FRAME: {buf[:FRAME_LEN].hex()}")
                    del buf[0]  # skip bad start byte and rescan
                    continue

                code = buf[1]
                val  = (buf[2] << 8) | buf[3]
                del buf[:FRAME_LEN]

                ts = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                print(f"[{ts}] {fmt_event(code, val)}")


def main() -> None:
    parser = argparse.ArgumentParser(description="OBDisplay-Uno debug frame decoder")
    parser.add_argument("--port", required=True, help="Serial port (e.g. /dev/ttyUSB0 or COM3)")
    parser.add_argument("--baud", type=int, default=115200, help="Baud rate (default: 115200)")
    parser.add_argument("--raw",  action="store_true", help="Show unrecognized bytes as hex")
    args = parser.parse_args()

    try:
        decode_stream(args.port, args.baud, args.raw)
    except serial.SerialException as e:
        print(f"Serial error: {e}")
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nStopped.")


if __name__ == "__main__":
    main()
