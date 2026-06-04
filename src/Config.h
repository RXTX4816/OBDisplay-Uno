// SPDX-License-Identifier: GPL-3.0-or-later
// User-tunable hardware and threshold configuration.
// This is the single place to adjust pins, timing, and warning thresholds.
#pragma once

#include <stdint.h>

// ── Firmware version ──────────────────────────────────────────────────────────
// Injected at build time by tools/inject_version.py from the git tag.
#ifndef APP_VERSION
#define APP_VERSION "dev"
#endif

// ── Cooperative task intervals (ms) ──────────────────────────────────────────
// KWP is timing-critical; 0 = run every loop iteration.
static constexpr uint16_t INTERVAL_KWP_MS = 0;
static constexpr uint16_t INTERVAL_INPUT_MS = 20;
static constexpr uint16_t INTERVAL_COMPUTE_MS = 50;
static constexpr uint16_t INTERVAL_DISPLAY_MS = 177;
static constexpr uint32_t INTERVAL_KEEPALIVE_MS = 800;

// ── Timing ────────────────────────────────────────────────────────────────────
static constexpr uint16_t ECU_TIMEOUT_MS = 1300;
static constexpr uint16_t BUTTON_TIMEOUT_MS = 222;
static constexpr uint16_t RECONNECT_DELAY_MS = 5000;
static constexpr uint16_t BUTTON_REPEAT_INITIAL_MS = 400;
static constexpr uint16_t BUTTON_REPEAT_PERIOD_MS = 120;

// ── 5-way navigation switch pins (active LOW, INPUT_PULLUP) ──────────────────
static constexpr uint8_t BTN_PIN_UP = 4;
static constexpr uint8_t BTN_PIN_DOWN = 5;
static constexpr uint8_t BTN_PIN_LEFT = 6;
static constexpr uint8_t BTN_PIN_RIGHT = 7;
static constexpr uint8_t BTN_PIN_MID = 8;

// Button bitmask values for the pending-button latch.
static constexpr uint8_t BTN_MASK_RIGHT = 0x01;
static constexpr uint8_t BTN_MASK_LEFT = 0x02;
static constexpr uint8_t BTN_MASK_UP = 0x04;
static constexpr uint8_t BTN_MASK_DOWN = 0x08;
static constexpr uint8_t BTN_MASK_MID = 0x10;

// ── Warning thresholds ────────────────────────────────────────────────────────
// Temperatures in °C (×1 integer).
static constexpr uint8_t WARN_OIL_TEMP_HIGH_C = 93;
static constexpr uint8_t WARN_COOLANT_HIGH_C = 93;
static constexpr uint8_t WARN_COOLANT_COLD_C = 40; // below = very cold
static constexpr uint8_t WARN_COOLANT_WARM_C = 75; // below = cold engine
// Fuel level in L (×1 integer).
static constexpr uint8_t WARN_FUEL_CRIT_L = 4;
static constexpr uint8_t WARN_FUEL_LOW_L = 8;
// Voltage in ×10 units (120 = 12.0 V).
static constexpr uint16_t WARN_VOLTAGE_LOW_X10 = 120;
// Engine load in % (×1 integer).
static constexpr uint8_t WARN_ENGINE_LOAD_HIGH = 90;
// Oil level raw ECU value (0–255; 255 = full).
static constexpr uint8_t WARN_OIL_LVL_CRIT_RAW = 51; // <20% of 255
static constexpr uint8_t WARN_OIL_LVL_LOW_RAW = 115; // <45% of 255

// ── Bar gauge (0x17 page 2) ───────────────────────────────────────────────────
// Maximum fuel tank capacity in litres — adjust to match your vehicle.
static constexpr uint8_t FUEL_TANK_MAX_LITERS = 55;

// ── 0x01 engine ECU fuel model ────────────────────────────────────────────────
// fuelPerHour ×10 = RPM × engineLoad / ENGINE01_FUEL_DENOM
// Calibrate: at cruise (2700 RPM, 35% load → ~8 L/hr) → DENOM ≈ 1181, use 1200.
// If displayed L/100km reads 10% high, raise DENOM by 10%; if 10% low, lower it.
static constexpr uint16_t ENGINE01_FUEL_DENOM = 1200;

// ── 0x01 additional warning thresholds ───────────────────────────────────────
// Lambda controller % — spec range -15 to +15%.
static constexpr int8_t WARN_LAMBDA_LEAN_PCT = 10;  // above this → lean warning
static constexpr int8_t WARN_LAMBDA_RICH_PCT = -10; // below this → rich warning
// Intake air temperature °C.
static constexpr uint8_t WARN_INTAKE_HOT_C = 50;
