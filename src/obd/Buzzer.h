// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>
#include "Config.h"

#ifdef BUZZER_PIN
void buzzerBegin();
// Plays `count` beeps of `onMs` each. Blocks for the pattern (≤ ~410 ms): the KWP
// loop can stall longer than a beep, so a non-blocking off-edge would stretch it.
void buzzerBeep(uint8_t count, uint8_t onMs);
// Plays the pattern for warning level 1–3 from Config.h.
void beepWarning(uint8_t level);
#else
inline void buzzerBegin() {}
inline void buzzerBeep(uint8_t, uint8_t) {}
inline void beepWarning(uint8_t) {}
#endif
