//
// Created by emilr on 2023-03-24.
//

#include "ShiftRegisterDriver.h"

ShiftRegisterDriver::ShiftRegisterDriver(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, uint8_t enablePin) {
    this->dataPin = dataPin;
    this->clockPin = clockPin;
    this->latchPin = latchPin;
    this->enablePin = enablePin;
    setFrequency(1000);
}

void ShiftRegisterDriver::begin() {
    pinMode(dataPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(latchPin, OUTPUT);
    pinMode(enablePin, OUTPUT);

    digitalWrite(dataPin, HIGH); //Output is inverted
    digitalWrite(clockPin, HIGH);
    digitalWrite(latchPin, HIGH);
    digitalWrite(enablePin, LOW);

    Serial.printf("Shift register on data pin %i is ready\n", dataPin);
}

void ShiftRegisterDriver::setFrequency(uint32_t freq) {
    this->frequency = freq;
    this->freqDelay4 = 1000000 / freq / 4;
}

void ShiftRegisterDriver::sendData(uint8_t *data, unsigned int len) {
    for (int i = 0; i < len; i++) {
        for (int bit = 7; bit >= 0; bit--) {
            digitalWrite(dataPin, !((data[i] >> bit) & 0b1));
            delayMicroseconds(freqDelay4);
            digitalWrite(clockPin, LOW);
            delayMicroseconds(freqDelay4);
            delayMicroseconds(freqDelay4);
            digitalWrite(clockPin, HIGH);
            delayMicroseconds(freqDelay4);
        }
    }

    delayMicroseconds(freqDelay4);
    digitalWrite(latchPin, LOW);
    delayMicroseconds(freqDelay4 * 4);
    digitalWrite(latchPin, HIGH);
}

void ShiftRegisterDriver::enable(bool enable) {
    digitalWrite(enablePin, enable);
}