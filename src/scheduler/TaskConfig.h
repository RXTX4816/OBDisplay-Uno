// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Cooperative task scheduler intervals (milliseconds).
// KWP serial is timing-critical and runs every loop iteration (0 = no delay).
// Display and compute tasks use longer intervals to avoid starving the ECU comms.
#define _TASK_SLEEP_ON_IDLE_RUN
#include <TaskScheduler.h>

static constexpr uint32_t INTERVAL_KWP_MS = 0;         // every loop — ECU must not time out
static constexpr uint32_t INTERVAL_INPUT_MS = 20;      // button latch polling interval
static constexpr uint32_t INTERVAL_COMPUTE_MS = 50;    // fuel calc, derived stats
static constexpr uint32_t INTERVAL_DISPLAY_MS = 177;   // ~5.6 FPS
static constexpr uint32_t INTERVAL_KEEPALIVE_MS = 800; // KWP ACK when idle
