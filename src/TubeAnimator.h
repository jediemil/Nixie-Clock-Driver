//
// Created by emilr on 2023-11-02.
//

#ifndef NIXIECLOCK_TUBEANIMATOR_H
#define NIXIECLOCK_TUBEANIMATOR_H

#include "includes.h"
#include "Arduino.h"
#include "TubeDriver.h"
#include <esp_task_wdt.h>
#include "weather.h"
#include "./../.pio/libdeps/esp32dev/ezTime/src/ezTime.h"

class TubeAnimator {
public:
    void setColonVis(bool visibility);
    void setColonVis(bool visibility1, bool visibility2);

    void antiCathodePoisonRoutine(int speed, bool reset);
    void antiCathodePoisonRoutineSC(int speed, bool reset);

    void showDate();
    void showWeather();

    void setWeather(struct weather weather);

    void begin(TubeDriver* tubes);

private:
    TubeDriver* tubes;
    struct weather weather;
};


#endif //NIXIECLOCK_TUBEANIMATOR_H
