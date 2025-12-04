#include "PID.h"
#include "PlantESPNow.h"
#include <PlantConfig.h>

volatile float DCpreviousError = 0.0f;
volatile float DCerrorSum = 0.0f;
volatile float DCError = 0.0f;
volatile float DCsignalPWM = 0.0f;
volatile float DCkP = 1.5f;
volatile float DCkI = 0.1f;
volatile float DCkD = 0.05f;
volatile float DCsetpoint = 50.0f;

// Tambahkan batas minimum PWM untuk anti-stall
#define DC_PWM_MIN 800.0f
#define AC_PWM_MIN 800.0f

volatile float ACpreviousError = 0.0f;
volatile float ACerrorSum = 0.0f;
volatile float ACError = 0.0f;
volatile float ACsignalPWM = 0.0f;
volatile float ACkP = 2.0f;
volatile float ACkI = 0.08f;
volatile float ACkD = 0.04f;
volatile float ACsetpoint = 0.0f;
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

  output = ACProportional + ACIntegral + ACDerivative;

  // === OUTPUT SATURATION & ANTI-STALL ===
  if (output > 255.0)
    output = 255.0;
  else if (output < 0.0)
    output = 0.0;
  // Anti-stall: jika output di antara 0 dan AC_PWM_MIN, naikkan ke AC_PWM_MIN
  else if (output > 0.0 && output < AC_PWM_MIN)
    output = AC_PWM_MIN;

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

  output = (DCProportional + DCIntegral + DCDerivative) * 10;

  // === OUTPUT SATURATION & ANTI-STALL ===
  if (output > 4095.0)
    output = 4095.0;
  else if (output < -4095.0)
    output = -4095.0;
  // Anti-stall: jika output positif tapi kecil, naikkan ke DC_PWM_MIN
  else if (output > 0.0 && output < DC_PWM_MIN)
    output = DC_PWM_MIN;
  // Jika output negatif tapi kecil, turunkan ke -DC_PWM_MIN
  else if (output < 0.0 && output > -DC_PWM_MIN)
    output = -DC_PWM_MIN;

  return output;
}

void UpdatePIDParam() {
  sendTaggedFloat(MSG_DC_KP, DCkP);
  Serial.print("DCkP terkirim ");
  delay(10);
  sendTaggedFloat(MSG_DC_KI, DCkI);
  Serial.print("DCkI terkirim ");
  delay(10);
  sendTaggedFloat(MSG_DC_KD, DCkD);
  Serial.print("DCkD terkirim ");
  delay(10);
  sendTaggedFloat(MSG_DC_Setpoint, DCsetpoint);
  Serial.print("DCsetpoint terkirim ");
  delay(10);
  sendTaggedFloat(MSG_AC_KP, ACkP);
  Serial.print("ACkP terkirim ");
  delay(10);
  sendTaggedFloat(MSG_AC_KI, ACkI);
  Serial.print("ACkI terkirim ");
  delay(10);
  sendTaggedFloat(MSG_AC_KD, ACkD);
  Serial.print("ACkD terkirim ");
  delay(10);
  sendTaggedFloat(MSG_AC_Setpoint, ACsetpoint);
  Serial.print("ACsetpoint terkirim ");
  delay(10);
}
