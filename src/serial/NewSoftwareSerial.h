// SPDX-License-Identifier: LGPL-2.1-or-later
// Stripped-down software serial for KWP-1281 K-Line communication.
// Keeps only the 5 baud rates used by VAG ECUs (1200–10400), removes
// Stream/Print inheritance (~400 bytes), inverse logic, multi-instance
// support, and peek/flush.  API compatible with KWP1281Session usage:
// begin(), write(), read(), available().
#pragma once

#include <inttypes.h>

#define _SS_MAX_RX_BUFF 64

class NewSoftwareSerial
{
  private:
    uint8_t _receivePin;
    uint8_t _receiveBitMask;
    volatile uint8_t* _receivePortRegister;
    uint8_t _transmitBitMask;
    volatile uint8_t* _transmitPortRegister;

    uint16_t _rx_delay_centering;
    uint16_t _rx_delay_intrabit;
    uint16_t _rx_delay_stopbit;
    uint16_t _tx_delay;

    static char _receive_buffer[_SS_MAX_RX_BUFF];
    static volatile uint8_t _receive_buffer_tail;
    static volatile uint8_t _receive_buffer_head;
    static NewSoftwareSerial* _instance; // single instance for ISR

    void recv();
    uint8_t rx_pin_read();
    void tx_pin_write(uint8_t pin_state);
    void setTX(uint8_t transmitPin);
    void setRX(uint8_t receivePin);

    static inline void tunedDelay(uint16_t delay);

  public:
    NewSoftwareSerial(uint8_t receivePin, uint8_t transmitPin);
    ~NewSoftwareSerial();

    void begin(long speed);
    void end();
    size_t write(uint8_t byte);
    int read();
    int available();
    bool isListening() { return _instance == this; }
    // cppcheck-suppress functionStatic
    void flush()
    {
        uint8_t s = SREG;
        cli();
        _receive_buffer_head = _receive_buffer_tail = 0;
        SREG = s;
    }

    static inline void handle_interrupt();
};
