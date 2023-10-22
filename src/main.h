//
// Created by emilr on 2023-03-24.
//
#define NUM_LEDS 6

#include <Arduino.h>
#include "./../.pio/libdeps/esp32dev/NeoPixelBus/src/NeoPixelBus.h"
#include "ShiftRegisterDriver.h"
#include "TubeDriver.h"
#include "WiFi.h"
#include <./../.pio/libdeps/esp32dev/AsyncTCP/src/AsyncTCP.h>
#include "./../.pio/libdeps/esp32dev/ESP Async WebServer/src/ESPAsyncWebServer.h"
#include "WiFiPass.h"
#include <ESPmDNS.h>
#include "SPIFFS.h"
#include <esp_task_wdt.h>
#include "time.h"
#include <Time.h>
#include <HTTPClient.h>
#include "TimeLib.h"
#include <ArduinoJSON.h>


#include <Update.h>

#ifndef NIXIECLOCK_MAIN_H
#define NIXIECLOCK_MAIN_H

#define U_PART U_SPIFFS
size_t content_len;

TaskHandle_t normalTubeRunnerHandle;


#define BUZZER_PIN 21
#define STATUS_LED 2
#define PSU_EN_PIN 19
#define COLON_1_PIN 13
#define COLON_2_PIN 27
#define HV_SENSE_PIN 36

#define LEFT_COMMA 10
#define RIGHT_COMMA 11

#define SC_PERCENT 0
#define SC_M 1
#define SC_P 2
#define SC_m 3
#define SC_DEG_C 4
#define SC_MY 5
#define SC_n 6
#define SC_KELVIN 7

#define SC_MINUS 8
#define SC_GT 9
#define SC_PLUS 10
#define SC_dB 11
#define SC_LT 12
#define SC_SINE 13
#define SC_DIVISION 14
#define SC_PI 15


#define TEMP_INTERVAL 5

AsyncWebServer server(80);

const char file_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta charset="UTF-8">
</head>
<body>
  <p><h1>File Upload</h1></p>
  <p>Free Storage: %FREESPIFFS% | Used Storage: %USEDSPIFFS% | Total Storage: %TOTALSPIFFS%</p>
  <form method="POST" action="/upload" enctype="multipart/form-data"><input type="file" name="data"/><input type="submit" name="upload" value="Upload File" title="Upload File"></form>
    <a href="/">Tillbaka</a>
  <p>After clicking upload it will take some time for the file to firstly upload and then be written to SPIFFS, there is no indicator that the upload began.  Please be patient.</p>
  <p>Once uploaded the page will refresh and the newly uploaded file will appear in the file list.</p>
  <p>If a file does not appear, it will be because the file was too big, or had unusual characters in the file name (like spaces).</p>
  <p>You can see the progress of the upload by watching the serial output.</p>
  <p>%FILELIST%</p>

    <h2>Upload BIN</h2>
    <form method="POST" action="/update" enctype="multipart/form-data"><input type="file" name="data"/><input type="submit" name="upload" value="Upload BIN" title="Upload BIN"></form>
</body>
</html>
)rawliteral";

NeoPixelBus<NeoRgbFeature, NeoEsp32I2s0800KbpsInvertedMethod> whiteStrip(NUM_LEDS, 22);
NeoPixelBus<NeoGrb48Feature, NeoEsp32Rmt0800KbpsInvertedMethod> RGBStrip(NUM_LEDS, 23);

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
