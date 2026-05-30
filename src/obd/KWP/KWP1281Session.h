// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <Arduino.h>
#include "../../serial/NewSoftwareSerial.h"
#include "../Model/OBDSignals.h"
#include "../Model/DTCStore.h"

namespace obd
{
namespace KWP
{

enum class Mode : uint8_t
{
    Ack = 0,
    ReadSensors = 1,
    ReadGroup = 2
};

class KWP1281Session
{
  public:
    KWP1281Session(NewSoftwareSerial& serial, uint8_t txPin);

    void setConfig(uint16_t baudRate, uint8_t ecuAddr);

    bool connectToEcu(bool simulationMode, bool autoSetup, uint16_t& baudRate,
                      const uint8_t& addrSelected);

    void disconnect();

    bool keepAlive();
    bool readSensorsGroup(uint8_t group, Model::OBDSignals& signals);
    int8_t readDtcCodes(Model::DTCStore& dtcStore);
    bool deleteDtcCodes();
    bool exitSession();

    uint8_t getBlockCounter() const { return blockCounter_; }
    uint8_t lastConnectError() const { return lastConnectError_; }

    uint8_t ecuLineCount() const { return ecuLineCount_; }
    const char* ecuLine(uint8_t i) const { return ecuLines_[i]; }
    const char (*ecuLinesData() const)[11] { return ecuLines_; }

  private:
    NewSoftwareSerial& obd_;
    uint8_t txPin_;
    uint16_t baudRate_;
    uint8_t ecuAddr_;
    uint8_t blockCounter_;
    bool connected_;
    bool comError_;
    uint16_t timeoutMs_;
    uint8_t lastConnectError_;

    char ecuLines_[6][11];
    uint8_t ecuLineCount_;

    void incrementBlockCounter_();
    void writeByte_(uint8_t data);
    int16_t readByte_();
    bool sendBlock_(const uint8_t* data, int size);
    bool receiveBlock_(uint8_t* buffer, int maxSize, int& size, int source = -1,
                       bool initializationPhase = false);
    bool sendAckBlock_();
    bool receiveAckBlock_();
    bool readConnectBlocks_(bool initializationPhase);
    void captureBlockText_(const uint8_t* s, int size);
    bool perform5BaudInit_();
};

} // namespace KWP
} // namespace obd
