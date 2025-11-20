#ifndef PID_H
#define PID_H

#include <Arduino.h>

// Anti-Windup Constants
#define DC_INTEGRAL_MAX 500.0  // Maksimal integral term untuk DC motor
#define DC_INTEGRAL_MIN -500.0 // Minimal integral term untuk DC motor
#define AC_INTEGRAL_MAX 500.0  // Maksimal integral term untuk AC motor
#define AC_INTEGRAL_MIN -500.0 // Minimal integral term untuk AC motor

extern volatile double DCpreviousError;
extern volatile double DCerrorSum;
extern volatile double DCError;
extern volatile double DCsignalPWM;
extern volatile double DCkP;
extern volatile double DCkI;
extern volatile double DCkD;
extern volatile double DCsetpoint;
extern float DCintegralSum;

    extern volatile double ACpreviousError;
extern volatile double ACerrorSum;
extern volatile double ACError;
extern volatile double ACsignalPWM;
extern volatile double ACkP;
extern volatile double ACkI;
extern volatile double ACkD;
extern volatile double ACsetpoint;

float AC_PID(float setpoint, float measured, float dt); // fungsi PID untuk AC
float DC_PID(float setpoint, float measured, float dt); // fungsi PID untuk DC

#endif