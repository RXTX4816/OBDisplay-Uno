// SPDX-License-Identifier: GPL-3.0-or-later
#include "KWP1281Session.h"
#include "KWPSensorDecode.h"
#include "../../debug.h"

namespace obd
{
namespace KWP
{

KWP1281Session::KWP1281Session(NewSoftwareSerial& serial, uint8_t txPin)
    : obd_(serial), txPin_(txPin), baudRate_(0), ecuAddr_(0), blockCounter_(0), connected_(false),
      comError_(false), timeoutMs_(1100), lastConnectError_(0)
{
    pinMode(txPin_, OUTPUT);
    digitalWrite(txPin_, HIGH);
}

void KWP1281Session::setConfig(uint16_t baudRate, uint8_t ecuAddr)
{
    baudRate_ = baudRate;
    ecuAddr_ = ecuAddr;
}

void KWP1281Session::incrementBlockCounter_()
{
    if (blockCounter_ >= 255)
    {
        blockCounter_ = 0;
    }
    else
    {
        ++blockCounter_;
    }
}

void KWP1281Session::writeByte_(uint8_t data)
{
    // Debug printing is handled in the original file; here we
    // focus on timing and transmission.
    uint8_t toDelay = 5;
    switch (baudRate_)
    {
        case 1200:
        case 2400:
        case 4800:
            toDelay = 15; // For old ECUs
            break;
        case 9600:
            toDelay = 10;
            break;
        default:
            break;
    }

    delay(toDelay);
    obd_.write(data);
}

int16_t KWP1281Session::readByte_()
{
    unsigned long timeout = millis() + timeoutMs_;
    while (!obd_.available())
    {
        if (millis() >= timeout)
        {
            return -1;
        }
    }
    int16_t data = obd_.read();
    return data;
}

bool KWP1281Session::sendBlock_(const uint8_t* s, int size)
{
    for (uint8_t i = 0; i < size; ++i)
    {
        uint8_t data = s[i];
        writeByte_(data);

        if (i < size - 1)
        {
            int16_t complement = readByte_();
            if (s[2] == 0x06 && s[3] == 0x03 && complement == -1)
            {
                // Manual KWP exit
                return true;
            }
            if (complement != (data ^ 0xFF))
            {
                DBGV(DBG_KWP_COMPLEMENT, i);
                lastConnectError_ = DBG_KWP_COMPLEMENT;
                return false;
            }
        }
    }
    incrementBlockCounter_();
    return true;
}

bool KWP1281Session::receiveBlock_(uint8_t s[], int maxsize, int& size, int source,
                                   bool initializationPhase)
{
    bool ackEachByte = false;
    int recvCount = 0;
    if (size == 0)
        ackEachByte = true;

    if (size > maxsize)
    {
        return false;
    }

    unsigned long timeout = millis() + timeoutMs_;
    uint16_t tempIterationCounter = 0;
    uint8_t temp0x0FCounter = 0; // For communication errors in startup procedure (1200 baud)

    while ((recvCount == 0) || (recvCount != size))
    {
        while (obd_.available())
        {
            int16_t data = readByte_();
            if (data == -1)
            {
                return false;
            }
            s[recvCount] = (uint8_t)data;
            ++recvCount;

            // 1200/2400/4800 baud init-phase fix, mirrored from original
            if ((baudRate_ == 1200 || baudRate_ == 2400 || baudRate_ == 4800) &&
                initializationPhase && (recvCount > maxsize))
            {
                if (data == 0x55)
                {
                    temp0x0FCounter = 0;
                    s[0] = 0x55;
                    size = 3;
                    recvCount = 1;
                    timeout = millis() + timeoutMs_;
                }
                else if (data == 0xFF)
                {
                    temp0x0FCounter = 0;
                }
                else if (data == 0x0F)
                {
                    if (temp0x0FCounter >= 1)
                    {
                        writeByte_(data ^ 0xFF);
                        timeout = millis() + timeoutMs_;
                        temp0x0FCounter = 0;
                    }
                    else
                    {
                        ++temp0x0FCounter;
                    }
                }
                else
                {
                    temp0x0FCounter = 0;
                }
                continue;
            }

            if ((size == 0) && (recvCount == 1))
            {
                if (source == 1 && (data != 0x0F && data != 0x03) && obd_.available())
                {
                    comError_ = true;
                    size = 6;
                }
                else
                {
                    size = data + 1;
                }
                if (size > maxsize)
                {
                    return false;
                }
            }

            if (comError_)
            {
                if (recvCount == 1)
                {
                    ackEachByte = false;
                }
                else if (recvCount == 3)
                {
                    ackEachByte = true;
                }
                else if (recvCount == 4)
                {
                    ackEachByte = false;
                }
                else if (recvCount == 6)
                {
                    ackEachByte = true;
                }
                continue;
            }

            if ((ackEachByte) && (recvCount == 2))
            {
                if (data != blockCounter_)
                {
                    if (data == 0x00)
                    {
                        blockCounter_ = 0; // Reset during init-phase errors
                    }
                    else
                    {
                        return false;
                    }
                }
            }

            if (((!ackEachByte) && (recvCount == size)) || ((ackEachByte) && (recvCount < size)))
            {
                writeByte_(data ^ 0xFF);
            }
            timeout = millis() + timeoutMs_;
        }

        if (millis() >= timeout)
        {
            DBGV(DBG_KWP_TIMEOUT, recvCount);
            lastConnectError_ = DBG_KWP_TIMEOUT;
            return false;
        }
        ++tempIterationCounter;
    }

    incrementBlockCounter_();
    return true;
}

bool KWP1281Session::sendAckBlock_()
{
    const uint8_t buf[4] = {0x03, blockCounter_, 0x09, 0x03};
    return sendBlock_(buf, 4);
}

bool KWP1281Session::receiveAckBlock_()
{
    uint8_t buf[32];
    int size = 0;
    if (!receiveBlock_(buf, 32, size))
    {
        return false;
    }
    if (buf[2] != 0x09)
    {
        return false;
    }
    if (comError_)
    {
        // Error block handling: send error block then read response
        uint8_t s[64] = {0x03, blockCounter_, 0x00, 0x03};
        if (!sendBlock_(s, 4))
        {
            comError_ = false;
            return false;
        }
        blockCounter_ = 0;
        comError_ = false;
        int size2 = 0;
        if (!receiveBlock_(s, 64, size2))
        {
            return false;
        }
        return false;
    }
    return true;
}

bool KWP1281Session::readConnectBlocks_(bool initializationPhase)
{
    while (true)
    {
        int size = 0;
        uint8_t s[64];
        if (!receiveBlock_(s, 64, size, -1, initializationPhase))
        {
            return false;
        }
        if (size == 0)
            return false;
        if (s[2] == 0x09)
            break; // ACK
        if (s[2] != 0xF6)
        {
            lastConnectError_ = DBG_KWP_BLOCKS_FAIL;
            return false;
        }
        if (!sendAckBlock_())
            return false;
    }
    return true;
}

bool KWP1281Session::perform5BaudInit_()
{
    // 5Bd, 7O1: start(0), 7 data bits LSB-first, odd parity, stop(1).
    // Classic approach: direct GPIO bit-bang while serial is already open.
    // The TX pin is shared with NewSoftwareSerial but direct digitalWrite
    // overrides the library's idle-high state temporarily.
    const int bitcount = 10;
    byte bits[bitcount];
    byte even = 1;
    byte bit;
    for (int i = 0; i < bitcount; ++i)
    {
        bit = 0;
        if (i == 0)
        {
            bit = 0; // start bit
        }
        else if (i == 8)
        {
            bit = even; // odd parity
        }
        else if (i == 9)
        {
            bit = 1; // stop bit
        }
        else
        {
            bit = (byte)((ecuAddr_ & (1 << (i - 1))) != 0);
            even = even ^ bit;
        }
        bits[i] = bit;
    }

    for (int i = 0; i < bitcount + 1; ++i)
    {
        if (i != 0)
        {
            delay(200); // 200 ms per bit = 5 baud
            if (i == bitcount)
                break;
        }
        digitalWrite(txPin_, bits[i] ? HIGH : LOW);
    }
    obd_.flush();
    return true;
}

bool KWP1281Session::connectToEcu(bool simulationMode, bool autoSetup, uint16_t& baudRate,
                                  const uint8_t& addrSelected)
{
    (void)simulationMode;
    (void)autoSetup;

    lastConnectError_ = 0;
    setConfig(baudRate, addrSelected);
    if (baudRate_ == 0)
    {
        baudRate_ = 9600;
        baudRate = baudRate_;
    }

    DBGV(DBG_KWP_CONNECT, baudRate_ / 100);

    obd_.end();
    obd_.begin(baudRate_);

    DBG(DBG_KWP_5BAUD_START);
    perform5BaudInit_();
    DBG(DBG_KWP_5BAUD_DONE);

    // Handshake: expect 0x55, 0x01, 0x8A
    uint8_t response[3] = {0, 0, 0};
    int responseSize = 3;
    DBG(DBG_KWP_SYNC_WAIT);
    if (!receiveBlock_(response, 3, responseSize, -1, true))
    {
        DBG(DBG_KWP_SYNC_FAIL);
        if (!lastConnectError_)
            lastConnectError_ = DBG_KWP_SYNC_FAIL;
        return false;
    }
    DBGV(DBG_KWP_SYNC_OK, response[0]);

    if (response[0] != 0x55 || response[1] != 0x01 || response[2] != 0x8A)
    {
        DBGV(DBG_KWP_SYNC_MISMATCH, response[0]);
        lastConnectError_ = DBG_KWP_SYNC_MISMATCH;
        return false;
    }

    DBG(DBG_KWP_BLOCKS_READ);
    if (!readConnectBlocks_(false))
    {
        DBG(DBG_KWP_BLOCKS_FAIL);
        return false;
    }

    connected_ = true;
    return true;
}

void KWP1281Session::disconnect()
{
    if (!connected_)
        return;
    obd_.end();
    connected_ = false;
    blockCounter_ = 0;
}

bool KWP1281Session::keepAlive()
{
    if (!sendAckBlock_())
    {
        DBG(DBG_KWP_KEEPALIVE_TX);
        return false;
    }
    if (!receiveAckBlock_())
    {
        DBG(DBG_KWP_KEEPALIVE_RX);
        return false;
    }
    return true;
}

bool KWP1281Session::readSensorsGroup(uint8_t group, Model::OBDSignals& signals)
{
    // Reset temporary measurement arrays equivalent
    for (uint8_t i = 0; i < 4; ++i)
    {
        signals.experimental.k[i] = 0;
        signals.experimental.v[i] = -1;
        // Set unit text to "ERR" (3 chars + terminator, rest cleared)
        signals.experimental.unit[i][0] = 'E';
        signals.experimental.unit[i][1] = 'R';
        signals.experimental.unit[i][2] = 'R';
        signals.experimental.unit[i][3] = '\0';
        for (uint8_t j = 4; j < obd::Model::ExperimentalGroup::UnitWidth + 1; ++j)
        {
            signals.experimental.unit[i][j] = '\0';
        }
    }

    uint8_t s[64];
    s[0] = 0x04;
    s[1] = blockCounter_;
    s[2] = 0x29;
    s[3] = group;
    s[4] = 0x03;

    if (!sendBlock_(s, 5))
        return false;

    int size = 0;
    if (!receiveBlock_(s, 64, size, 1))
        return false;

    if (comError_)
    {
        uint8_t e[64];
        e[0] = 0x03;
        e[1] = blockCounter_;
        e[2] = 0x00;
        e[3] = 0x03;
        if (!sendBlock_(e, 4))
        {
            comError_ = false;
            return false;
        }
        blockCounter_ = 0;
        comError_ = false;
        int size2 = 0;
        if (!receiveBlock_(e, 64, size2))
        {
            return false;
        }
    }

    if (s[2] != 0xE7)
    {
        bool isSpecialCase = false;
        bool isSuperSpecialCase = false;

        if (baudRate_ == 9600 && ecuAddr_ == 0x01)
        {
            if (group == 1)
            {
                if (s[2] == 0x02)
                {
                    isSpecialCase = true;
                }
                else if (s[2] == 0xF4)
                {
                    isSuperSpecialCase = true;
                }
                else
                {
                    delay(2000);
                    return false;
                }
            }
            else
            {
                if (s[2] == 0x02)
                {
                    isSpecialCase = true;
                }
                else if (s[2] == 0xF4)
                {
                    isSuperSpecialCase = true;
                }
                else
                {
                    delay(2000);
                    return false;
                }
            }
        }

        if (isSpecialCase)
        {
            switch (group)
            {
                case 1:
                {
                    uint16_t rpm = (uint16_t)(0.2f * s[4] * s[5]);
                    if (signals.instruments.engineRpm != rpm)
                    {
                        signals.instruments.engineRpm = rpm;
                        signals.instruments.engineRpmUpdated = true;
                    }

                    uint8_t cool = (uint8_t)(s[7] * (s[8] - 100) * 0.1f);
                    if (signals.instruments.coolantTemp != cool)
                    {
                        signals.instruments.coolantTemp = cool;
                        signals.instruments.coolantTempUpdated = true;
                    }

                    float volt = 0.001f * s[10] * s[11];
                    if (signals.engine.voltage != volt)
                    {
                        signals.engine.voltage = volt;
                        signals.engine.voltageUpdated = true;
                    }
                    break;
                }
                default:
                    break;
            }
            return true;
        }

        if (isSuperSpecialCase)
        {
            return true;
        }
    }

    int count = (size - 4) / 3;
    for (int idx = 0; idx < count; ++idx)
    {
        byte k = s[3 + idx * 3];
        byte a = s[3 + idx * 3 + 1];
        byte b = s[3 + idx * 3 + 2];
        processKwpMeasurement(ecuAddr_, group, idx, k, a, b, signals);
    }

    return true;
}

int8_t KWP1281Session::readDtcCodes(Model::DTCStore& dtcStore)
{
    // Minimal port of read_DTC_codes(); assumes simulation is
    // handled externally.

    uint8_t s[64];
    // Send DTC read block
    const uint8_t req[4] = {0x03, blockCounter_, 0x07, 0x03};
    if (!sendBlock_(req, 4))
        return -1;

    dtcStore.reset();
    uint8_t dtcCounter = 0;

    while (true)
    {
        int size = 0;
        if (!receiveBlock_(s, 64, size))
            return -1;

        if (s[2] == 0x09)
            break; // No more DTC blocks
        if (s[2] != 0xFC)
            return -1;

        int count = (size - 4) / 3;
        for (int i = 0; i < count; ++i)
        {
            uint8_t byteHigh = s[3 + 3 * i];
            uint8_t byteLow = s[3 + 3 * i + 1];
            uint8_t byteStatus = s[3 + 3 * i + 2];

            if (byteHigh == 0xFF && byteLow == 0xFF && byteStatus == 0x88)
            {
                // No DTC codes
            }
            else
            {
                uint16_t dtc = (byteHigh << 8) + byteLow;
                dtcStore.set(dtcCounter, dtc, byteStatus);
                ++dtcCounter;
            }
        }

        if (!sendAckBlock_())
        {
            return -1;
        }
    }

    return (int8_t)dtcCounter;
}

bool KWP1281Session::deleteDtcCodes()
{
    const uint8_t s[4] = {0x03, blockCounter_, 0x05, 0x03};
    if (!sendBlock_(s, 4))
        return false;

    int size = 0;
    uint8_t resp[64];
    if (!receiveBlock_(resp, 64, size))
        return false;
    if (resp[2] != 0x09)
        return false;
    return true;
}

bool KWP1281Session::exitSession()
{
    const uint8_t s[4] = {0x03, blockCounter_, 0x06, 0x03};
    if (!sendBlock_(s, 4))
    {
        return false;
    }
    return true;
}

} // namespace KWP
} // namespace obd
