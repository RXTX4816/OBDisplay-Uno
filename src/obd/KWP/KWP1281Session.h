#pragma once

#include <Arduino.h>
#include "../../NewSoftwareSerial.h"
#include "../Model/OBDSignals.h"
#include "../Model/DTCStore.h"

namespace obd {
namespace KWP {

enum class Mode : uint8_t {
    Ack = 0,
    ReadSensors = 1,
    ReadGroup = 2
};

class KWP1281Session {
public:
    explicit KWP1281Session(NewSoftwareSerial &serial);

    void setConfig(uint16_t baudRate, uint8_t ecuAddr);

    bool connectToEcu(bool simulationMode,
                      bool autoSetup,
                      uint16_t &baudRate,
                      uint8_t &addrSelected);

    void disconnect();

    bool keepAlive();
    bool readSensorsGroup(uint8_t group, Model::OBDSignals &signals);
    int8_t readDtcCodes(Model::DTCStore &dtcStore);
    bool deleteDtcCodes();
    bool exitSession();

private:
    NewSoftwareSerial &obd_;
    uint16_t baudRate_;
    uint8_t ecuAddr_;
    uint8_t blockCounter_;
    bool connected_;
    bool comError_;
    uint16_t timeoutMs_;

    void incrementBlockCounter_();
    void writeByte_(uint8_t data);
    int16_t readByte_();
    bool sendBlock_(uint8_t *data, int size);
    bool receiveBlock_(uint8_t *buffer, int maxSize, int &size,
                       int source = -1, bool initializationPhase = false);
    bool sendAckBlock_();
    bool receiveAckBlock_();
    bool readConnectBlocks_(bool initializationPhase);
    bool perform5BaudInit_();
};

} // namespace KWP
} // namespace obd
