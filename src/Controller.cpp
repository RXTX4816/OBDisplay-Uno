// SPDX-License-Identifier: GPL-3.0-or-later
#include "Controller.h"
#include "debug.h"

Controller* Controller::instance_ = nullptr;

Controller::Controller() : display_(), obdDisplay_(nullptr)
{
    instance_ = this;
}

void Controller::setup()
{
    DBGV(DBG_CTRL_STEP, 1);
    // Static local: lives in BSS (no heap, no malloc/free needed).
    static obd::OBDDisplay staticObj(3, 2, display_); // RX=3, TX=2
    obdDisplay_ = &staticObj;
    DBGV(DBG_CTRL_STEP, 2);
    obdDisplay_->begin();
    DBGV(DBG_CTRL_STEP, 3);
}

void Controller::loop()
{
    if (obdDisplay_ != nullptr)
    {
        obdDisplay_->update();
    }
}

void Controller::taskKwp() {}
void Controller::taskInput()
{
    if (instance_ != nullptr && instance_->obdDisplay_ != nullptr)
    {
        instance_->obdDisplay_->pollButtons();
    }
}
void Controller::taskCompute() {}
void Controller::taskDisplay() {}
