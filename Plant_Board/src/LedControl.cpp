#include "LedControl.h"
#include "PlantConfig.h"

unsigned long LEDCurrentTime;
unsigned long LEDPreviousTime;

void initLedControl() {
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_PID, OUTPUT);
  pinMode(LED_DC_CONTROL, OUTPUT);
  pinMode(LED_AC_CONTROL, OUTPUT);
}

void setLedState(int ledPin, bool state) { digitalWrite(ledPin, state); }
void blinkLed(int ledPin, long delayTime) {
  LEDCurrentTime = millis();
  if (LEDCurrentTime - LEDPreviousTime >= delayTime) {
    LEDPreviousTime = LEDCurrentTime;
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
}