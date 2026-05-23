#pragma once

#include <Arduino.h>
#include "display/Display.h"
#include "obd/OBDDisplay.h"

class Controller
{
  public:
    Controller();

    void setup();
    void loop();

    static void taskKwp();
    static void taskInput();
    static void taskCompute();
    static void taskDisplay();

  private:
    static Controller* instance_;

    Display display_;
    obd::OBDDisplay* obdDisplay_;

    Controller(const Controller&) = delete;
    Controller& operator=(const Controller&) = delete;
};
