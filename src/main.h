//
// Created by emilr on 2023-03-24.
//
#define NUM_LEDS 6

#include <Arduino.h>
#include "./../.pio/libdeps/esp32dev/NeoPixelBus/src/NeoPixelBus.h"


#define BUZZER_PIN 21
#define STATUS_LED 2
#define PSU_EN_PIN 19
#define COLON_1_PIN 13
#define COLON_2_PIN 27
#define HV_SENSE_PIN 36

NeoPixelBus<NeoRgbFeature, NeoEsp32Rmt0800KbpsInvertedMethod> whiteStrip(NUM_LEDS, 22);
NeoPixelBus<NeoGrb48Feature, NeoEsp32I2s0800KbpsInvertedMethod> RGBStrip(NUM_LEDS, 23);

#ifndef NIXIECLOCK_MAIN_H
#define NIXIECLOCK_MAIN_H

#endif //NIXIECLOCK_MAIN_H
