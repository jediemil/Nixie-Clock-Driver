//
// Created by emilr on 2023-03-24.
//

#include "Arduino.h"

#ifndef NIXIECLOCK_SHIFTREGISTERDRIVER_H
#define NIXIECLOCK_SHIFTREGISTERDRIVER_H


class ShiftRegisterDriver {
private:
    uint8_t dataPin{};
    uint8_t clockPin{};
    uint8_t latchPin{};
    uint8_t enablePin{};
    uint32_t frequency{};
    uint32_t freqDelay4{};

public:
    ShiftRegisterDriver(uint8_t dataPin, uint8_t clockPin, uint8_t latchPin, uint8_t enablePin);
    void sendData(uint8_t* data, int len);
    void begin();
    void setFrequency(uint32_t freq);
    void enable(bool enable);
};


#endif //NIXIECLOCK_SHIFTREGISTERDRIVER_H
