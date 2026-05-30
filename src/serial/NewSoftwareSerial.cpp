// SPDX-License-Identifier: LGPL-2.1-or-later
// Stripped NewSoftwareSerial: 16 MHz only, 5 KWP baud rates,
// no Stream/Print, no inverse logic, single instance.
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <Arduino.h>
#include "NewSoftwareSerial.h"

typedef struct
{
    long baud;
    unsigned short rx_delay_centering;
    unsigned short rx_delay_intrabit;
    unsigned short rx_delay_stopbit;
    unsigned short tx_delay;
} DELAY_TABLE;

// 16 MHz — only the 5 baud rates needed for KWP-1281 VAG ECUs.
static const DELAY_TABLE PROGMEM table[] = {
    // baud   rxcenter  rxintra  rxstop   tx
    {10400, 106, 218, 218, 215}, {9600, 114, 236, 236, 233},    {4800, 233, 474, 474, 471},
    {2400, 471, 950, 950, 947},  {1200, 947, 1902, 1902, 1899},
};

const int XMIT_START_ADJUSTMENT = 5;

// Static member definitions
char NewSoftwareSerial::_receive_buffer[_SS_MAX_RX_BUFF];
volatile uint8_t NewSoftwareSerial::_receive_buffer_tail = 0;
volatile uint8_t NewSoftwareSerial::_receive_buffer_head = 0;
NewSoftwareSerial* NewSoftwareSerial::_instance = nullptr;

inline void NewSoftwareSerial::tunedDelay(uint16_t delay)
{
    uint8_t tmp = 0;
    asm volatile("sbiw    %0, 0x01 \n\t"
                 "ldi %1, 0xFF \n\t"
                 "cpi %A0, 0xFF \n\t"
                 "cpc %B0, %1 \n\t"
                 "brne .-10 \n\t"
                 : "+r"(delay), "+a"(tmp)
                 : "0"(delay));
}

void NewSoftwareSerial::recv()
{
    // Only sample if start bit is present (RX line low)
    if (!rx_pin_read())
    {
        uint8_t d = 0;
        tunedDelay(_rx_delay_centering);

        for (uint8_t i = 0x1; i; i <<= 1)
        {
            tunedDelay(_rx_delay_intrabit);
            uint8_t noti = ~i;
            if (rx_pin_read())
                d |= i;
            else
                d &= noti;
        }

        tunedDelay(_rx_delay_stopbit);

        if ((_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF != _receive_buffer_head)
        {
            _receive_buffer[_receive_buffer_tail] = d;
            _receive_buffer_tail = (_receive_buffer_tail + 1) % _SS_MAX_RX_BUFF;
        }
    }
}

void NewSoftwareSerial::tx_pin_write(uint8_t pin_state)
{
    if (pin_state == LOW)
        *_transmitPortRegister &= ~_transmitBitMask;
    else
        *_transmitPortRegister |= _transmitBitMask;
}

uint8_t NewSoftwareSerial::rx_pin_read()
{
    return *_receivePortRegister & _receiveBitMask;
}

inline void NewSoftwareSerial::handle_interrupt()
{
    if (_instance)
        _instance->recv();
}

#if defined(PCINT0_vect)
ISR(PCINT0_vect)
{
    NewSoftwareSerial::handle_interrupt();
}
#endif
#if defined(PCINT1_vect)
ISR(PCINT1_vect)
{
    NewSoftwareSerial::handle_interrupt();
}
#endif
#if defined(PCINT2_vect)
ISR(PCINT2_vect)
{
    NewSoftwareSerial::handle_interrupt();
}
#endif
#if defined(PCINT3_vect)
ISR(PCINT3_vect)
{
    NewSoftwareSerial::handle_interrupt();
}
#endif

NewSoftwareSerial::NewSoftwareSerial(uint8_t receivePin, uint8_t transmitPin)
    : _receivePin(0), _receiveBitMask(0), _receivePortRegister(nullptr), _transmitBitMask(0),
      _transmitPortRegister(nullptr), _rx_delay_centering(0), _rx_delay_intrabit(0),
      _rx_delay_stopbit(0), _tx_delay(0)
{
    setTX(transmitPin);
    setRX(receivePin);
}

NewSoftwareSerial::~NewSoftwareSerial()
{
    end();
}

void NewSoftwareSerial::setTX(uint8_t tx)
{
    pinMode(tx, OUTPUT);
    digitalWrite(tx, HIGH);
    _transmitBitMask = digitalPinToBitMask(tx);
    _transmitPortRegister = portOutputRegister(digitalPinToPort(tx));
}

void NewSoftwareSerial::setRX(uint8_t rx)
{
    pinMode(rx, INPUT);
    digitalWrite(rx, HIGH); // enable pullup
    _receivePin = rx;
    _receiveBitMask = digitalPinToBitMask(rx);
    _receivePortRegister = portInputRegister(digitalPinToPort(rx));
}

void NewSoftwareSerial::begin(long speed)
{
    _rx_delay_centering = _rx_delay_intrabit = _rx_delay_stopbit = _tx_delay = 0;

    for (uint8_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
    {
        long baud = pgm_read_dword(&table[i].baud);
        if (baud == speed)
        {
            _rx_delay_centering = pgm_read_word(&table[i].rx_delay_centering);
            _rx_delay_intrabit = pgm_read_word(&table[i].rx_delay_intrabit);
            _rx_delay_stopbit = pgm_read_word(&table[i].rx_delay_stopbit);
            _tx_delay = pgm_read_word(&table[i].tx_delay);
            break;
        }
    }

    if (_rx_delay_stopbit)
    {
        if (digitalPinToPCICR(_receivePin))
        {
            *digitalPinToPCICR(_receivePin) |= _BV(digitalPinToPCICRbit(_receivePin));
            *digitalPinToPCMSK(_receivePin) |= _BV(digitalPinToPCMSKbit(_receivePin));
        }
        tunedDelay(_tx_delay);
    }

    _receive_buffer_head = _receive_buffer_tail = 0;
    _instance = this;
}

void NewSoftwareSerial::end()
{
    if (digitalPinToPCMSK(_receivePin))
        *digitalPinToPCMSK(_receivePin) &= ~_BV(digitalPinToPCMSKbit(_receivePin));
}

// cppcheck-suppress functionStatic
int NewSoftwareSerial::read()
{
    if (_receive_buffer_head == _receive_buffer_tail)
        return -1;
    uint8_t d = _receive_buffer[_receive_buffer_head];
    _receive_buffer_head = (_receive_buffer_head + 1) % _SS_MAX_RX_BUFF;
    return d;
}

// cppcheck-suppress functionStatic
int NewSoftwareSerial::available()
{
    return (_receive_buffer_tail + _SS_MAX_RX_BUFF - _receive_buffer_head) % _SS_MAX_RX_BUFF;
}

size_t NewSoftwareSerial::write(uint8_t b)
{
    if (_tx_delay == 0)
        return 0;

    uint8_t oldSREG = SREG;
    cli();

    tx_pin_write(LOW); // start bit
    tunedDelay(_tx_delay + XMIT_START_ADJUSTMENT);

    for (byte mask = 0x01; mask; mask <<= 1)
    {
        if (b & mask)
            tx_pin_write(HIGH);
        else
            tx_pin_write(LOW);
        tunedDelay(_tx_delay);
    }

    tx_pin_write(HIGH); // stop bit / idle
    SREG = oldSREG;
    tunedDelay(_tx_delay);

    return 1;
}
