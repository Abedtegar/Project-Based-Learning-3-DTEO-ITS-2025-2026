#ifndef LEDCONTROL_H
#define LEDCONTROL_H

#include <Arduino.h>
extern long Currenttime;
extern long PreviousTime;

void initLedControl();
void setLedState(int ledPin, bool state);
void blinkLed(int ledPin, long delayTime);

#endif