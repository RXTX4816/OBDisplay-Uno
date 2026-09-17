// SPDX-License-Identifier: GPL-3.0-or-later
#include "Buzzer.h"

#ifdef BUZZER_PIN
void buzzerBegin()
{
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void buzzerBeep(uint8_t count, uint8_t onMs)
{
    for (uint8_t i = 0; i < count; ++i)
    {
        if (i)
            delay(BUZZER_GAP_MS);
        digitalWrite(BUZZER_PIN, HIGH);
        delay(onMs);
        digitalWrite(BUZZER_PIN, LOW);
    }
}

void beepWarning(uint8_t level)
{
    if (level == 0)
        return;
    if (level > 3)
        level = 3;
    buzzerBeep(BUZZER_BEEP_COUNT[level - 1], BUZZER_BEEP_MS[level - 1]);
}
#endif
