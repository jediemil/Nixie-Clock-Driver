//
// Created by emilr on 2023-04-24.
//

#ifndef NIXIECLOCK_WEATHERPARSER_H
#define NIXIECLOCK_WEATHERPARSER_H

#include "Arduino.h"
#include "JsonListener.h"

class WeatherParser: public JsonListener {
public:
    virtual void whitespace(char c);

    virtual void startDocument();

    virtual void key(String key);

    virtual void value(String value);

    virtual void endArray();

    virtual void endObject();

    virtual void endDocument();

    virtual void startArray();

    virtual void startObject();

    WeatherParser(String timeWeather);

    String t;
    String ws;
    String r;
    String msl;
    String wsymb2;


private:
    bool foundTime;
    bool inTimeSeries;
    String timeCode;
    bool foundWeatherKey;
    String keyName;
    bool foundValues;
};


#endif //NIXIECLOCK_WEATHERPARSER_H
