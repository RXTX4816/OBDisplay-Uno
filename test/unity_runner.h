// Shared Unity entry point for every test suite.
//
// Each suite defines runTests() with its RUN_TEST() calls and ends with
// UNITY_SUITE_MAIN(). The same suite then runs on the host ([env:native],
// main()) and on a simulated ATmega328P ([env:uno_sim], setup() under simavr).
// It is a macro so that __FILE__ in failure reports names the suite file.
#pragma once

#include <unity.h>

#ifdef ARDUINO
#include <Arduino.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// Drain the UART, then sleep with interrupts off: simavr treats that as a clean
// exit, so the test command terminates instead of spinning forever.
#define UNITY_SUITE_MAIN()                                                                         \
    void setup()                                                                                   \
    {                                                                                              \
        UNITY_BEGIN();                                                                             \
        runTests();                                                                                \
        UNITY_END();                                                                               \
        Serial.flush();                                                                            \
        cli();                                                                                     \
        sleep_enable();                                                                            \
        sleep_cpu();                                                                               \
    }                                                                                              \
    void loop() {}
#else
#define UNITY_SUITE_MAIN()                                                                         \
    int main()                                                                                     \
    {                                                                                              \
        UNITY_BEGIN();                                                                             \
        runTests();                                                                                \
        return UNITY_END();                                                                        \
    }
#endif
