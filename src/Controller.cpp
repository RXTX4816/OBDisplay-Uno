#include "Controller.h"

Controller* Controller::instance_ = nullptr;

Controller::Controller() : display_(), obdDisplay_(nullptr)
{
    instance_ = this;
}

void Controller::setup()
{
    Serial.println(F("1"));
    obdDisplay_ = new obd::OBDDisplay(3, 2, display_); // RX=3, TX=2
    Serial.println(F("2"));
    obdDisplay_->begin();
    Serial.println(F("3"));
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
