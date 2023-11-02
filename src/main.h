//
// Created by emilr on 2023-03-24.
//

#include <Arduino.h>
#include "./../.pio/libdeps/esp32dev/NeoPixelBus/src/NeoPixelBus.h"
#include "TubeDriver.h"
#include "WiFi.h"
#include "WiFiPass.h"
#include "includes.h"
#include "TubeAnimator.h"
#include "ServerRunner.h"


#ifndef NIXIECLOCK_MAIN_H
#define NIXIECLOCK_MAIN_H

TaskHandle_t normalTubeRunnerHandle;

AsyncWebServer server(80);

ServerRunner serverRunner(&server);
TubeAnimator tubeAnim;

NeoPixelBus<NeoRgbFeature, NeoEsp32I2s0800KbpsInvertedMethod> whiteStrip(NUM_LEDS, 22);
NeoPixelBus<NeoGrb48Feature, NeoEsp32Rmt0800KbpsInvertedMethod> RGBStrip(NUM_LEDS, 23);

Timezone timezone;

int lookup1[] = {38, 45, 46, 47, 43, 42, 41, 40, 36, 37, 44, 39};
auto tube1 = new IN14Tube(lookup1);

int lookup2[] = {25, 33, 34, 35, 31, 30, 29, 28, 27, 26, 32, 24};
auto tube2 = new IN14Tube(lookup2);

int lookup3[] = {12, 21, 22, 23, 19, 18, 17, 16, 15, 14, 20, 13};
auto tube3 = new IN14Tube(lookup3);

int lookup4[] = {2, 9, 10, 11, 7, 6, 5, 4, 0, 1, 8, 3};
auto tube4 = new IN14Tube(lookup4);

static IN14Tube* tubeTable[] = {tube1, tube2, tube3, tube4};

ShiftRegisterDriver ShiftRegisterNUM(26, 32, 33, 25);
ShiftRegisterDriver ShiftRegisterSC(18, 17, 16, 4);
TubeDriver tubes(&ShiftRegisterNUM, tubeTable, 4, &ShiftRegisterSC, {}, 2);

bool dayENTable[24] = {
        false, false, false, false, false, false, false, true, true, true, false, false,
        false, false, true, true, true, true, true, true, true, true, true, false
};

bool animENTable[24] = {
        false, false, false, false, false, false, false, false, false, true, true, true,
        true, true, true, true, true, true, true, true, true, true, true, true
};


#endif //NIXIECLOCK_MAIN_H
