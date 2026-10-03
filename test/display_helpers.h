// Helpers shared by the AVR-only display suites (test_display_*).
//
// On AVR every string literal is copied to RAM at startup, and the test images
// share 2 KB with Unity and the UART buffers. Expected strings and messages
// therefore live in flash: EXPECT_TEXT(got, "literal") and assertLayout()
// read them with the *_P functions.
#pragma once

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <unity.h>

#include "display/Display.h"
#include "obd/Display/DisplayManager.h"

// Friend of ::Display (see Display.h): read-only view of the text table.
struct DisplayTestAccess
{
    static uint8_t count(const ::Display& d) { return d.entryCount_; }
    static uint8_t maxEntries() { return ::Display::kMaxEntries; }
    static uint8_t x(const ::Display& d, uint8_t i) { return d.entries_[i].x; }
    static uint8_t line(const ::Display& d, uint8_t i) { return d.entries_[i].line; }
    static uint8_t scale(const ::Display& d, uint8_t i) { return d.entries_[i].scale; }
    static const char* text(const ::Display& d, uint8_t i) { return d.entries_[i].text; }
    static bool dirty(const ::Display& d) { return d.dirty_; }
    static void reset(::Display& d)
    {
        d.batchDepth_ = 0;
        d.entryCount_ = 0;
        d.dirty_ = false;
    }
};
using DTA = DisplayTestAccess;

static ::Display display;
static obd::Display::DisplayManager dm(display);

// A failed assertion skips the rest of a test, possibly leaving a frame open:
// start every test from an idle display so one failure cannot cascade.
void setUp()
{
    DTA::reset(display);
}
void tearDown() {}

// Start a frame the way OBDDisplay::updateDisplay does.
static void beginFrame()
{
    display.beginBatch();
    display.clear();
}

// Close the frame: flush() renders every entry into the page buffer.
static void endFrame()
{
    display.endBatch();
    TEST_ASSERT_FALSE(DTA::dirty(display));
}

// Text of the entry drawn at (x, line) with the given scale, or nullptr.
static const char* textAt(uint8_t scale, uint8_t x, uint8_t line)
{
    for (uint8_t i = 0; i < DTA::count(display); ++i)
        if (DTA::scale(display, i) == scale && DTA::x(display, i) == x &&
            DTA::line(display, i) == line)
            return DTA::text(display, i);
    return nullptr;
}
static const char* small(uint8_t col, uint8_t row)
{
    return textAt(1, (uint8_t)(col * 6), row);
}
static const char* big(uint8_t xPx, uint8_t yPx)
{
    return textAt(2, xPx, yPx);
}

static void expectText(const char* got, PGM_P want, uint16_t line)
{
    if (got != nullptr && strcmp_P(got, want) == 0)
        return;
    char msg[48];
    snprintf_P(msg, sizeof(msg), PSTR("want \"%S\", got \"%s\""), want, got ? got : "(none)");
    UNITY_TEST_FAIL(line, msg);
}
#define EXPECT_TEXT(got, literal) expectText((got), PSTR(literal), __LINE__)

// Every entry must be fully on the 64x128 panel, and the table must have room
// to spare: a full table means later draw calls of the frame were dropped.
// `what` is a flash string (PSTR).
static void assertLayout(PGM_P what, uint8_t detail = 0)
{
    char msg[72];
    uint8_t n = DTA::count(display);
    if (n >= DTA::maxEntries())
    {
        snprintf_P(msg, sizeof(msg), PSTR("%S %u: table full (%u entries)"), what, detail, n);
        TEST_FAIL_MESSAGE(msg);
    }
    for (uint8_t i = 0; i < n; ++i)
    {
        uint8_t x = DTA::x(display, i);
        uint8_t line = DTA::line(display, i);
        const char* t = DTA::text(display, i);
        uint8_t len = (uint8_t)strlen(t);
        bool ok;
        switch (DTA::scale(display, i))
        {
            case 1: // 6 px per char (5 px glyph + gap), 8 px rows
                ok = line < 16 && x + len * 6u <= 65u;
                break;
            case 2: // 12 px per char (10 px glyph + gap), 14 px tall
                ok = line + 14u <= 128u && x + len * 12u <= 66u;
                break;
            default: // 3/4: bar, text[0]=w, text[1]=h
                ok = x + (uint8_t)t[0] <= 64u && line + (uint8_t)t[1] <= 128u;
                t = "(bar)";
                break;
        }
        if (!ok)
        {
            snprintf_P(msg, sizeof(msg), PSTR("%S %u: \"%s\" x=%u line=%u scale=%u off panel"),
                       what, detail, t, x, line, DTA::scale(display, i));
            TEST_FAIL_MESSAGE(msg);
        }
    }
}
