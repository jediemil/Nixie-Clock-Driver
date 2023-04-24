//
// Created by emilr on 2023-04-24.
//

#include "WeatherParser.h"

void WeatherParser::whitespace(char c) {
}

void WeatherParser::startDocument() {
    Serial.println("start document");
}

void WeatherParser::key(String key) {
    //Serial.println("key: " + key);

    if (key == "validTime") {
        Serial.println("Found time series");
        inTimeSeries = true;
    }

    if (foundWeatherKey && key == "values") {
        Serial.println("Found values");
        foundValues = true;
    }
}

void WeatherParser::value(String value) {
    //Serial.println("value: " + value);

    if (inTimeSeries && value == timeCode) {
        Serial.println("Found timecode");
        foundTime = true;
        inTimeSeries = false;
    } else if (inTimeSeries) {
        Serial.println("Not equal");
        inTimeSeries = false;
    }
    if (foundValues) {
        Serial.printf("Found value %s\n", keyName.c_str());
        if (keyName == "t") {
            t = value;
        }
        if (keyName == "ws") {
            ws = value;
        }
        if (keyName == "r") {
            r = value;
        }
        if (keyName == "msl") {
            msl = value;
        }
        if (keyName == "Wsymb2") {
            wsymb2 = value;
            foundTime = false;
        }
        foundValues = false;
        foundWeatherKey = false;
    }

    if (foundTime && value == "t") {
        foundWeatherKey = true;
        keyName = "t";
    }
    if (foundTime && value == "ws") {
        foundWeatherKey = true;
        keyName = "ws";
    }
    if (foundTime && value == "r") {
        foundWeatherKey = true;
        keyName = "r";
    }
    if (foundTime && value == "msl") {
        foundWeatherKey = true;
        keyName = "msl";
    }
    if (foundTime && value == "Wsymb2") {
        foundWeatherKey = true;
        keyName = "Wsymb2";
    }
}

void WeatherParser::endArray() {
}

void WeatherParser::endObject() {
}

void WeatherParser::endDocument() {
    Serial.println("end document. ");
}

void WeatherParser::startArray() {
}

void WeatherParser::startObject() {
}

WeatherParser::WeatherParser(String timeWeather) {
    timeCode = timeWeather;
    foundTime = false;
    inTimeSeries = false;
    foundWeatherKey = false;
    keyName = "NULL";
    foundValues = false;
}
