//
// Created by emilr on 2023-03-24.
//

#include "ShiftRegisterDriver.h"
#include "tubes/IN14Tube.h"
#include "tubes/IN19Tube.h"

#ifndef NIXIECLOCK_TUBEDRIVER_H
#define NIXIECLOCK_TUBEDRIVER_H


class TubeDriver {
private:
    ShiftRegisterDriver* ShiftRegisterNUM;
    ShiftRegisterDriver* ShiftRegisterSC;

    IN14Tube* in14Tubes;
    IN19Tube* in19Tubes;
    uint8_t numIN14;
    uint8_t numIN19;

public:
    TubeDriver(ShiftRegisterDriver* numRegisters, IN14Tube* tubesNUM, uint8_t numNUMTubes, ShiftRegisterDriver* scRegisters, IN19Tube* tubesSC, uint8_t numSCTubes);
    void show();
    void clear();
    void setNumber(int tube, int number);
    void setCharacter(int tube, char character);
    void setVisibility(bool visible);
};


#endif //NIXIECLOCK_TUBEDRIVER_H
