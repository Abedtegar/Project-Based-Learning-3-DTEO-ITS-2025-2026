#include "Plant.h"
#include "PID.h"
#include <PlantConfig.h>

// Definisi variabel global
volatile long DCencoder = 0;
volatile long DClastEncoder = 0;
volatile float DCrpm = 0;
volatile long DCpulseCount = 0;
volatile bool DCnewDataReady = false;
static int DClastStateA = LOW;
static int DClastStateB = LOW;
volatile bool DCDirection = false;
volatile bool ACPID = false;
volatile bool DCPID = false;
volatile unsigned long ACencoder;
volatile float ACrpm;
volatile bool ACnewDataReady = false;

// Variable internal
static hw_timer_t *a_timer = NULL;
static hw_timer_t *m_timer = NULL;

static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ISR Handler
void IRAM_ATTR DConTimer() {
  if (DCPID) {
    portENTER_CRITICAL_ISR(&timerMux);
    DCpulseCount = DCencoder - DClastEncoder;
    DClastEncoder = DCencoder;
    DCrpm = (DCpulseCount * 6000.0 / DCREAD_INTERVAL) / PPR;
    DCsignalPWM = DC_PID(DCsetpoint, DCrpm, DCREAD_INTERVAL / 1000.0);
    DCmotorControl(DCDirection, DCsignalPWM);
    DCnewDataReady = true;
    portEXIT_CRITICAL_ISR(&timerMux);
  }
}

void IRAM_ATTR AConTimer() {
  if (ACPID) {
    portENTER_CRITICAL_ISR(&timerMux);
    ACsignalPWM = AC_PID(ACsetpoint, ACgetRPM(), ACREAD_INTERVAL / 1000.0);
    ACmotorControl(true, ACsignalPWM, true, 1);
    ACnewDataReady = true;
    portEXIT_CRITICAL_ISR(&timerMux);
  }
}

void IRAM_ATTR DChandleEncoderA() {
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA != DClastStateA) {
    if (stateA == HIGH) {
      if (stateB == LOW)
        DCencoder++;
      else
        DCencoder--;
    } else {
      if (stateB == HIGH)
        DCencoder++;
      else
        DCencoder--;
    }
    DClastStateA = stateA;
  }
}

void IRAM_ATTR DChandleEncoderB() {
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateB != DClastStateB) {
    if (stateB == HIGH) {
      if (stateA == HIGH)
        DCencoder++;
      else
        DCencoder--;
    } else {
      if (stateA == LOW)
        DCencoder++;
      else
        DCencoder--;
    }
    DClastStateB = stateB;
  }
}

void DCinitEncoder() {
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), DChandleEncoderA,
                  CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), DChandleEncoderB,
                  CHANGE);
}

void DCstartEncoderTimer() {
  m_timer = timerBegin(0, 80, true);
  timerAlarmWrite(m_timer, DCREAD_INTERVAL * 1000, true);
  timerAttachInterrupt(m_timer, &DConTimer, true);
  timerAlarmEnable(m_timer);
}

void DCstartMotorTimer() {
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0); // speed 0 - 255
}

float DCgetRPM() {
  float currentRpm = DCrpm;
  return currentRpm;
}

long DCgetEncoderCount() {
  long count = DCencoder;
  return count;
}

void DCresetEncoder() {
  DCencoder = 0;
  DClastEncoder = 0;
}

void DCprintEncoderData() {
  if (DCnewDataReady) {
    portENTER_CRITICAL(&timerMux);
    long enc = DCencoder;
    long pulse = DCpulseCount;
    float currentRpm = DCrpm;
    DCnewDataReady = false;
    portEXIT_CRITICAL(&timerMux);

    Serial.print("Encoder: ");
    Serial.print(enc);
    Serial.print(" | Pulse: ");
    Serial.print(pulse);
    Serial.print(" | RPM: ");
    Serial.print(currentRpm);
    Serial.print("| Setpoint: ");
    Serial.print(DCsetpoint);
    Serial.print("| PID Error: ");
    Serial.print(DCError);
    Serial.print("| PID PWM: ");
    Serial.print(DCsignalPWM);
    Serial.print("| KP:");
    Serial.print(DCkP);
    Serial.print("| KI:");
    Serial.print(DCkI);
    Serial.print("| KD:");
    Serial.print(DCkD);
    Serial.println("");
  }
}

void ACinitEncoder() {
  pinMode(AC_ENCODER_PIN, INPUT);
  analogReadResolution(12);
}

void ACstartEncoderTimer() {
  a_timer = timerBegin(1, 80, true);
  timerAlarmWrite(a_timer, ACREAD_INTERVAL * 1000, true);
  timerAttachInterrupt(a_timer, &AConTimer, true);
  timerAlarmEnable(a_timer);
}
void ACstartMotorTimer() {
  ledcSetup(ACPWM_CHANNEL, ACPWM_FREQ, ACPWM_RESOLUTION);
  ledcAttachPin(AC_DAC2_PIN, ACPWM_CHANNEL);
  ledcWrite(ACPWM_CHANNEL, 0); // speed 0 - 255
}
float ACgetRPM() {
  ACencoder = analogRead(AC_ENCODER_PIN);
  ACrpm = (ACencoder / 4095.0) * 100;
  return ACrpm;
}


void ACprintEncoderData() {

  if (ACnewDataReady) {
    ACnewDataReady = false;
    Serial.print(" AC RPM: ");
    Serial.print(ACgetRPM());
    Serial.print("| Setpoint: ");
    Serial.print(ACsetpoint);
    Serial.print("| PID Error: ");
    Serial.print(ACError);
    Serial.print("| PID PWM: ");
    Serial.print(ACsignalPWM);
    Serial.print("| KP:");
    Serial.print(ACkP);
    Serial.print("| KI:");
    Serial.print(ACkI);
    Serial.print("| KD:");
    Serial.print(ACkD);
    Serial.println("");
  }
}

void ACmotorControl(bool direction, long speed, bool voltage, int mode) {
  int dacspeed = constrain(speed, 0, 255);
  digitalWrite(AC_DAC_VOLTAGE_SELECT_PIN, voltage);

  switch (mode) {
  case 0:
    ledcWrite(ACPWM_CHANNEL, speed);
    break;
  case 1:
    dacWrite(AC_DAC1_PIN, dacspeed);
    break;
  case 2:
    break;
  }
}

void DCmotorControl(bool direction, long speed) {
  ledcWrite(PWM_CHANNEL, speed);
  digitalWrite(MOTOR_DIR_PIN, direction);
}