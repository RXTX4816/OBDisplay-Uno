// SPDX-License-Identifier: GPL-3.0-or-later
#include <Arduino.h>
#include "Controller.h"
#include "scheduler/TaskConfig.h"
#include "debug.h"

static Controller controller;

struct SimpleTask
{
    uint16_t interval; // ms; 0 = run every iteration
    uint32_t next;
    void (*fn)();
};

static SimpleTask tasks[] = {
    {INTERVAL_KWP_MS, 0, Controller::taskKwp},
    {INTERVAL_INPUT_MS, 0, Controller::taskInput},
    {INTERVAL_COMPUTE_MS, 0, Controller::taskCompute},
    {INTERVAL_DISPLAY_MS, 0, Controller::taskDisplay},
};

void setup()
{
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);

#ifdef OBD_DEBUG
    uartDebugBegin();
    delay(500);
    DBG(DBG_CTRL_STEP);
#endif

    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);

    controller.setup();
    DBG(DBG_CTRL_STEP);
}

void loop()
{
    uint32_t now = millis();
    for (auto& t : tasks)
    {
        if (now >= t.next)
        {
            t.fn();
            t.next = (t.interval == 0) ? 0 : now + t.interval;
        }
    }
}
