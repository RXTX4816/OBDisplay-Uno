#include "Controller.h"

Controller* Controller::instance_ = nullptr;

Controller::Controller() : display_(), obdDisplay_(nullptr)
{
    instance_ = this;
}

void Controller::setup()
{
    obdDisplay_ = new obd::OBDDisplay(3, 2, display_); // RX=3, TX=2
    obdDisplay_->begin();
}

void Controller::loop()
{
    if (obdDisplay_ != nullptr)
    {
        obdDisplay_->update();
    }
}

void Controller::taskKwp() {}
void Controller::taskInput() {}
void Controller::taskCompute() {}
void Controller::taskDisplay() {}
