#include "main.h"
#include "ShiftRegisterDriver.h"
#include "TubeDriver.h"


ShiftRegisterDriver ShiftRegisterNUM(26, 32, 33, 25);
ShiftRegisterDriver ShiftRegisterSC(18, 17, 16, 4);
TubeDriver tubes(&ShiftRegisterNUM, &ShiftRegisterSC);

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
    whiteStrip.ClearTo(RgbColor(0, 0, 0));
    whiteStrip.Show();

    RGBStrip.Begin();
    RGBStrip.ClearTo(RgbColor(0, 0, 0));
    RGBStrip.Show();

    Serial.println("LEDs started");
    delay(100);
    Serial.println("Starting Shift Registers");

    ShiftRegisterNUM.begin();
    ShiftRegisterSC.begin();

    Serial.println("Shift registers started");
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

    digitalWrite(PSU_EN_PIN, HIGH);
}

void loop() {
    delay(1000);
    uint8_t shiftRegisterTable[8] = {
            0b1 << 1, 0b1 << 7, 0b1 << 2, 0b1 << 3, 0b1 << 1, 0b1 << 3, 0b1 << 5, 0b1 << 5
    };
    ShiftRegisterNUM.sendData(shiftRegisterTable, 8);
    ShiftRegisterSC.sendData(shiftRegisterTable, 8);
    ShiftRegisterNUM.enable(true);
    ShiftRegisterSC.enable(true);

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