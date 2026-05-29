// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// Cooperative task intervals (milliseconds).
// KWP is timing-critical; interval=0 means run every loop iteration.
static constexpr uint16_t INTERVAL_KWP_MS = 0;
static constexpr uint16_t INTERVAL_INPUT_MS = 20;
static constexpr uint16_t INTERVAL_COMPUTE_MS = 50;
static constexpr uint16_t INTERVAL_DISPLAY_MS = 177;
static constexpr uint32_t INTERVAL_KEEPALIVE_MS = 800;
