// KWP-1281 block reception (KWP1281Session::receiveBlock_) on the simulated
// ATmega328P. Bytes are placed straight into the soft-serial RX buffer, as the
// pin-change ISR would; the serial is never begun, so the acknowledgement bytes
// the session writes back go nowhere.
//
// Every receive buffer is followed by a canary: a byte stored past maxsize used
// to overwrite the stack and crash the firmware with a garbled screen (#80).

#include "../unity_runner.h"

#include "obd/KWP/KWP1281Session.h"

struct SoftSerialTestAccess
{
    static void push(uint8_t b)
    {
        NewSoftwareSerial::_receive_buffer[NewSoftwareSerial::_receive_buffer_tail] = (char)b;
        NewSoftwareSerial::_receive_buffer_tail =
            (uint8_t)((NewSoftwareSerial::_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF);
    }
};

namespace obd
{
namespace KWP
{
struct KwpTestAccess
{
    static bool receive(KWP1281Session& k, uint8_t* s, int maxsize, int& size, bool initPhase)
    {
        return k.receiveBlock_(s, maxsize, size, -1, initPhase);
    }
};
} // namespace KWP
} // namespace obd

using obd::KWP::KWP1281Session;
using obd::KWP::KwpTestAccess;

static NewSoftwareSerial serial(3, 2);
static KWP1281Session kwp(serial, 2);

static const uint8_t kCanary = 0xA5;

struct GuardedBuf
{
    uint8_t s[4];
    uint8_t canary[16];
};

static void initGuard(GuardedBuf& g)
{
    memset(g.s, 0, sizeof(g.s));
    memset(g.canary, kCanary, sizeof(g.canary));
}

static void assertCanaryIntact(const GuardedBuf& g)
{
    for (uint8_t i = 0; i < sizeof(g.canary); ++i)
        TEST_ASSERT_EQUAL_HEX8(kCanary, g.canary[i]);
}

static void pushAll(const uint8_t* b, uint8_t n)
{
    for (uint8_t i = 0; i < n; ++i)
        SoftSerialTestAccess::push(b[i]);
}

void setUp()
{
    serial.flush();
}
void tearDown() {}

// An ACK block (len 3, counter 0, title 0x09, end 0x03) immediately followed by
// the start of the next block and line noise, all already in the RX buffer.
// The block must end at its own end byte; the rest stays queued.
void test_block_ends_at_its_end_byte_and_leaves_following_bytes()
{
    kwp.setConfig(10400, 0x17);
    const uint8_t block[] = {0x03, 0x00, 0x09, 0x03};
    pushAll(block, sizeof(block));
    for (uint8_t i = 0; i < 40; ++i)
        SoftSerialTestAccess::push((uint8_t)(0x0F + i));

    GuardedBuf g;
    initGuard(g);
    int size = 0;
    TEST_ASSERT_TRUE(KwpTestAccess::receive(kwp, g.s, sizeof(g.s), size, false));
    TEST_ASSERT_EQUAL(4, size);
    TEST_ASSERT_EQUAL_HEX8_ARRAY(block, g.s, sizeof(block));
    assertCanaryIntact(g);
    TEST_ASSERT_EQUAL(40, serial.available());
    TEST_ASSERT_EQUAL(0x0F, serial.read());
}

// A full RX buffer of noise whose first byte announces a short block: only that
// block is consumed, nothing is written past it.
void test_noise_burst_cannot_write_past_the_buffer()
{
    kwp.setConfig(9600, 0x01);
    SoftSerialTestAccess::push(0x02); // len 2 → 3-byte block
    SoftSerialTestAccess::push(0x00); // block counter 0
    for (uint8_t i = 0; i < _SS_MAX_RX_BUFF - 3; ++i)
        SoftSerialTestAccess::push(0xEE);

    GuardedBuf g;
    initGuard(g);
    int size = 0;
    TEST_ASSERT_TRUE(KwpTestAccess::receive(kwp, g.s, sizeof(g.s), size, false));
    TEST_ASSERT_EQUAL(3, size);
    assertCanaryIntact(g);
    TEST_ASSERT_EQUAL(_SS_MAX_RX_BUFF - 4, serial.available());
}

// 1200 baud sync: garbage before 0x55 0x01 0x8A arrives in one burst. The resync
// logic still finds the sync bytes, but the garbage counted past maxsize is no
// longer stored behind the 3-byte buffer.
void test_low_baud_init_resync_stays_inside_the_buffer()
{
    kwp.setConfig(1200, 0x01);
    const uint8_t burst[] = {0xFF, 0x12, 0x34, 0x56, 0x78, 0x9A, 0x55, 0x01, 0x8A};
    pushAll(burst, sizeof(burst));

    GuardedBuf g;
    initGuard(g);
    int size = 3;
    TEST_ASSERT_TRUE(KwpTestAccess::receive(kwp, g.s, 3, size, true));
    TEST_ASSERT_EQUAL(3, size);
    const uint8_t sync[] = {0x55, 0x01, 0x8A};
    TEST_ASSERT_EQUAL_HEX8_ARRAY(sync, g.s, sizeof(sync));
    TEST_ASSERT_EQUAL_HEX8(0, g.s[3]);
    assertCanaryIntact(g);
}

// Nothing arrives: the receive gives up after the session timeout.
void test_silent_line_times_out()
{
    kwp.setConfig(10400, 0x17);
    GuardedBuf g;
    initGuard(g);
    int size = 0;
    uint32_t start = millis();
    TEST_ASSERT_FALSE(KwpTestAccess::receive(kwp, g.s, sizeof(g.s), size, false));
    uint32_t took = millis() - start;
    TEST_ASSERT_TRUE(took >= 1000 && took < 1500);
    assertCanaryIntact(g);
}

static void runTests()
{
    RUN_TEST(test_block_ends_at_its_end_byte_and_leaves_following_bytes);
    RUN_TEST(test_noise_burst_cannot_write_past_the_buffer);
    RUN_TEST(test_low_baud_init_resync_stays_inside_the_buffer);
    RUN_TEST(test_silent_line_times_out);
}

UNITY_SUITE_MAIN()
