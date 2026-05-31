// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <Arduino.h>

// Uncomment and set pin when buzzer is physically wired (pins 10–13 are free).
// #define BUZZER_PIN 10

#ifdef BUZZER_PIN
inline void beepWarning(uint8_t level)
{
    uint16_t freq = (level >= 3) ? 1760u : (level >= 2) ? 880u : 440u;
    tone(BUZZER_PIN, freq, 200);
}
#else
inline void beepWarning(uint8_t) {}
#endif
