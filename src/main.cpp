#include "main.h"
#include "JsonStreamingParser.h"
#include "JsonListener.h"
#include "WeatherParser.h"

float temp = 0;
float windSpeed = 0;
int humidity = 0;
float pressure = 0;
int wSymb2 = 0;

bool forceExtraTubeInfo = false;

bool tubesRunning = false;
bool manualTubes = false;

extern void antiCathodePoisonRoutine(int speed, bool reset);
extern void antiCathodePoisonRoutineSC(int speed, bool reset);
extern void setColonVis(bool visibility);

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
        rgbClearTo(Rgb48Color(255, 0, 0));
        RGBStrip.Show();
        delay(1000);
        rgbClearTo(Rgb48Color(255, 100, 0));
        RGBStrip.Show();
        delay(1000);
    }
    Serial.println(WiFi.localIP());
}

void startmDNS() {
    if (!MDNS.begin("klocka")) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
}

void startSPIFFS() {
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
}

void handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index){
        Serial.println("Update");
        content_len = request->contentLength();
        // if filename includes spiffs, update the spiffs partition
        int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
        if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
            Update.printError(Serial);
        }
    }

    if (Update.write(data, len) != len) {
        Update.printError(Serial);
    }

    if (final) {
        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
        response->addHeader("Refresh", "10");
        response->addHeader("Location", "/");
        request->send(response);
        if (!Update.end(true)){
            Update.printError(Serial);
        } else {
            Serial.println("Update complete");
            Serial.flush();
            ESP.restart();
        }
    }
}

void printProgress(size_t prg, size_t sz) {
    Serial.printf("Progress: %d%%\n", (prg*100)/content_len);
}

void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
    Serial.println(logmessage);

    if (!index) {
        logmessage = "Upload Start: " + String(filename);

        String storeLocation = "/" + filename;

        if (filename.endsWith(".js")) {
            storeLocation = "/js/" + filename;
        } else if (filename.endsWith(".css")) {
            storeLocation = "/css/" + filename;
        } else if (filename.endsWith(".html")) {
            storeLocation = "/" + filename;
        }

        // open the file on first call and store the file handle in the request object
        if (SPIFFS.exists(storeLocation)) {
            SPIFFS.remove(storeLocation);
        }
        request->_tempFile = SPIFFS.open(storeLocation, "w");
        Serial.println(logmessage);
    }

    if (len) {
        // stream the incoming chunk to the opened file
        request->_tempFile.write(data, len);
        logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
        Serial.println(logmessage);
    }

    if (final) {
        logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
        // close the file handle as the upload is now done
        request->_tempFile.close();
        Serial.println(logmessage);
        request->redirect("/");
    }
}

String humanReadableSize(const size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
    else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

String listFiles(bool ishtml) {
    String returnText = "";
    Serial.println("Listing files stored on SD");
    File root = SPIFFS.open("/");
    //Serial.println(root.name());
    File foundfile = root.openNextFile();
    //Serial.println(foundfile.name());
    if (ishtml) {
        returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th><th></th><th></th></tr>";
    }
    while (foundfile) {
        Serial.println(foundfile.name());
        if (ishtml) {
            if (foundfile.isDirectory()) {
                returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
                returnText += "<td><button onclick=\"openDirectory(\'" + String(foundfile.name()) + "\')\">Open directory</button></tr>"; //<button onclick=\"deleteFile(\'" + String(foundfile.name()) + "\')\">Delete directory</button></tr>";
            } else {
                returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td>";
                returnText += "<td><button onclick=\"deleteFile(\'" + String(foundfile.name()) + "\')\">Delete</button></tr>";
            }
        } else {
            returnText += "File: " + String(foundfile.name()) + " Size: " + humanReadableSize(foundfile.size()) + "\n";
        }
        foundfile.close();
        foundfile = root.openNextFile();
    }
    if (ishtml) {
        returnText += "</table>";
    }
    foundfile.close();
    root.close();
    return returnText;
}

String processor(const String& var) {
    if (var == "FILELIST") {
        return listFiles(true);
    }
    if (var == "FREESPIFFS") {
        return humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
    }

    if (var == "USEDSPIFFS") {
        return humanReadableSize(SPIFFS.usedBytes());
    }

    if (var == "TOTALSPIFFS") {
        return humanReadableSize(SPIFFS.totalBytes());
    }

    return String();
}

void startServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", file_html, processor);
    });

    server.serveStatic("/", SPIFFS, "/");

    server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200);
    }, handleUpload);

    server.on("/update", HTTP_POST,
              [](AsyncWebServerRequest *request) {},
              [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                 size_t len, bool final) {
                  handleDoUpdate(request, filename, index, data, len, final);
              });
    Update.onProgress(printProgress);

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
            setColonVis(false);
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

    server.on("/reset", HTTP_POST,[](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin","*");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type, data");
        response->addHeader("Access-Control-Allow-Methods", "POST, GET");
        response->setCode(200);
        request->send(response);

        delay(500);

        ESP.restart();
    });
}

void printLocalTime(){
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    Serial.print("Day of week: ");
    Serial.println(&timeinfo, "%A");
    Serial.print("Month: ");
    Serial.println(&timeinfo, "%B");
    Serial.print("Day of Month: ");
    Serial.println(&timeinfo, "%d");
    Serial.print("Year: ");
    Serial.println(&timeinfo, "%Y");
    Serial.print("Hour: ");
    Serial.println(&timeinfo, "%H");
    Serial.print("Hour (12 hour format): ");
    Serial.println(&timeinfo, "%I");
    Serial.print("Minute: ");
    Serial.println(&timeinfo, "%M");
    Serial.print("Second: ");
    Serial.println(&timeinfo, "%S");

    Serial.println("Time variables");
    char timeHour[3];
    strftime(timeHour,3, "%H", &timeinfo);
    Serial.println(timeHour);
    char timeWeekDay[10];
    strftime(timeWeekDay,10, "%A", &timeinfo);
    Serial.println(timeWeekDay);
    Serial.println();
}

unsigned long getTimeUnix() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        //Serial.println("Failed to obtain time");
        return(0);
    }
    time(&now);
    return now;
}

void setColonVis(bool visibility) {
    digitalWrite(COLON_1_PIN, visibility);
    digitalWrite(COLON_2_PIN, visibility);
}

void antiCathodePoisonRoutine(int speed, bool reset) {
    for (uint8_t number = 0; number < 12; number++) {
        tubes.clearNUMTo(number);
        tubes.showNUM();
        delay(speed);
        if (reset) esp_task_wdt_reset();
    }
    tubes.clear();
    tubes.show();
}

void antiCathodePoisonRoutineSC(int speed, bool reset) {
    for (uint8_t number = 0; number < 8; number++) {
        tubes.setCharacter(0, number);
        tubes.setCharacter(1, number);
        tubes.showSC();
        delay(speed);
        if (reset) esp_task_wdt_reset();
    }
    tubes.clear();
    tubes.show();
}

String serverName = "http://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/18.087880/lat/59.511650/data.json";

void getWeather() {
    unsigned long unixTime = getTimeUnix();
    unixTime += 24*60*60;
    time_t newTime = unixTime;

    String monthNow = String(month(newTime));
    String dayNow = String(day(newTime));

    if (monthNow.length() < 2) {
        monthNow = "0" + monthNow;
    }
    if (dayNow.length() < 2) {
        dayNow = "0" + dayNow;
    }

    String code = String(year(newTime)) + "-" + monthNow + "-" + dayNow + "T14:00:00Z";
    Serial.println(code);

    WiFiClient client;
    HTTPClient http;

    http.begin(client, serverName);
    Serial.print("[HTTP] GET...\n");

    JsonStreamingParser parser;
    WeatherParser listener(code);
    parser.setListener(&listener);

    int httpCode = http.GET();
    if (httpCode > 0) {
        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        if (httpCode == HTTP_CODE_OK) {
            int len = http.getSize();
            uint8_t buff[128] = { 0 };
            WiFiClient* stream = &client;

            while (http.connected() && (len > 0 || len == -1)) {
                // read up to 128 byte
                int c = stream->readBytes(buff, std::min((size_t)len, sizeof(buff)));
                Serial.printf("readBytes: %d\n", c);
                if (!c) { Serial.println("read timeout"); }

                // write it to Serial
                //Serial.write(buff, c);
                for (unsigned char & i : buff) {
                    parser.parse(i);
                    esp_task_wdt_reset();
                }

                if (len > 0) { len -= c; }
            }
            Serial.println();
            Serial.print("[HTTP] connection closed or file end.\n");
        }
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    esp_task_wdt_reset();

    Serial.println(listener.t);
    Serial.println(listener.ws);
    Serial.println(listener.msl);
    Serial.println(listener.r);
    Serial.println(listener.wsymb2);

    temp = listener.t.toFloat();
    pressure = listener.msl.toFloat();
    windSpeed = listener.ws.toFloat();
    humidity = listener.r.toInt();
    wSymb2 = listener.wsymb2.toInt();

    http.end();
}

void showWeather() {
    // temp

    tubes.setCharacter(1, SC_DEG_C);
    int sign = SC_PLUS;
    if (temp < 0) {
        sign = SC_MINUS;
    }

    tubes.setCharacter(0, sign);
    tubes.setNumber(0, abs((int) temp) / 10);
    tubes.setNumber(1, abs((int) temp) % 10);
    tubes.setNumber(2, abs((int) (temp * 10)) % 10);
    tubes.setNumber(3, 0);
    digitalWrite(COLON_1_PIN, HIGH);
    tubes.show();

    delay(3000);
    esp_task_wdt_reset();
    delay(3000);

    tubes.clear();
    setColonVis(false);
    tubes.show();
    delay(1000);

    if (humidity == 100) {
        tubes.setNumber(1, 1);
    }
    tubes.setNumber(2, (humidity/10) % 10);
    tubes.setNumber(3, humidity%10);
    tubes.setCharacter(1, SC_PERCENT);
    tubes.show();

    esp_task_wdt_reset();
    delay(5000);

    tubes.clear();
    tubes.show();
    delay(1000);
    esp_task_wdt_reset();

    tubes.setNumber(0, (int) pressure / 1000);
    tubes.setNumber(1, ((int) pressure / 100) % 10);
    tubes.setNumber(2, ((int) pressure / 10) % 10);
    tubes.enableNumber(2, RIGHT_COMMA);
    tubes.setNumber(3, ((int) round(pressure)) % 10);
    for (int i = 0; i < 3; i++) {
        tubes.setCharacter(1, SC_KELVIN);
        tubes.show();
        delay(1500);
        esp_task_wdt_reset();
        tubes.setCharacter(1, SC_P);
        tubes.show();
        delay(1500);
    }

    esp_task_wdt_reset();
    tubes.clear();
    tubes.show();
    delay(1000);
    antiCathodePoisonRoutine(1000, true);
    antiCathodePoisonRoutineSC(1000, true);
}

void showDate() {
    setColonVis(false);

    antiCathodePoisonRoutine(1000, true); //TODO: Every five minutes is just a made up number. Research the best time spend doing this.
    // TODO Better cathode poisoning algorithm need to be added, and also resyncing and long cathode poisoning program at night. Also turn the clock off at specified times.
    //Show temp and date here
    tubes.setVisibility(false);
    struct tm timeNow;
    getLocalTime(&timeNow);

    int dayNow = timeNow.tm_mday;
    tubes.setNumber(0, dayNow/10); //Int division -> remove the last digit
    tubes.setNumber(1, dayNow%10);

    int monthNow = timeNow.tm_mon + 1;
    tubes.setNumber(2, monthNow/10);
    tubes.setNumber(3, monthNow%10);
    //Display something on SC Tubes?
    tubes.showNUM();

    delay(1000);
    setColonVis(true);
    delay(1000);
    tubes.setVisibility(true);

    delay(3000);
    esp_task_wdt_reset();
    delay(3000);
    esp_task_wdt_reset();

    tubes.setVisibility(false);
    tubes.clear();
    tubes.show();
    delay(1000);
    setColonVis(false);
    delay(1000);
    tubes.setVisibility(true);
}

[[noreturn]] void normalTubeLoop(void * parameter) {
    tubes.setVisibilityNUM(true);
    while (true) {
        struct tm timeStart;
        getLocalTime(&timeStart);
        bool showTime = true;
        while (showTime) {
            struct tm timeNow;
            getLocalTime(&timeNow);

            int hour = timeNow.tm_hour;
            tubes.setNumber(0, hour/10); //Int division -> remove the last digit
            tubes.setNumber(1, hour%10);

            int minute = timeNow.tm_min;
            tubes.setNumber(2, minute/10);
            tubes.setNumber(3, minute%10);

            tubes.showNUM();

            if (minute % TEMP_INTERVAL == 0 || forceExtraTubeInfo) {
                showTime = false;
            }

            for (int i = 0; i < 30 - timeNow.tm_sec/2; i++) {
                setColonVis(false);
                delay(1000);
                setColonVis(true);
                delay(1000);
                esp_task_wdt_reset();
            }
        }
        showDate();

        struct tm timeNow;
        getLocalTime(&timeNow);
        if (timeNow.tm_min % 10 < 2 || forceExtraTubeInfo)  { //If the last digit of minute is less than 2, only show weather every 10 minute with the 5 minute date interval.
            showWeather();
        } else {
            antiCathodePoisonRoutine(1000, true);
            delay(1000);
        }
    }
}

[[noreturn]] void clockOffLoop(void * args) {
    int lastColor = random(0, 5);
    while (true) {
        struct tm timeNow;
        getLocalTime(&timeNow);

        if (animENTable[timeNow.tm_hour]) {
            int color = random(0, 5);
            uint32_t steps = random(255, 1000);
            double step = 1.0 / steps;

            for (int i = 0; i < steps; i++) {
                uint16_t lastColorValue = 100.0 - (i * step * 100);
                uint16_t newColorValue = i * step * 100;

                uint16_t r = lastColorValue * (lastColor == 0) + newColorValue * (color == 0);
                uint16_t g = lastColorValue * (lastColor == 1) + newColorValue * (color == 1);
                uint16_t b = lastColorValue * (lastColor == 2) + newColorValue * (color == 2);
                uint16_t ww = lastColorValue * (lastColor == 3) + newColorValue * (color == 3);
                uint16_t nw = lastColorValue * (lastColor == 4) + newColorValue * (color == 4);
                uint16_t cw = lastColorValue * (lastColor == 5) + newColorValue * (color == 5);

                rgbClearTo(Rgb48Color(r, g, b));
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
    antiCathodePoisonRoutine(250, true);
    tubes.clear();
    tubes.show();
    antiCathodePoisonRoutineSC(500, true);
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
    whiteStrip.SetPixelColor(3, RgbColor(25, 0, 0));
    whiteStrip.SetPixelColor(4, RgbColor(0, 25, 0));
    whiteStrip.SetPixelColor(5, RgbColor(0, 0, 25));
    whiteStrip.Show();

    RGBStrip.Begin();
    //RGBStrip.ClearTo(Rgb48Color(40 << 8, 0, 50 << 8)); // Stupid ClearTo function does not work for 16-bit and I wasted 1 hour of my life.
    RGBStrip.SetPixelColor(0, Rgb48Color(255, 0, 0));
    RGBStrip.SetPixelColor(1, Rgb48Color(0, 255, 0));
    RGBStrip.SetPixelColor(2, Rgb48Color(0, 0, 255));
    RGBStrip.Show();

    Serial.println("LEDs started");
    delay(100);
    Serial.println("Starting Shift Registers");

    ShiftRegisterNUM.begin();
    ShiftRegisterSC.begin();

    Serial.println("Shift registers started");
    delay(100);
    Serial.println("Starting SPIFFS");

    startSPIFFS();

    Serial.println("SPIFFS started");
    delay(100);
    Serial.println("Connecting WiFi");

    connectWiFi();

    Serial.println("WiFi connected");
    delay(100);
    Serial.println("Starting mDNS");

    startmDNS();

    Serial.println("mDNS responder started");
    delay(100);
    Serial.println("Starting WebServer");

    startServer();
    server.begin();

    configTime(3600, 3600, "pool.ntp.org");
    printLocalTime();

    Serial.println("WebServer Started");
    delay(100);
    Serial.println("Setup done");

    digitalWrite(STATUS_LED, LOW);
    ledcWriteTone(0, 1000);
    delay(400);
    digitalWrite(STATUS_LED, HIGH);
    ledcWrite(0, 0);
    delay(400);
    ledcWriteTone(0, 1000);
    delay(400);
    ledcWrite(0, 0);
    digitalWrite(STATUS_LED, LOW);

    whiteStrip.ClearTo(RgbColor(0, 0, 0));
    whiteStrip.Show();

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

    struct tm timeNow;
    getLocalTime(&timeNow);

    if (dayENTable[timeNow.tm_hour]) {
        startTubes();
    } else {
        shutDownTubes();
    }
}


int loopI = 0;
void loop() {
    delay(2000);
    esp_task_wdt_reset(); // TODO Better cathode poisoning algorithm need to be added, and also resyncing and long cathode poisoning program at night. Also turn the clock off at specified times.

    // https://www.manula.com/manuals/daliborfarny-com/nixie-tubes/1/en/topic/cathode-poisoning-prevention-routine
    // For Dalibor Farny Tubes use ratio 60 : 0.2 seconds.

    if (WiFi.status() != WL_CONNECTED) {
        Serial.print(millis());
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
    }

    struct tm timeNow;
    getLocalTime(&timeNow);

    if (!manualTubes) {
        if (tubesRunning && !dayENTable[timeNow.tm_hour]) {
            esp_task_wdt_delete(normalTubeRunnerHandle);
            vTaskDelete(normalTubeRunnerHandle);

            for (int i = 0; i < 20; i++) {
                antiCathodePoisonRoutine(2000, true);
            }
            for (int i = 0; i < 10; i++) {
                antiCathodePoisonRoutineSC(2000, true);
            }

            shutDownTubes();

        } else if (!tubesRunning && dayENTable[timeNow.tm_hour]) {
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
        getWeather();
        Serial.println("Weather done");
        Serial.println("Re-sync time");
        configTime(3600, 3600, "pool.ntp.org");
        Serial.println("Time re-synced");
    }


    if (loopI % 10 == 0) {
        uint16_t adc_data = analogRead(HV_SENSE_PIN);
        double voltage = ((adc_data / 4095.0) * 3.3) / 2000.0 * 202000;
        Serial.println(voltage);
    }

    loopI++;
    loopI %= 60 * 60 / 2;
}