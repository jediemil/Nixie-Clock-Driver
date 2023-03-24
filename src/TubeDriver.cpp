//
// Created by emilr on 2023-03-24.
//

#include "TubeDriver.h"

TubeDriver::TubeDriver(ShiftRegisterDriver* numRegisters, IN14Tube* tubesNUM, uint8_t numNUMTubes, ShiftRegisterDriver* scRegisters, IN19Tube* tubesSC, uint8_t numSCTubes) {
    ShiftRegisterSC = scRegisters;
    ShiftRegisterNUM = numRegisters;

    numIN14 = numNUMTubes;
    numIN19 = numSCTubes;
    in14Tubes = tubesNUM;
    in19Tubes = tubesSC;
}

void TubeDriver::setVisibility(bool visible) {

}
