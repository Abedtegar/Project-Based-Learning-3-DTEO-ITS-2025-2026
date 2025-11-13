#ifndef Plant_H
#define Plant_H

#include <Arduino.h>

// DC
extern volatile long DCencoder;
extern volatile long DClastEncoder;
extern volatile float DCrpm;
extern volatile long DCpulseCount;

extern volatile unsigned long ACencoder;
extern volatile float ACrpm;

// Deklarasi fungsi
void DCinitEncoder();
void DCstartEncoderTimer();
float DCgetRPM();
long DCgetEncoderCount();
void DCresetEncoder();
void DCprintEncoderData();

void ACinitEncoder();
float ACgetRPM();
long ACgetEncoderCount();
void ACresetEncoder();
void ACprintEncoderData();

#endif