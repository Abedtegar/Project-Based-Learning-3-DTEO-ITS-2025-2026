#ifndef PID_H
#define PID_H

#include <Arduino.h>
extern volatile double DCpreviousError;
extern volatile double DCerrorSum;
extern volatile double DCError;
extern volatile double DCsignalPWM;
extern volatile double DCkP;
extern volatile double DCkI;
extern volatile double DCkD;
extern volatile double DCsetpoint;

extern volatile double ACpreviousError;
extern volatile double ACerrorSum;
extern volatile double ACError;
extern volatile double ACsignalPWM;
extern volatile double ACkP;
extern volatile double ACkI;
extern volatile double ACkD;
extern volatile double ACsetpoint;

float AC_PID(float setpoint, float measured, float dt);
float DC_PID(float setpoint, float measured, float dt);

#endif