#ifndef Plant_H
#define Plant_H

#include <Arduino.h>

// DC
extern volatile long DCencoder;
extern volatile long DClastEncoder;
extern volatile float DCrpm;
extern volatile long DCpulseCount;
extern volatile bool DCDirection;

extern volatile unsigned long ACencoder;
extern volatile float ACrpm;
extern volatile bool ACDirection;
extern volatile bool ACVoltage;
extern volatile int ACMotorControlMode;

// Deklarasi fungsi
void DCinitEncoder();                            // setup encoder DC
void DCstartEncoderTimer();                      // mulai timer encoder DC
float DCgetRPM();                                // dapatkan RPM DC
long DCgetEncoderCount();                        // dapatkan jumlah encoder DC
void DCresetEncoder();                           // reset encoder DC
void DCprintEncoderData();                       // cetak data encoder DC
void DCmotorControl(bool direction, long speed); // kontrol motor DC
void DCstartMotorTimer();                        // mulai timer motor DC

void ACinitEncoder();      // setup encoder AC
float ACgetRPM();          // dapatkan RPM AC
long ACgetEncoderCount();  // dapatkan jumlah encoder AC
void ACresetEncoder();     // reset encoder AC
void ACprintEncoderData(); // cetak data encoder AC
void ACmotorControl(bool direction, long speed, bool voltage,int mode); // kontrol motor AC
void ACstartMotorTimer();      // mulai timer motor DC
void ACstartEncoderTimer();
#endif