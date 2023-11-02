#include "main.h"

bool forceExtraTubeInfo = false;

bool tubesRunning = false;
bool manualTubes = false;

void rgbClearTo(Rgb48Color col) { //NeoPixelBus ClearTo with Rgb48Color is not working
    for (int i = 0; i < NUM_LEDS; i++) {
        RGBStrip.SetPixelColor(i, col);
    }
}

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        rgbClearTo(Rgb48Color(0x2fff, 0, 0));
        RGBStrip.Show();
        delay(1000);
        rgbClearTo(Rgb48Color(0x2fff, 0x0fff, 0));
        RGBStrip.Show();
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}


void addTubeAPI() {
    server.on("/cathode_pois", HTTP_POST,[](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin","*");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type, data");
        response->addHeader("Access-Control-Allow-Methods", "POST, GET");
        response->setCode(200);
        request->send(response);

        esp_task_wdt_reset();

        bool use0 = false;
        bool use1 = false;
        bool use2 = false;
        bool use3 = false;

        if (request->hasHeader("tube0")) {
            use0 = true;
        }
        if (request->hasHeader("tube1")) {
            use1 = true;
        }
        if (request->hasHeader("tube2")) {
            use2 = true;
        }
        if (request->hasHeader("tube3")) {
            use3 = true;
        }

        if (use0 || use1 || use2 || use3) {

            manualTubes = true;
            delay(1000);

            if (tubesRunning) {
                esp_task_wdt_delete(normalTubeRunnerHandle);
                vTaskDelete(normalTubeRunnerHandle);
            }

            tubes.clear();
            tubes.setVisibility(true);
            tubes.show();
            tubeAnim.setColonVis(false);
            delay(1000);

            esp_task_wdt_reset();

            digitalWrite(PSU_EN_PIN, HIGH);
            for (int i = 0; i < 60; i++) {
                for (int j = 0; j < 12; j++) {
                    if (use0) tubes.setNumber(0, j);
                    if (use1) tubes.setNumber(1, j);
                    if (use2) tubes.setNumber(2, j);
                    if (use3) tubes.setNumber(3, j);

                    tubes.showNUM();
                    delay(5000);
                    esp_task_wdt_reset();
                }
                //antiCathodePoisonRoutine(5000, true);
            }
            digitalWrite(PSU_EN_PIN, LOW);
            tubes.clear();
            tubes.show();

            delay(1000);

            ESP.restart();
        }
    });
}

[[noreturn]] void normalTubeLoop(void * parameter) {
    tubes.setVisibilityNUM(true);
    while (true) {
        //struct tm timeStart;
        //getLocalTime(&timeStart);
        bool showTime = true;
        while (showTime) {
            //struct tm timeNow;
            //getLocalTime(&timeNow);

            int hourNow = hour(TIME_NOW);
            tubes.setNumber(0, hourNow/10); //Int division -> remove the last digit
            tubes.setNumber(1, hourNow%10);

            int minuteNow = minute(LAST_READ);
            tubes.setNumber(2, minuteNow/10);
            tubes.setNumber(3, minuteNow%10);

            tubes.showNUM();

            if (minuteNow % TEMP_INTERVAL == 0 || forceExtraTubeInfo) {
                showTime = false;
            }

            uint8_t goal = 30 - second(TIME_NOW)/2 + 1;
            for (int i = 0; i < goal; i++) {
                tubeAnim.setColonVis(false);
                delay(1000);
                tubeAnim.setColonVis(true);
                delay(1000);
                esp_task_wdt_reset();
            }
            if (second(TIME_NOW) > 58) delay(2000); //In case of syncing error
            /*while (!minuteChanged()) { //Should work, but sometimes froze and looped too long or endlessly.
                tubeAnim.setColonVis(false);
                delay(1000);
                tubeAnim.setColonVis(true);
                delay(1000);
                esp_task_wdt_reset();
            }*/
        }
        tubeAnim.showDate();

        //struct tm timeNow;
        //getLocalTime(&timeNow);
        if (minute(TIME_NOW) % 10 < 2 || forceExtraTubeInfo)  { //If the last digit of minute is less than 2, only show weather every 10 minute with the 5 minute date interval.
            tubeAnim.showWeather();
        } else {
            tubeAnim.antiCathodePoisonRoutine(1000, true);
            delay(1000);
        }
    }
}

[[noreturn]] void clockOffLoop(void * args) {
    int lastColor = random(0, 5);
    while (true) {
        //struct tm timeNow;
        //getLocalTime(&timeNow);

        if (animENTable[hour(TIME_NOW)]) {
            int color = random(0, 5);
            uint32_t steps = random(255, 1000);
            double step = 200.0 / steps;

            for (int i = 0; i < steps; i++) {
                uint16_t lastColorValue = 200.0 - (i * step);
                uint16_t newColorValue = i * step;

                uint16_t r = lastColorValue * (lastColor == 0) + newColorValue * (color == 0);
                uint16_t g = lastColorValue * (lastColor == 1) + newColorValue * (color == 1);
                uint16_t b = lastColorValue * (lastColor == 2) + newColorValue * (color == 2);
                uint16_t ww = (lastColorValue * (lastColor == 3) + newColorValue * (color == 3)) >> 2;
                uint16_t nw = (lastColorValue * (lastColor == 4) + newColorValue * (color == 4)) >> 2;
                uint16_t cw = (lastColorValue * (lastColor == 5) + newColorValue * (color == 5)) >> 2;

                rgbClearTo(Rgb48Color(r << 8, g << 8, b << 8));
                //RGBStrip.SetPixelColor(0, Rgb48Color(r, g, b));
                //RGBStrip.SetPixelColor(1, Rgb48Color(0, 255, 0));
                //RGBStrip.SetPixelColor(2, Rgb48Color(0, 0, 255));
                whiteStrip.ClearTo(RgbColor(ww, nw, cw));
                RGBStrip.Show();
                whiteStrip.Show();
                delay(1000);
                esp_task_wdt_reset();
            }
            lastColor = color;

        } else {
            rgbClearTo(RgbColor(0, 0, 0));
            whiteStrip.ClearTo(RgbColor(0, 0, 0));
            RGBStrip.Show();
            whiteStrip.Show();
            for (int i = 0; i < 100; i++) {
                esp_task_wdt_reset();
                delay(5000);
            }
        }
    }
}

void startTubes() {
    whiteStrip.ClearTo(RgbColor(0, 0, 0));
    whiteStrip.Show();
    RGBStrip.ClearTo(RgbColor(0, 0, 0));
    RGBStrip.Show();

    digitalWrite(PSU_EN_PIN, HIGH);
    tubes.setVisibility(true);
    tubeAnim.antiCathodePoisonRoutine(250, true);
    tubes.clear();
    tubes.show();
    tubeAnim.antiCathodePoisonRoutineSC(500, true);
    tubes.clear();
    tubes.show();

    xTaskCreate(
            normalTubeLoop,
            "Tube Normal Task",
            10000,
            NULL,
            1,
            &normalTubeRunnerHandle
    );
    esp_task_wdt_add(normalTubeRunnerHandle);

    tubesRunning = true;
    Serial.println("Tubes started");
}

void shutDownTubes() {
    if (!manualTubes) {
        digitalWrite(PSU_EN_PIN, LOW);
        tubes.clear();
        tubes.show();
        tubes.setVisibility(false);

        xTaskCreate(
                clockOffLoop,
                "Tube Normal Task",
                10000,
                NULL,
                1,
                &normalTubeRunnerHandle
        );
        esp_task_wdt_add(normalTubeRunnerHandle);
    }
    tubesRunning = false;
}

void setup() {
    Serial.begin(115200);

    Serial.println("Boot");
    Serial.println("Attaching pins");

    pinMode(STATUS_LED, OUTPUT);
    pinMode(PSU_EN_PIN, OUTPUT);
    pinMode(COLON_1_PIN, OUTPUT);
    pinMode(COLON_2_PIN, OUTPUT);

    digitalWrite(STATUS_LED, HIGH);
    digitalWrite(PSU_EN_PIN, LOW);
    digitalWrite(COLON_1_PIN, LOW);
    digitalWrite(COLON_2_PIN, LOW);

    ledcSetup(0, 1000, 8);
    ledcAttachPin(BUZZER_PIN, 0);

    Serial.println("Starting LEDs");
    whiteStrip.Begin();
    whiteStrip.SetPixelColor(3, RgbColor(40, 0, 0));
    whiteStrip.SetPixelColor(4, RgbColor(0, 40, 0));
    whiteStrip.SetPixelColor(5, RgbColor(0, 0, 40));
    whiteStrip.Show();

    RGBStrip.Begin();
    //RGBStrip.ClearTo(Rgb48Color(40 << 8, 0, 50 << 8)); // Stupid ClearTo function does not work for 16-bit and I wasted 1 hour of my life.
    RGBStrip.SetPixelColor(0, Rgb48Color(40 << 8, 0, 0));
    RGBStrip.SetPixelColor(1, Rgb48Color(0, 40 << 8, 0));
    RGBStrip.SetPixelColor(2, Rgb48Color(0, 0, 40 << 8));
    RGBStrip.Show();

    Serial.println("LEDs started");
    delay(100);
    Serial.println("Starting Shift Registers");

    ShiftRegisterNUM.begin();
    ShiftRegisterSC.begin();
    tubeAnim.begin(&tubes);

    Serial.println("Shift registers started");
    delay(100);
    Serial.println("Starting SPIFFS");

    serverRunner.startSPIFFS();

    Serial.println("SPIFFS started");
    delay(100);
    Serial.println("Connecting WiFi");

    whiteStrip.ClearTo(RgbColor(0, 0, 0));
    whiteStrip.Show();

    connectWiFi();

    Serial.println("WiFi connected");
    delay(100);
    Serial.println("Starting mDNS");

    serverRunner.startmDNS();

    Serial.println("mDNS responder started");
    delay(100);
    Serial.println("Starting WebServer");

    serverRunner.startServer();
    addTubeAPI();
    server.begin();

    //configTime(3600, 3600, "pool.ntp.org");
    waitForSync();

    timezone.setLocation(F("Europe/Berlin"));
    timezone.setPosix("CET-1CEST,M3.5.0,M10.5.0/3");
    timezone.setDefault();
    Serial.println(dateTime(RFC850));

    Serial.println("WebServer Started");
    delay(100);
    Serial.println("Setup done");

    digitalWrite(STATUS_LED, LOW);
    ledcWriteTone(0, 1047);
    delay(400);
    digitalWrite(STATUS_LED, HIGH);
    ledcWriteTone(0, 1319);
    delay(400);
    ledcWriteTone(0, 1568);
    delay(400);
    ledcWrite(0, 0);
    digitalWrite(STATUS_LED, LOW);

    //RGBStrip.ClearTo(Rgb48Color(0, 0, 0));
    RGBStrip.ClearTo(RgbColor(0, 0, 0));
    RGBStrip.Show();

    delay(100);
    Serial.println("Configuring WDT and tube normal setup");

    tubes.clear();
    tubes.show();
    esp_task_wdt_init(8, true); //enable panic so ESP32 restarts, 8 seconds
    esp_task_wdt_add(NULL); //add current thread to WDT watch

    Serial.println("WDT and tube started");

    //struct tm timeNow;
    //getLocalTime(&timeNow);
    uint8_t hourNow = hour(TIME_NOW);
    //Serial.println(hourNow);

    if (dayENTable[hourNow]) {
        startTubes();
    } else {
        shutDownTubes();
    }
}


int loopI = 0;
void loop() {
    delay(2000);
    esp_task_wdt_reset(); // TODO Better cathode poisoning algorithm need to be added, and also resyncing and long cathode poisoning program at night. Also turn the clock off at specified times.
    events();
    // https://www.manula.com/manuals/daliborfarny-com/nixie-tubes/1/en/topic/cathode-poisoning-prevention-routine
    // For Dalibor Farny Tubes use ratio 60 : 0.2 seconds.

    if (WiFi.status() != WL_CONNECTED) {
        Serial.print(millis());
        Serial.println("Reconnecting to WiFi...");
        //WiFi.disconnect();
        WiFi.reconnect();
    }

    uint8_t hourNow = hour(TIME_NOW);

    if (!manualTubes) {
        if (tubesRunning && !dayENTable[hourNow]) {
            esp_task_wdt_delete(normalTubeRunnerHandle);
            vTaskDelete(normalTubeRunnerHandle);

            for (int i = 0; i < 20; i++) {
                tubeAnim.antiCathodePoisonRoutine(2000, true);
            }
            for (int i = 0; i < 10; i++) {
                tubeAnim.antiCathodePoisonRoutineSC(2000, true);
            }

            shutDownTubes();

        } else if (!tubesRunning && dayENTable[hourNow]) {
            esp_task_wdt_delete(normalTubeRunnerHandle);
            vTaskDelete(normalTubeRunnerHandle);

            startTubes();
        }
    }

    //printLocalTime();

    /*for (uint8_t number = 0; number < 12; number++) {
        for (uint8_t tube = 0; tube < 4; tube++) {
            tubes.setNumber(tube, (number + tube)%12);
        }
        tubes.showNUM();
        tubes.setVisibilityNUM(true);
        delay(1000);
    }*/
    if (loopI == 0) {
        Serial.println("Do weather");
        tubeAnim.setWeather(serverRunner.getWeather());
        Serial.println("Weather done");
        //Serial.println("Re-sync time");
        //configTime(3600, 3600, "pool.ntp.org");
        //Serial.println("Time re-synced");
    }


    if (loopI % 10 == 0) {
        uint16_t adc_data = analogRead(HV_SENSE_PIN);
        double voltage = ((adc_data / 4095.0) * 3.3) / 2000.0 * 202000;
        Serial.println(voltage);
    }

    loopI++;
    loopI %= 60 * 60 / 2;
}