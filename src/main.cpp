#include "main.h"

ShiftRegisterDriver ShiftRegisterNUM(26, 32, 33, 25);
ShiftRegisterDriver ShiftRegisterSC(18, 17, 16, 4);
TubeDriver tubes(&ShiftRegisterNUM, tubeTable, 4, &ShiftRegisterSC, {}, 0);

void connectWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi ..");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
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

    server.on("/doUpdate", HTTP_POST,
              [](AsyncWebServerRequest *request) {},
              [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                 size_t len, bool final) {
                  handleDoUpdate(request, filename, index, data, len, final);
              });
    Update.onProgress(printProgress);
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
    whiteStrip.ClearTo(RgbColor(20, 0, 0));
    whiteStrip.Show();

    RGBStrip.Begin();
    RGBStrip.ClearTo(Rgb48Color(40, 0, 50));
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

    RGBStrip.ClearTo(Rgb48Color(0, 0, 0));
    RGBStrip.Show();

    digitalWrite(PSU_EN_PIN, HIGH);
}

void loop() {
    /*for (uint8_t number = 0; number < 12; number++) {
        uint8_t shiftRegisterTable[6] = {0, 0, 0, 0, 0, 0};

        for (uint8_t tube = 0; tube < 4; tube++) {
            uint8_t shift = tubeTable[tube]->lookup[number] % 8;
            uint8_t index = tubeTable[tube]->lookup[number] / 8;
            shiftRegisterTable[index] |= 0b1 << shift;
        }
        Serial.println(number);
        ShiftRegisterNUM.sendData(shiftRegisterTable, 6);
        delay(1000);
    }*/
    //ShiftRegisterSC.sendData(shiftRegisterTable, 8);
    //ShiftRegisterSC.enable(true);

    for (uint8_t number = 0; number < 12; number++) {
        //uint8_t shiftRegisterTable[6] = {0, 0, 0, 0, 0, 0};
        for (uint8_t tube = 0; tube < 4; tube++) {
            tubes.setNumber(tube, (number + tube)%12);
            /*uint8_t shift = tubeTable[tube]->lookup[number] % 8;
            uint8_t index = tubeTable[tube]->lookup[number] / 8;
            shiftRegisterTable[index] |= 0b1 << shift;*/
        }
        /*for (int i = 5; i >= 0; i--) {
            for (int8_t aBit = 7; aBit >= 0; aBit--)
                Serial.write(bitRead(shiftRegisterTable[i], aBit) ? '1' : '0');
        }
        Serial.println("");*/
        tubes.showNUM();
        tubes.setVisibilityNUM(true);
        delay(1000);
    }

    uint16_t adc_data = analogRead(HV_SENSE_PIN);
    double voltage = ((adc_data / 4095.0) * 3.3) / 2000.0 * 202000;
    Serial.println(voltage);

    /*whiteStrip.SetPixelColor(0, RgbColor(20, 0, 0));
    whiteStrip.SetPixelColor(1, RgbColor(0, 20, 0));
    whiteStrip.SetPixelColor(2, RgbColor(0, 0, 20));
    whiteStrip.Show();

    RGBStrip.SetPixelColor(0, Rgb48Color(20, 0, 0));
    RGBStrip.SetPixelColor(1, Rgb48Color(0, 20, 0));
    RGBStrip.SetPixelColor(2, Rgb48Color(0, 0, 20));
    RGBStrip.Show();
    digitalWrite(STATUS_LED, LOW);
    delay(100);
    digitalWrite(STATUS_LED, HIGH);
    //Serial.println("Alive");
    delay(100);*/


}