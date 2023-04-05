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

    IN14Tube** in14Tubes;
    IN19Tube** in19Tubes;
    uint8_t numIN14;
    uint8_t numIN19;

    uint16_t numDataTable[4] = {0, 0, 0, 0};
    uint16_t scDataTable[2] = {0, 0};

public:
    TubeDriver(ShiftRegisterDriver* numRegisters, IN14Tube** tubesNUM, uint8_t numNUMTubes, ShiftRegisterDriver* scRegisters, IN19Tube** tubesSC, uint8_t numSCTubes);
    void showNUM();
    void showSC();
    void show();
    void clear();
    void setNumber(int tube, int number);
    void enableNumber(int tube, int number);
    void disableNumber(int tube, int number);
    void setCharacter(int tube, char character);
    void setVisibility(bool visible);
    void setVisibilityNUM(bool visible);
    void setVisibilitySC(bool visible);
};


#endif //NIXIECLOCK_TUBEDRIVER_H
