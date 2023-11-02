//
// Created by emilr on 2023-11-02.
//

#include "ServerRunner.h"

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

String serverName = "http://opendata-download-metfcst.smhi.se/api/category/pmp3g/version/2/geotype/point/lon/18.087880/lat/59.511650/data.json";

void ServerRunner::startmDNS() {
    if (!MDNS.begin("klocka")) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
}

void ServerRunner::startSPIFFS() {
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
}

void ServerRunner::startServer() {
    server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });

    server->on("/file", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send_P(200, "text/html", file_html, processor);
    });

    server->serveStatic("/", SPIFFS, "/");

    server->on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) {
        request->send(200);
    }, handleUpload);

    server->on("/update", HTTP_POST,
               [](AsyncWebServerRequest *request) {},
               [this](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,
                      size_t len, bool final) {
                   handleDoUpdate(request, filename, index, data, len, final);
               });
    Update.onProgress(printProgress);

    server->on("/reset", HTTP_GET,[](AsyncWebServerRequest *request) {
        AsyncResponseStream *response = request->beginResponseStream("text/plain");
        response->addHeader("Access-Control-Allow-Origin","*");
        response->addHeader("Access-Control-Allow-Headers", "Content-Type, data");
        response->addHeader("Access-Control-Allow-Methods", "POST, GET");
        response->setCode(200);
        response->write("Episkt");
        request->send(response);

        delay(500);

        ESP.restart();
    });
}

struct weather ServerRunner::getWeather() {
    unsigned long unixTime = UTC.now();
    unixTime += 24*60*60;
    time_t newTime = unixTime;

    /*String monthNow = String(month(newTime));
    String dayNow = String(day(newTime));

    if (monthNow.length() < 2) {
        monthNow = "0" + monthNow;
    }
    if (dayNow.length() < 2) {
        dayNow = "0" + dayNow;
    }*/

    String monthNow = dateTime(newTime, "m");
    String dayNow = dateTime(newTime, "d");

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

    struct weather newWeather;

    newWeather.temp = listener.t.toFloat();
    newWeather.pressure = listener.msl.toFloat();
    newWeather.windSpeed = listener.ws.toFloat();
    newWeather.humidity = listener.r.toInt();
    newWeather.wSymb2 = listener.wsymb2.toInt();

    //tubeAnim.setWeather(newWeather);

    http.end();

    return newWeather;
}

void ServerRunner::handleDoUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (!index){
        Serial.println("Update");
        //content_len = request->contentLength();
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

void ServerRunner::printProgress(size_t prg, size_t sz) {
    //Serial.printf("Progress: %d%%\n", (prg*100)/content_len);
    Serial.printf("Progress : %dB\n", prg);
}

void ServerRunner::handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
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

String ServerRunner::humanReadableSize(const size_t bytes) {
    if (bytes < 1024) return String(bytes) + " B";
    else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
    else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
}

String ServerRunner::listFiles(bool ishtml) {
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

String ServerRunner::processor(const String& var) {
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

ServerRunner::ServerRunner(AsyncWebServer *server) {
    this->server = server;
}
