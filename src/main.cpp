// SPDX-License-Identifier: GPL-3.0-or-later
#include <Arduino.h>
#include "Controller.h"
#include "scheduler/TaskConfig.h"
#include "debug.h"

static Controller controller;

static Task tKwp(INTERVAL_KWP_MS, TASK_FOREVER, &Controller::taskKwp);
static Task tInput(INTERVAL_INPUT_MS, TASK_FOREVER, &Controller::taskInput);
static Task tCompute(INTERVAL_COMPUTE_MS, TASK_FOREVER, &Controller::taskCompute);
static Task tDisplay(INTERVAL_DISPLAY_MS, TASK_FOREVER, &Controller::taskDisplay);

static Scheduler runner;

void setup()
{
    pinMode(13, OUTPUT);
    digitalWrite(13, HIGH);
    delay(500);
    digitalWrite(13, LOW);

#ifdef OBD_DEBUG
    Serial.begin(115200);
    delay(500);
    DBG(DBG_CTRL_STEP); // step 0: serial ready
#endif

    digitalWrite(13, HIGH);
    delay(100);
    digitalWrite(13, LOW);

    controller.setup();
    DBG(DBG_CTRL_STEP); // step 0 repeated here is fine; Controller.cpp sends step 1/2/3

    runner.init();
    runner.addTask(tKwp);
    runner.addTask(tInput);
    runner.addTask(tCompute);
    runner.addTask(tDisplay);

    tKwp.enable();
    tInput.enable();
    tCompute.enable();
    tDisplay.enable();
}

void loop()
{
    // The scheduler calls enabled task callbacks at their configured intervals.
    // KWP (tKwp interval = 0) runs on every execute() call, ensuring the
    // timing-critical ECU serial work is never starved by display or compute tasks.
    runner.execute();

    // OBDDisplay::update() contains the cooperative state machine.
    // Until the task callbacks are fully wired to individual sub-systems,
    // the full update still runs here so nothing is broken.
    controller.loop();
}
