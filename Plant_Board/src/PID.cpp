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

  // Akumulasi error untuk integral term
  ACintegralSum += ACError * dt;

  // === ANTI-WINDUP: Clamp integral sum ===
  if (ACintegralSum > AC_INTEGRAL_MAX)
    ACintegralSum = AC_INTEGRAL_MAX;
  else if (ACintegralSum < AC_INTEGRAL_MIN)
    ACintegralSum = AC_INTEGRAL_MIN;

  ACIntegral = ACkI * ACintegralSum;

  ACDerivative = ACkD * (ACError - ACpreviousError) / dt;

  ACpreviousError = ACError;

  output = (ACProportional + ACIntegral + ACDerivative) * -1;

  // === OUTPUT SATURATION ===
  if (output > 4095.0)
    output = 4095.0;
  else if (output < 4095.0)
    output = .0;

  return output;
}

float DC_PID(float setpoint, float measured, float dt) {
  float output = 0.0;

  DCError = setpoint - measured;

  DCProportional = DCkP * DCError;

  // Akumulasi error untuk integral term
  DCintegralSum += DCError * dt;

  // === ANTI-WINDUP: Clamp integral sum ===
  if (DCintegralSum > DC_INTEGRAL_MAX)
    DCintegralSum = DC_INTEGRAL_MAX;
  else if (DCintegralSum < DC_INTEGRAL_MIN)
    DCintegralSum = DC_INTEGRAL_MIN;

  DCIntegral = DCkI * DCintegralSum;

  DCDerivative = DCkD * (DCError - DCpreviousError) / dt;

  DCpreviousError = DCError;

  output = DCProportional + DCIntegral + DCDerivative;

  // === OUTPUT SATURATION ===
  // Batasi output untuk DC motor (asumsi range: -255 hingga 255)
  if (output > 4095.0)
    output = 4095.0;
  else if (output < -4095.0)
    output = -4095.0;

  return output;
}
