#include <Arduino.h>
#include "Controller.h"
#include "scheduler/TaskConfig.h"

static Controller controller;

static Task tKwp(INTERVAL_KWP_MS, TASK_FOREVER, &Controller::taskKwp);
static Task tInput(INTERVAL_INPUT_MS, TASK_FOREVER, &Controller::taskInput);
static Task tCompute(INTERVAL_COMPUTE_MS, TASK_FOREVER, &Controller::taskCompute);
static Task tDisplay(INTERVAL_DISPLAY_MS, TASK_FOREVER, &Controller::taskDisplay);

static Scheduler runner;

void setup()
{
    controller.setup();

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
