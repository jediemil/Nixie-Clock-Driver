//
// Created by emilr on 2023-11-02.
//

#include "TubeAnimator.h"

void TubeAnimator::begin(TubeDriver *tubes) {
    this->tubes = tubes;
}

void TubeAnimator::setColonVis(bool visibility) {
    digitalWrite(COLON_1_PIN, visibility);
    digitalWrite(COLON_2_PIN, visibility);
}

void TubeAnimator::setColonVis(bool visibility1, bool visibility2) {
    digitalWrite(COLON_1_PIN, visibility1);
    digitalWrite(COLON_2_PIN, visibility2);
}

void TubeAnimator::antiCathodePoisonRoutine(int speed, bool reset) {
    for (uint8_t number = 0; number < 12; number++) {
        tubes->clearNUMTo(number);
        tubes->showNUM();
        delay(speed);
        if (reset) esp_task_wdt_reset();
    }
    tubes->clear();
    tubes->show();
}

void TubeAnimator::antiCathodePoisonRoutineSC(int speed, bool reset) {
    for (uint8_t number = 0; number < 8; number++) {
        tubes->setCharacter(0, number);
        tubes->setCharacter(1, number);
        tubes->showSC();
        delay(speed);
        if (reset) esp_task_wdt_reset();
    }
    tubes->clear();
    tubes->show();
}

void TubeAnimator::showDate() {
    setColonVis(false);

    antiCathodePoisonRoutine(1000, true); //TODO: Every five minutes is just a made up number. Research the best time spend doing this.
    // TODO Better cathode poisoning algorithm need to be added, and also resyncing and long cathode poisoning program at night. Also turn the clock off at specified times.
    //Show temp and date here
    tubes->setVisibility(false);
    //struct tm timeNow;
    //getLocalTime(&timeNow);

    int dayNow = day(TIME_NOW);
    tubes->setNumber(0, dayNow/10); //Int division -> remove the last digit
    tubes->setNumber(1, dayNow%10);

    int monthNow = month(LAST_READ);
    tubes->setNumber(2, monthNow/10);
    tubes->setNumber(3, monthNow%10);
    //Display something on SC Tubes?
    tubes->showNUM();

    delay(1000);
    setColonVis(true);
    delay(1000);
    tubes->setVisibility(true);

    delay(3000);
    esp_task_wdt_reset();
    delay(3000);
    esp_task_wdt_reset();

    tubes->setVisibility(false);
    tubes->clear();
    tubes->show();
    delay(1000);
    setColonVis(false);
    delay(1000);
    tubes->setVisibility(true);
}

void TubeAnimator::showWeather() {
    // temp

    tubes->setCharacter(1, SC_DEG_C);
    int sign = SC_PLUS;
    if (weather.temp < 0) {
        sign = SC_MINUS;
    }

    tubes->setCharacter(0, sign);
    tubes->setNumber(0, abs((int) weather.temp) / 10);
    tubes->setNumber(1, abs((int) weather.temp) % 10);
    tubes->setNumber(2, abs((int) (weather.temp * 10)) % 10);
    tubes->setNumber(3, 0);
    setColonVis(true, false);
    tubes->show();

    delay(3000);
    esp_task_wdt_reset();
    delay(3000);

    tubes->clear();
    setColonVis(false);
    tubes->show();
    delay(1000);

    if (weather.humidity == 100) {
        tubes->setNumber(1, 1);
    }
    tubes->setNumber(2, (weather.humidity/10) % 10);
    tubes->setNumber(3, weather.humidity%10);
    tubes->setCharacter(1, SC_PERCENT);
    tubes->show();

    esp_task_wdt_reset();
    delay(5000);

    tubes->clear();
    tubes->show();
    delay(1000);
    esp_task_wdt_reset();

    tubes->setNumber(0, (int) weather.pressure / 1000);
    tubes->setNumber(1, ((int) weather.pressure / 100) % 10);
    tubes->setNumber(2, ((int) weather.pressure / 10) % 10);
    tubes->enableNumber(2, RIGHT_COMMA);
    tubes->setNumber(3, ((int) round(weather.pressure)) % 10);
    for (int i = 0; i < 3; i++) {
        tubes->setCharacter(1, SC_KELVIN);
        tubes->show();
        delay(1500);
        esp_task_wdt_reset();
        tubes->setCharacter(1, SC_P);
        tubes->show();
        delay(1500);
    }

    esp_task_wdt_reset();
    tubes->clear();
    tubes->show();
    delay(1000);
    antiCathodePoisonRoutine(1000, true);
    antiCathodePoisonRoutineSC(1000, true);
}

void TubeAnimator::setWeather(struct weather weather) {
    this->weather = weather;
}
