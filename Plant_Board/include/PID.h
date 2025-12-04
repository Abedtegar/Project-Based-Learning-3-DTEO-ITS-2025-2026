#ifndef PID_H
#define PID_H

#include <Arduino.h>

// Anti-Windup Constants
#define DC_INTEGRAL_MAX 1000.0  // Maksimal integral term untuk DC motor
#define DC_INTEGRAL_MIN -1000.0 // Minimal integral term untuk DC motor
#define AC_INTEGRAL_MAX 1000.0  // Maksimal integral term untuk AC motor
#define AC_INTEGRAL_MIN -1000.0 // Minimal integral term untuk AC motor

extern volatile float DCpreviousError;
extern volatile float DCerrorSum;
extern volatile float DCError;
extern volatile float DCsignalPWM;
extern volatile float DCkP;
extern volatile float DCkI;
extern volatile float DCkD;
extern volatile float DCsetpoint;
extern float DCintegralSum;

extern volatile float ACpreviousError;
extern volatile float ACerrorSum;
extern volatile float ACError;
extern volatile float ACsignalPWM;
extern volatile float ACkP;
extern volatile float ACkI;
extern volatile float ACkD;
extern volatile float ACsetpoint;
extern float ACintegralSum;

extern bool PIDMODE;
float AC_PID(float setpoint, float measured, float dt); // fungsi PID untuk AC
float DC_PID(float setpoint, float measured, float dt); // fungsi PID untuk DC
void UpdatePIDParam();
#endif