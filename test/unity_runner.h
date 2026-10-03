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
#include <avr/pgmspace.h>
#include <avr/sleep.h>

// Unity keeps every test name as a string literal, i.e. in RAM on AVR: ~25
// bytes per test, enough to push a large suite into stack overflow. Keep the
// names in flash and copy only the running test's name into one buffer.
static char unityTestName[64];
#undef RUN_TEST
#define RUN_TEST(func)                                                                             \
    do                                                                                             \
    {                                                                                              \
        static const char name_[] PROGMEM = #func;                                                 \
        strncpy_P(unityTestName, name_, sizeof(unityTestName) - 1);                                \
        UnityDefaultTestRun(func, unityTestName, __LINE__);                                        \
    } while (0)

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
