#include "LedControl.h"
#include "PlantConfig.h"

long Currenttime;
long PreviousTime;

void initLedControl() {
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_PID, OUTPUT);
  pinMode(LED_DC_CONTROL, OUTPUT);
  pinMode(LED_AC_CONTROL, OUTPUT);
}

void setLedState(int ledPin, bool state) { digitalWrite(ledPin, state); }
void blinkLed(int ledPin, long delayTime) {
  Currenttime = millis();
  if (Currenttime - PreviousTime >= delayTime) {
    PreviousTime = Currenttime;
    digitalWrite(ledPin, !digitalRead(ledPin));
  }
}