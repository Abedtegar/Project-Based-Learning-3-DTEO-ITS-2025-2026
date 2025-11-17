#include "PID.h"
#include <PlantConfig.h>

volatile double DCpreviousError = 0.0;
volatile double DCerrorSum = 0.0;
volatile double DCError = 0.0;
volatile double DCsignalPWM = 0.0;
volatile double DCkP = 1.5;
volatile double DCkI = 0.1;
volatile double DCkD = 0.05;
volatile double DCsetpoint = 50.0;

volatile double ACpreviousError = 0.0;
volatile double ACerrorSum = 0.0;
volatile double ACError = 0.0;
volatile double ACsignalPWM = 0.0;
volatile double ACkP = 2.0;
volatile double ACkI = 0.08;
volatile double ACkD = 0.04;
volatile double ACsetpoint = 0.0;
float DCProportional = 0.0, DCIntegral = 0.0, DCDerivative = 0.0,
      DCintegralSum = 0.0;
float ACProportional = 0.0, ACIntegral = 0.0, ACDerivative = 0.0,
      ACintegralSum = 0.0;

float AC_PID(float setpoint, float measured, float dt) {
  float output = 0.0;

  ACError = setpoint - measured;

  ACProportional = ACkP * ACError;

  ACintegralSum += ACError * dt;

  if (ACintegralSum > 500.0)
    ACintegralSum = 500.0;
  else if (ACintegralSum < -500.0)
    ACintegralSum = -500.0;

  ACIntegral = ACkI * ACintegralSum;

  ACDerivative = ACkD * (ACError - ACpreviousError) / dt;

  ACpreviousError = ACError;

  output = (ACProportional + ACIntegral + ACDerivative) * -1;

  // Saturate output
  if (output > 255.0)
    output = 255.0;
  else if (output < 0.0)
    output = 0.0;

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
