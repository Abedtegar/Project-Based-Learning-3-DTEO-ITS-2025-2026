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
extern volatile bool DCPID;

extern volatile double ACpreviousError;
extern volatile double ACerrorSum;
extern volatile double ACError;
extern volatile double ACsignalPWM;
extern volatile double ACkP;
extern volatile double ACkI;
extern volatile double ACkD;
extern volatile double ACsetpoint;
extern volatile bool ACPID;

float AC_PID(float setpoint, float measured, float dt); // fungsi PID untuk AC
float DC_PID(float setpoint, float measured, float dt); // fungsi PID untuk DC

#endif