#include "PID.h"
#include <PlantConfig.h>

volatile double DCpreviousError;
volatile double DCerrorSum;
volatile double DCError;
volatile double DCsignalPWM;
volatile double DCkP;
volatile double DCkI;
volatile double DCkD;
volatile double DCsetpoint;

volatile double ACpreviousError;
volatile double ACerrorSum;
volatile double ACError;
volatile double ACsignalPWM;
volatile double ACkP;
volatile double ACkI;
volatile double ACkD;
volatile double ACsetpoint;

float DCProportional, DCIntegral, DCDerivative, DCintegralSum;
float ACProportional, ACIntegral, ACDerivative, ACintegralSum;

float AC_PID(float setpoint, float measured, float dt) {
  float output = 0.0;

  ACError = setpoint - measured;

  ACProportional = ACkP * ACError;

  ACintegralSum += ACError * dt;
  //   if (integralSum > MaxMotorSpeed)
  //     integralSum = MaxMotorSpeed; // anti-windup
  //   else if (integralSum < -MaxMotorSpeed)
  //     integralSum = -MaxMotorSpeed;

  ACIntegral = ACkI * ACintegralSum;

  ACDerivative = ACkD * (ACError - ACpreviousError) / dt;

  ACpreviousError = ACError;

  output = ACProportional + ACIntegral + ACDerivative;

  // Saturate output
  //   if (output > MaxMotorSpeed)
  //     output = MaxMotorSpeed;
  //   else if (output < -MaxMotorSpeed)
  //     output = -MaxMotorSpeed;

  return output;
}

float DC_PID(float setpoint, float measured, float dt) {
  float output = 0.0;

  DCError = setpoint - measured;

  DCProportional = DCkP * DCError;

  DCintegralSum += DCError * dt;
  //   if (integralSum > MaxMotorSpeed)
  //     integralSum = MaxMotorSpeed; // anti-windup
  //   else if (integralSum < -MaxMotorSpeed)
  //     integralSum = -MaxMotorSpeed;

  DCIntegral = DCkI * DCintegralSum;

  DCDerivative = DCkD * (DCError - DCpreviousError) / dt;

  DCpreviousError = DCError;

  output = DCProportional + DCIntegral + DCDerivative;

  // Saturate output
  //   if (output > MaxMotorSpeed)
  //     output = MaxMotorSpeed;
  //   else if (output < -MaxMotorSpeed)
  //     output = -MaxMotorSpeed;

  return output;
}

