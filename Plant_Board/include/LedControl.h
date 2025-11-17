#ifndef LEDCONTROL_H
#define LEDCONTROL_H

#include <Arduino.h>
extern unsigned long LEDCurrentTime;
extern unsigned long LEDPreviousTime;

void initLedControl();                     // deklarsi pin
void setLedState(int ledPin, bool state);  // set status led
void blinkLed(int ledPin, long delayTime); // kedipkan led

#endif