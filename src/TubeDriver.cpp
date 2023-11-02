//
// Created by emilr on 2023-03-24.
//

#include "TubeDriver.h"

TubeDriver::TubeDriver(ShiftRegisterDriver *numRegisters, IN14Tube **tubesNUM, uint8_t numNUMTubes,
                       ShiftRegisterDriver *scRegisters, IN19Tube **tubesSC, uint8_t numSCTubes) {
    ShiftRegisterSC = scRegisters;
    ShiftRegisterNUM = numRegisters;

    numIN14 = numNUMTubes;
    numIN19 = numSCTubes;
    in14Tubes = tubesNUM;
    in19Tubes = tubesSC;
}

void TubeDriver::setNumber(int tube, int number) {
    numDataTable[tube] = 0b1 << in14Tubes[tube]->lookup[number] % 12;
    //Serial.printf("%u\n", numDataTable[tube]);
}

void TubeDriver::enableNumber(int tube, int number) {
    numDataTable[tube] |= 0b1 << in14Tubes[tube]->lookup[number] % 12;
}

void TubeDriver::disableNumber(int tube, int number) {
    numDataTable[tube] &= ~(0b1 << in14Tubes[tube]->lookup[number]) % 12;
}

void TubeDriver::setCharacter(int tube, char character) {
}

void TubeDriver::setCharacter(int tube, int character) {
    scDataTable[tube] = 0b1 << (character % 8);
}

void TubeDriver::clear() {
    for (int i = 0; i < numIN14; i++) {
        numDataTable[i] = 0x00;
    }
    for (int i = 0; i < numIN19; i++) {
        scDataTable[i] = 0x00;
    }
}

void TubeDriver::clearNUMTo(int number) {
    for (int i = 0; i < numIN14; i++) {
        setNumber(i, number);
    }
}


void TubeDriver::setVisibility(bool visible) {
    setVisibilityNUM(visible);
    setVisibilitySC(visible);
}

void TubeDriver::setVisibilityNUM(bool visible) {
    ShiftRegisterNUM->enable(visible);
}

void TubeDriver::setVisibilitySC(bool visible) {
    ShiftRegisterSC->enable(visible);
}


void TubeDriver::show() {
    showNUM();
    showSC();
}

void TubeDriver::showSC() {
    uint8_t shiftRegisterTable[2] = {0, 0};
    shiftRegisterTable[0] = scDataTable[1];
    shiftRegisterTable[1] = scDataTable[0];
    ShiftRegisterSC->sendData(shiftRegisterTable, 2);
}

void TubeDriver::showNUM() {
    uint8_t shiftRegisterTable[6] = {0, 0, 0, 0, 0, 0};
    /*shiftRegisterTable[0] = numDataTable[0] & 0xff;
    shiftRegisterTable[1] = (numDataTable[0] >> 8 || numDataTable[1] << 4) & 0xff;
    shiftRegisterTable[2] = (numDataTable[1] >> 4) & 0xff;
    shiftRegisterTable[3] = numDataTable[2] & 0xff;
    shiftRegisterTable[4] = (numDataTable[2] >> 8 || numDataTable[3] << 4) & 0xff;
    shiftRegisterTable[5] = (numDataTable[3] >> 4) & 0xff;*/
    for (int i = 3; i >= 0; i--) {
        for (int8_t aBit = 11; aBit >= 0; aBit--) {
            //Serial.write(bitRead(numDataTable[i], aBit) ? '1' : '0');
            int index = (i*12 + aBit) / 8;
            int shift = (i*12 + aBit) % 8;
            shiftRegisterTable[index] |= bitRead(numDataTable[3-i], aBit) << shift;
        }
    }
    //Serial.println("B");

    ShiftRegisterNUM->sendData(shiftRegisterTable, 6);
}


