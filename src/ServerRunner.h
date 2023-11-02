//
// Created by emilr on 2023-11-02.
//

#ifndef NIXIECLOCK_SERVERRUNNER_H
#define NIXIECLOCK_SERVERRUNNER_H

#include "Arduino.h"
#include <./../.pio/libdeps/esp32dev/AsyncTCP/src/AsyncTCP.h>
#include "./../.pio/libdeps/esp32dev/ESP Async WebServer/src/ESPAsyncWebServer.h"
#include <ESPmDNS.h>
#include "SPIFFS.h"
#include <Update.h>
#include "weather.h"
#include "./../.pio/libdeps/esp32dev/ezTime/src/ezTime.h"
#include <HTTPClient.h>
#include <esp_task_wdt.h>
#include "JsonStreamingParser.h"
#include "JsonListener.h"
#include "WeatherParser.h"

#define U_PART U_SPIFFS

class ServerRunner {
public:
    void startServer();
    void startmDNS();
    void startSPIFFS();

    struct weather getWeather();

    ServerRunner(AsyncWebServer *server);

private:
    void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final);
    static void printProgress(size_t prg, size_t sz);
    static void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);

     static String humanReadableSize(const size_t bytes);
     static String listFiles(bool ishtml);
    static String processor(const String& var);

    AsyncWebServer* server;

    //static size_t content_len;

};


#endif //NIXIECLOCK_SERVERRUNNER_H
