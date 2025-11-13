#include "Plant.h"
#include <PlantConfig.h>
#include "PID.h"

// Definisi variabel global
volatile long DCencoder = 0;
volatile long DClastEncoder = 0;
volatile float DCrpm = 0;
volatile long DCpulseCount = 0;
volatile bool DCnewDataReady = false;
static int DClastStateA = LOW;
static int DClastStateB = LOW;

volatile unsigned long ACencoder;
volatile float ACrpm;
volatile bool ACnewDataReady = false;

// Variable internal
static hw_timer_t *a_timer = NULL;
static hw_timer_t *m_timer = NULL;

static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// ISR Handler
void IRAM_ATTR DConTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  DCpulseCount = DCencoder - DClastEncoder;
  DClastEncoder = DCencoder;
  DCrpm = (DCpulseCount * 6000.0 / DCREAD_INTERVAL) / PPR;
  DCnewDataReady = true;
  DC_PID(50, DCgetRPM(), DCREAD_INTERVAL / 1000.0);
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR AConTimer() {
  portENTER_CRITICAL_ISR(&timerMux);

  ACencoder = analogRead(AC_ENCODER_PIN);
  ACnewDataReady = true;
  AC_PID(50, ACgetRPM(), ACREAD_INTERVAL / 1000.0);

  portEXIT_CRITICAL_ISR(&timerMux);
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
    Serial.println(currentRpm);
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

float ACgetRPM() {
  ACrpm = (ACencoder / 4095.0) * 100;
  return ACrpm;
}

void ACprintEncoderData() {

  if (ACnewDataReady) {
    ACnewDataReady = false;
    Serial.print(" AC RPM: ");
    Serial.println(ACgetRPM());
  }
}