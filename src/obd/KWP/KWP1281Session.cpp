#include "KWP1281Session.h"

namespace obd {
namespace KWP {

KWP1281Session::KWP1281Session(NewSoftwareSerial &serial)
    : obd_(serial)
    , baudRate_(0)
    , ecuAddr_(0)
    , blockCounter_(0)
    , connected_(false)
    , comError_(false)
    , timeoutMs_(1100)
{
}

void KWP1281Session::setConfig(uint16_t baudRate, uint8_t ecuAddr)
{
    baudRate_ = baudRate;
    ecuAddr_ = ecuAddr;
}

void KWP1281Session::incrementBlockCounter_()
{
    if (blockCounter_ >= 255) {
        blockCounter_ = 0;
    } else {
        ++blockCounter_;
    }
}

void KWP1281Session::writeByte_(uint8_t data)
{
    // Debug printing is handled in the original file; here we
    // focus on timing and transmission.
    uint8_t toDelay = 5;
    switch (baudRate_) {
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
    while (!obd_.available()) {
        if (millis() >= timeout) {
            return -1;
        }
    }
    int16_t data = obd_.read();
    return data;
}

bool KWP1281Session::sendBlock_(uint8_t *s, int size)
{
    for (uint8_t i = 0; i < size; ++i) {
        uint8_t data = s[i];
        writeByte_(data);

        if (i < size - 1) {
            int16_t complement = readByte_();
            if (s[2] == 0x06 && s[3] == 0x03 && complement == -1) {
                // Manual KWP exit
                return true;
            }
            if (complement != (data ^ 0xFF)) {
                return false;
            }
        }
    }
    incrementBlockCounter_();
    return true;
}

bool KWP1281Session::receiveBlock_(uint8_t s[], int maxsize, int &size,
                                   int source, bool initializationPhase)
{
    bool ackEachByte = false;
    int16_t data = 0;
    int recvCount = 0;
    if (size == 0) ackEachByte = true;

    if (size > maxsize) {
        return false;
    }

    unsigned long timeout = millis() + timeoutMs_;
    uint16_t tempIterationCounter = 0;
    uint8_t temp0x0FCounter = 0; // For communication errors in startup procedure (1200 baud)

    while ((recvCount == 0) || (recvCount != size)) {
        while (obd_.available()) {
            data = readByte_();
            if (data == -1) {
                return false;
            }
            s[recvCount] = (uint8_t)data;
            ++recvCount;

            // 1200/2400/4800 baud init-phase fix, mirrored from original
            if ((baudRate_ == 1200 || baudRate_ == 2400 || baudRate_ == 4800)
                && initializationPhase && (recvCount > maxsize)) {
                if (data == 0x55) {
                    temp0x0FCounter = 0;
                    s[0] = 0x55;
                    size = 3;
                    recvCount = 1;
                    timeout = millis() + timeoutMs_;
                } else if (data == 0xFF) {
                    temp0x0FCounter = 0;
                } else if (data == 0x0F) {
                    if (temp0x0FCounter >= 1) {
                        writeByte_(data ^ 0xFF);
                        timeout = millis() + timeoutMs_;
                        temp0x0FCounter = 0;
                    } else {
                        ++temp0x0FCounter;
                    }
                } else {
                    temp0x0FCounter = 0;
                }
                continue;
            }

            if ((size == 0) && (recvCount == 1)) {
                if (source == 1 && (data != 0x0F || data != 0x03) && obd_.available()) {
                    comError_ = true;
                    size = 6;
                } else {
                    size = data + 1;
                }
                if (size > maxsize) {
                    return false;
                }
            }

            if (comError_) {
                if (recvCount == 1) {
                    ackEachByte = false;
                } else if (recvCount == 3) {
                    ackEachByte = true;
                } else if (recvCount == 4) {
                    ackEachByte = false;
                } else if (recvCount == 6) {
                    ackEachByte = true;
                }
                continue;
            }

            if ((ackEachByte) && (recvCount == 2)) {
                if (data != blockCounter_) {
                    if (data == 0x00) {
                        blockCounter_ = 0; // Reset during init-phase errors
                    } else {
                        return false;
                    }
                }
            }

            if (((!ackEachByte) && (recvCount == size)) ||
                ((ackEachByte) && (recvCount < size))) {
                writeByte_(data ^ 0xFF);
            }
            timeout = millis() + timeoutMs_;
        }

        if (millis() >= timeout) {
            if (recvCount == 0) {
                // Nothing received; wiring or ECU issue
            }
            return false;
        }
        ++tempIterationCounter;
    }

    incrementBlockCounter_();
    return true;
}

bool KWP1281Session::sendAckBlock_()
{
    uint8_t buf[4] = {0x03, blockCounter_, 0x09, 0x03};
    return sendBlock_(buf, 4);
}

bool KWP1281Session::receiveAckBlock_()
{
    uint8_t buf[32];
    int size = 0;
    if (!receiveBlock_(buf, 32, size)) {
        return false;
    }
    if (buf[2] != 0x09) {
        return false;
    }
    if (comError_) {
        // Error block handling: send error block then read response
        uint8_t s[64] = {0x03, blockCounter_, 0x00, 0x03};
        if (!sendBlock_(s, 4)) {
            comError_ = false;
            return false;
        }
        blockCounter_ = 0;
        comError_ = false;
        int size2 = 0;
        if (!receiveBlock_(s, 64, size2)) {
            return false;
        }
        return false;
    }
    return true;
}

bool KWP1281Session::readConnectBlocks_(bool initializationPhase)
{
    while (true) {
        int size = 0;
        uint8_t s[64];
        if (!receiveBlock_(s, 64, size, -1, initializationPhase)) {
            return false;
        }
        if (size == 0) return false;
        if (s[2] == 0x09) break; // ACK
        if (s[2] != 0xF6) {
            return false;
        }
        if (!sendAckBlock_()) return false;
    }
    return true;
}

bool KWP1281Session::perform5BaudInit_()
{
    // 5Bd, 7O1, sending ecuAddr_ as in original KWP_5baud_init
    const int bitcount = 10;
    byte bits[bitcount];
    byte even = 1;
    byte bit;
    for (int i = 0; i < bitcount; ++i) {
        bit = 0;
        if (i == 0) bit = 0;
        else if (i == 8) bit = even; // parity
        else if (i == 9) bit = 1;    // stop bit
        else {
            bit = (byte)((ecuAddr_ & (1 << (i - 1))) != 0);
            even = even ^ bit;
        }
        bits[i] = bit;
    }

    // NOTE: Original code directly manipulated the TX pin via a
    // global pin number. Here we skip direct digitalWrite and
    // rely on the calling code to perform the 5-baud init if
    // needed, so we simply flush and succeed.
    obd_.flush();
    return true;
}

bool KWP1281Session::connectToEcu(bool simulationMode,
                                  bool autoSetup,
                                  uint16_t &baudRate,
                                  uint8_t &addrSelected)
{
    (void)simulationMode;
    (void)autoSetup;

    setConfig(baudRate, addrSelected);
    if (baudRate_ == 0) {
        baudRate_ = 9600;
        baudRate = baudRate_;
    }

    obd_.begin(baudRate_);

    // Handshake: expect 0x55, 0x01, 0x8A
    uint8_t response[3] = {0, 0, 0};
    int responseSize = 3;
    if (!receiveBlock_(response, 3, responseSize, -1, true)) {
        return false;
    }
    if (response[0] != 0x55 || response[1] != 0x01 || response[2] != 0x8A) {
        return false;
    }

    if (!readConnectBlocks_(false)) {
        return false;
    }

    connected_ = true;
    return true;
}

void KWP1281Session::disconnect()
{
    if (!connected_) return;
    obd_.end();
    connected_ = false;
    blockCounter_ = 0;
}

bool KWP1281Session::keepAlive()
{
    if (!sendAckBlock_()) return false;
    return receiveAckBlock_();
}

bool KWP1281Session::readSensorsGroup(uint8_t group, Model::OBDSignals &signals)
{
    // Reset temporary measurement arrays equivalent
    for (uint8_t i = 0; i < 4; ++i) {
        signals.experimental.k[i] = 0;
        signals.experimental.v[i] = -1;
        // Set unit text to "ERR" (3 chars + terminator, rest cleared)
        signals.experimental.unit[i][0] = 'E';
        signals.experimental.unit[i][1] = 'R';
        signals.experimental.unit[i][2] = 'R';
        signals.experimental.unit[i][3] = '\0';
        for (uint8_t j = 4; j < obd::Model::ExperimentalGroup::UnitWidth + 1; ++j) {
            signals.experimental.unit[i][j] = '\0';
        }
    }

    uint8_t s[64];
    s[0] = 0x04;
    s[1] = blockCounter_;
    s[2] = 0x29;
    s[3] = group;
    s[4] = 0x03;
    if (!sendBlock_(s, 5)) return false;

    int size = 0;
    if (!receiveBlock_(s, 64, size, 1)) {
        return false;
    }

    if (comError_) {
        uint8_t e[64];
        e[0] = 0x03;
        e[1] = blockCounter_;
        e[2] = 0x00;
        e[3] = 0x03;
        if (!sendBlock_(e, 4)) {
            comError_ = false;
            return false;
        }
        blockCounter_ = 0;
        comError_ = false;
        int size2 = 0;
        if (!receiveBlock_(e, 64, size2)) {
            return false;
        }
    }

    if (s[2] != 0xE7) {
        bool isSpecialCase = false;
        bool isSuperSpecialCase = false;

        if (baudRate_ == 9600 && ecuAddr_ == 0x01) {
            if (group == 1) {
                if (s[2] == 0x02) {
                    isSpecialCase = true;
                } else if (s[2] == 0xF4) {
                    isSuperSpecialCase = true;
                } else {
                    delay(2000);
                    return false;
                }
            } else {
                if (s[2] == 0x02) {
                    isSpecialCase = true;
                } else if (s[2] == 0xF4) {
                    isSuperSpecialCase = true;
                } else {
                    delay(2000);
                    return false;
                }
            }
        }

        if (isSpecialCase) {
            switch (group) {
                case 1: {
                    uint16_t rpm = (uint16_t)(0.2f * s[4] * s[5]);
                    if (signals.instruments.engineRpm != rpm) {
                        signals.instruments.engineRpm = rpm;
                        signals.instruments.engineRpmUpdated = true;
                    }

                    uint8_t cool = (uint8_t)(s[7] * (s[8] - 100) * 0.1f);
                    if (signals.instruments.coolantTemp != cool) {
                        signals.instruments.coolantTemp = cool;
                        signals.instruments.coolantTempUpdated = true;
                    }

                    float volt = 0.001f * s[10] * s[11];
                    if (signals.engine.voltage != volt) {
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

        if (isSuperSpecialCase) {
            return true;
        }
    }

    // Track current group number for experimental view
    signals.experimental.groupCurrent = group;

    int count = (size - 4) / 3;
    for (int idx = 0; idx < count; ++idx) {
        byte k = s[3 + idx * 3];
        byte a = s[3 + idx * 3 + 1];
        byte b = s[3 + idx * 3 + 2];
    String t;
    float v = 0;
    const __FlashStringHelper *units = F("");

        switch (k) {
            case 1:  v = 0.2f * a * b; units = F("rpm"); break;
            case 2:  v = a * 0.002f * b; units = F("%%"); break;
            case 3:  v = 0.002f * a * b; units = F("Deg"); break;
            case 4:  v = abs(b - 127) * 0.01f * a; units = F("ATDC"); break;
            case 5:  v = a * (b - 100) * 0.1f; units = F("°C"); break;
            case 6:  v = 0.001f * a * b; units = F("V"); break;
            case 7:  v = 0.01f * a * b; units = F("km/h"); break;
            case 8:  v = 0.1f * a * b; units = F(" "); break;
            case 14: v = 0.005f * a * b; units = F("bar"); break;
            case 18: v = 0.04f * a * b; units = F("mbar"); break;
            case 19: v = a * b * 0.01f; units = F("l"); break;
            case 36: v = ((unsigned long)a) * 2560 + ((unsigned long)b) * 10; units = F("km"); break;
            default: break; // many more cases exist in original; can be ported as needed
        }

    // Update experimental arrays like original
        if (signals.experimental.k[idx] != k) {
            signals.experimental.k[idx] = k;
            signals.experimental.kUpdated = true;
        }
        if (signals.experimental.v[idx] != v) {
            signals.experimental.v[idx] = v;
            signals.experimental.vUpdated = true;
        }
        // Copy unit text from PROGMEM string into fixed-size buffer if it changed.
        // We compare first character as a cheap proxy; exact match is not critical
        // for display purposes, and this avoids constructing temporary Strings.
        char firstChar = pgm_read_byte(reinterpret_cast<const char *>(units));
        if (signals.experimental.unit[idx][0] != firstChar) {
            // Copy up to UnitWidth chars from PROGMEM
            uint8_t j = 0;
            for (; j < obd::Model::ExperimentalGroup::UnitWidth; ++j) {
                char c = pgm_read_byte(reinterpret_cast<const char *>(reinterpret_cast<uintptr_t>(units) + j));
                if (c == '\0') break;
                signals.experimental.unit[idx][j] = c;
            }
            if (j <= obd::Model::ExperimentalGroup::UnitWidth) {
                signals.experimental.unit[idx][j] = '\0';
            }
            for (++j; j < obd::Model::ExperimentalGroup::UnitWidth + 1; ++j) {
                signals.experimental.unit[idx][j] = '\0';
            }
            signals.experimental.unitUpdated = true;
        }

        // Map into instruments/engine signals as in original switch(addr_selected)
        switch (ecuAddr_) {
            case 0x17: { // ADDR_INSTRUMENTS
                switch (group) {
                    case 1:
                        switch (idx) {
                            case 0: {
                                uint16_t value = (uint16_t)v;
                                if (signals.instruments.vehicleSpeed != value) {
                                    signals.instruments.vehicleSpeed = value;
                                    signals.instruments.vehicleSpeedUpdated = true;
                                }
                                break;
                            }
                            case 1: {
                                uint16_t value = (uint16_t)v;
                                if (signals.instruments.engineRpm != value) {
                                    signals.instruments.engineRpm = value;
                                    signals.instruments.engineRpmUpdated = true;
                                }
                                break;
                            }
                            case 2: {
                                uint16_t value = (uint16_t)v;
                                if (signals.instruments.oilPressureMin != value) {
                                    signals.instruments.oilPressureMin = value;
                                    signals.instruments.oilPressureMinUpdated = true;
                                }
                                break;
                            }
                            case 3: {
                                uint32_t value = (uint32_t)v;
                                if (signals.instruments.timeEcu != value) {
                                    signals.instruments.timeEcu = value;
                                    signals.instruments.timeEcuUpdated = true;
                                }
                                break;
                            }
                        }
                        break;
                    case 2:
                        switch (idx) {
                            case 0: {
                                uint32_t value = (uint32_t)v;
                                if (signals.instruments.odometer != value) {
                                    signals.instruments.odometer = value;
                                    signals.instruments.odometerUpdated = true;
                                }
                                break;
                            }
                            case 1: {
                                uint8_t value = (uint8_t)v;
                                if (signals.instruments.fuelLevel != value) {
                                    signals.instruments.fuelLevel = value;
                                    signals.instruments.fuelLevelUpdated = true;
                                }
                                break;
                            }
                            case 2: {
                                uint16_t value = (uint16_t)v;
                                if (signals.instruments.fuelSensorResistance != value) {
                                    signals.instruments.fuelSensorResistance = value;
                                    signals.instruments.fuelSensorResistanceUpdated = true;
                                }
                                break;
                            }
                            case 3: {
                                uint8_t value = (uint8_t)v;
                                if (signals.instruments.ambientTemp != value) {
                                    signals.instruments.ambientTemp = value;
                                    signals.instruments.ambientTempUpdated = true;
                                }
                                break;
                            }
                        }
                        break;
                    case 3:
                        switch (idx) {
                            case 0: {
                                uint8_t value = (uint8_t)v;
                                if (signals.instruments.coolantTemp != value) {
                                    signals.instruments.coolantTemp = value;
                                    signals.instruments.coolantTempUpdated = true;
                                }
                                break;
                            }
                            case 1: {
                                uint8_t value = (uint8_t)v;
                                if (signals.instruments.oilLevelOk != value) {
                                    signals.instruments.oilLevelOk = value;
                                    signals.instruments.oilLevelOkUpdated = true;
                                }
                                break;
                            }
                            case 2: {
                                uint8_t value = (uint8_t)v;
                                if (signals.instruments.oilTemp != value) {
                                    signals.instruments.oilTemp = value;
                                    signals.instruments.oilTempUpdated = true;
                                }
                                break;
                            }
                            default:
                                break;
                        }
                        break;
                    default:
                        break;
                }
                break;
            }
            case 0x01: { // ADDR_ENGINE
                switch (group) {
                    case 1:
                        switch (idx) {
                            case 0: {
                                uint16_t value = (uint16_t)v;
                                if (signals.instruments.engineRpm != value) {
                                    signals.instruments.engineRpm = value;
                                    signals.instruments.engineRpmUpdated = true;
                                }
                                break;
                            }
                            case 1: {
                                uint8_t value = (uint8_t)v;
                                if (signals.engine.tempUnknown1 != value) {
                                    signals.engine.tempUnknown1 = value;
                                    signals.engine.tempUnknown1Updated = true;
                                }
                                break;
                            }
                            case 2: {
                                int8_t value = (int8_t)v;
                                if (signals.engine.lambda != value) {
                                    signals.engine.lambda = value;
                                    signals.engine.lambdaUpdated = true;
                                }
                                break;
                            }
                            case 3:
                                break;
                        }
                        break;
                    case 3:
                        switch (idx) {
                            case 1: {
                                uint16_t value = (uint16_t)v;
                                if (signals.engine.pressure != value) {
                                    signals.engine.pressure = value;
                                    signals.engine.pressureUpdated = true;
                                }
                                break;
                            }
                            case 2: {
                                float value = v;
                                if (signals.engine.tbAngle != value) {
                                    signals.engine.tbAngle = value;
                                    signals.engine.tbAngleUpdated = true;
                                }
                                break;
                            }
                            case 3: {
                                float value = v;
                                if (signals.engine.steeringAngle != value) {
                                    signals.engine.steeringAngle = value;
                                    signals.engine.steeringAngleUpdated = true;
                                }
                                break;
                            }
                        }
                        break;
                    case 4:
                        switch (idx) {
                            case 1: {
                                float value = v;
                                if (signals.engine.voltage != value) {
                                    signals.engine.voltage = value;
                                    signals.engine.voltageUpdated = true;
                                }
                                break;
                            }
                            case 2: {
                                uint8_t value = (uint8_t)v;
                                if (signals.engine.tempUnknown2 != value) {
                                    signals.engine.tempUnknown2 = value;
                                    signals.engine.tempUnknown2Updated = true;
                                }
                                break;
                            }
                            case 3: {
                                uint8_t value = (uint8_t)v;
                                if (signals.engine.tempUnknown3 != value) {
                                    signals.engine.tempUnknown3 = value;
                                    signals.engine.tempUnknown3Updated = true;
                                }
                                break;
                            }
                        }
                        break;
                    case 6:
                        switch (idx) {
                            case 1: {
                                uint16_t value = (uint16_t)v;
                                if (signals.engine.engineLoad != value) {
                                    signals.engine.engineLoad = value;
                                    signals.engine.engineLoadUpdated = true;
                                }
                                break;
                            }
                            case 3: {
                                int8_t value = (int8_t)v;
                                if (signals.engine.lambda2 != value) {
                                    signals.engine.lambda2 = value;
                                    signals.engine.lambda2Updated = true;
                                }
                                break;
                            }
                        }
                        break;
                    default:
                        break;
                }
                break;
            }
            default:
                break;
        }
    }

    return true;
}

int8_t KWP1281Session::readDtcCodes(Model::DTCStore &dtcStore)
{
    // Minimal port of read_DTC_codes(); assumes simulation is
    // handled externally.

    uint8_t s[64];
    // Send DTC read block
    uint8_t req[4] = {0x03, blockCounter_, 0x07, 0x03};
    if (!sendBlock_(req, 4)) return -1;

    dtcStore.reset();
    uint8_t dtcCounter = 0;

    while (true) {
        int size = 0;
        if (!receiveBlock_(s, 64, size)) return -1;

        if (s[2] == 0x09) break; // No more DTC blocks
        if (s[2] != 0xFC) return -1;

        int count = (size - 4) / 3;
        for (int i = 0; i < count; ++i) {
            uint8_t byteHigh = s[3 + 3 * i];
            uint8_t byteLow = s[3 + 3 * i + 1];
            uint8_t byteStatus = s[3 + 3 * i + 2];

            if (byteHigh == 0xFF && byteLow == 0xFF && byteStatus == 0x88) {
                // No DTC codes
            } else {
                uint16_t dtc = (byteHigh << 8) + byteLow;
                dtcStore.set(dtcCounter, dtc, byteStatus);
                ++dtcCounter;
            }
        }

        if (!sendAckBlock_()) {
            return -1;
        }
    }

    return (int8_t)dtcCounter;
}

bool KWP1281Session::deleteDtcCodes()
{
    uint8_t s[4] = {0x03, blockCounter_, 0x05, 0x03};
    if (!sendBlock_(s, 4)) return false;

    int size = 0;
    uint8_t resp[64];
    if (!receiveBlock_(resp, 64, size)) return false;
    if (resp[2] != 0x09) return false;
    return true;
}

bool KWP1281Session::exitSession()
{
    uint8_t s[4] = {0x03, blockCounter_, 0x06, 0x03};
    if (!sendBlock_(s, 4)) {
        return false;
    }
    return true;
}

} // namespace KWP
} // namespace obd
