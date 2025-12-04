#include "Plant.h"
#include "PID.h"
#include "PlantESPNow.h"
#include <DataStorage.h>
#include <PlantConfig.h>

// Definisi variabel global
volatile long DCencoder = 0;
volatile long DClastEncoder = 0;
volatile float DCrpm = 0;
volatile float DCGearboxRPM = 0;
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
long waktu_awal_motor = 0;
bool ACVoltage = true;

// Kalman Filter variables for AC motor
float AC_estimate = 0.0;       // Estimated state (RPM)
float AC_error_estimate = 1.0; // Estimated error covariance
float AC_error_measure = 4.0; // Measurement error covariance (tuning parameter)
float AC_process_noise = 0.01; // Process noise covariance (tuning parameter)
float AC_kalman_gain = 0.0;    // Kalman gain
int ACMotorControlMode = 0;
bool dcspeedRequest = false;
bool acspeedRequest = false;
int dacspeed = 0;
// Variable internal
static hw_timer_t *a_timer = NULL;
static hw_timer_t *m_timer = NULL;

volatile float RPM_FACTOR = 60000.0 / DCREAD_INTERVAL / PPR;

static portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void DC_ProsesPID() {
  if (DCnewDataReady) {
    portENTER_CRITICAL_ISR(&timerMux);
    DCnewDataReady = false;
    DCpulseCount = DCencoder - DClastEncoder;
    DClastEncoder = DCencoder;
    DCrpm = (abs(DCpulseCount) * RPM_FACTOR);
    DCGearboxRPM = DCrpm / 200.0; // Assuming a gearbox ratio of 200:1
    portEXIT_CRITICAL_ISR(&timerMux);
    if (DCMode && PIDMODE) {
      DCsignalPWM = DC_PID(DCsetpoint, DCGearboxRPM, DCREAD_INTERVAL / 1000.0);
      DCmotorControl(DCDirection, DCsignalPWM);
    } else if (DCMode && !PIDMODE) {
      DCmotorControl(DCDirection, map(DCsetpoint, 0, 115, 0, 4095));
    } else if (!DCMode) {
      DCmotorControl(0, 0);
    }
  }
}

void AC_ProsesPID() {
  if (ACnewDataReady) {
    ACnewDataReady = false;
    if (ACMode && PIDMODE) {
      // Get RPM outside of critical section to avoid blocking
      float currentRPM = ACgetRPM();
      portENTER_CRITICAL_ISR(&timerMux);
      ACsignalPWM = AC_PID(ACsetpoint, currentRPM, ACREAD_INTERVAL / 1000.0);
      portEXIT_CRITICAL_ISR(&timerMux);
      ACmotorControl(true, ACsignalPWM, true, 0);
    } else if (ACMode && !PIDMODE) {
      ACmotorControl(true, map(ACsetpoint, 0, 1500, 0, 255), true, 0);
    } else {
      ACmotorControl(false, 0, true, 0);
      // Reset integral sum when motor is off to prevent windup
      ACintegralSum = 0.0;
      ACpreviousError = 0.0;
    }
  }
}

// ISR Handler
void IRAM_ATTR DConTimer() {
  if (dcspeedRequest) {
    DCnewDataReady = true;
    // Serial.print("DC Speed Request Enabled\n");
  } else {
    DCnewDataReady = false;
    DCmotorControl(0, 0);
  }
}

void IRAM_ATTR AConTimer() {
  if (acspeedRequest) {
    ACnewDataReady = true;
  } else {
    ACnewDataReady = false;
    ACmotorControl(false, 0, 1, 0);
  }
}
void IRAM_ATTR DChandleEncoderA() {
  portENTER_CRITICAL_ISR(&timerMux);
  // Serial.print("ISR A called\n");
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
  portEXIT_CRITICAL_ISR(&timerMux);
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
  // attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN),
  // DChandleEncoderB,CHANGE);
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
    long waktu_sekarang = millis() - waktu_awal_motor;
    long enc = DCencoder;
    float currentRpm = DCrpm;
    portEXIT_CRITICAL(&timerMux);
    sendTaggedFloat(MSG_DC_SPEED, DCGearboxRPM);
    sendTaggedFloat(MSG_TIMESTAMP, waktu_sekarang);
    Serial.print("DC RPM Data: ");
    Serial.print(waktu_sekarang);
    Serial.print(" , ");
    Serial.print(enc);
    Serial.print(" , ");
    Serial.print(DCGearboxRPM);
    Serial.print(" , ");
    Serial.print(DCsetpoint);
    Serial.print(" , ");
    Serial.print(DCError);
    Serial.print(" , ");
    if (PIDMODE) {
      Serial.print(DCsignalPWM);
    } else {
      Serial.print(map(DCsetpoint, 0, 150, 0, 4095));
    }
    Serial.print(" , ");
    Serial.print(DCkP);
    Serial.print(" , ");
    Serial.print(DCkI);
    Serial.print(" , ");
    Serial.println(DCkD);
  }
}

void ACinitEncoder() {
  pinMode(AC_ENCODER_PIN, INPUT);
  analogReadResolution(10);

  // Initialize Kalman Filter with first reading
  ACencoder = analogRead(AC_ENCODER_PIN);
  AC_estimate = (ACencoder * AC_READ_SCALING_FACTOR);
}

void ACstartEncoderTimer() {
  a_timer = timerBegin(1, 80, true);
  timerAlarmWrite(a_timer, ACREAD_INTERVAL * 1000, true);
  timerAttachInterrupt(a_timer, &AConTimer, true);
  timerAlarmEnable(a_timer);
}
void ACstartMotorTimer() {
  pinMode(AC_DAC_SOURCE_PIN, OUTPUT);
  pinMode(AC_DAC_VOLTAGE_SELECT_PIN, OUTPUT);
  ledcSetup(ACPWM_CHANNEL, ACPWM_FREQ, ACPWM_RESOLUTION);
  ledcAttachPin(AC_DAC2_PIN, ACPWM_CHANNEL);
  ledcWrite(ACPWM_CHANNEL, 0);
}
float ACgetRPM() {
  // Read raw sensor value
  ACencoder = analogRead(AC_ENCODER_PIN);
  float raw_rpm = (ACencoder * AC_READ_SCALING_FACTOR);

  // Kalman Filter implementation
  // Prediction step
  float predicted_estimate = AC_estimate;
  float predicted_error = AC_error_estimate + AC_process_noise;

  // Update step
  AC_kalman_gain = predicted_error / (predicted_error + AC_error_measure);
  AC_estimate =
      predicted_estimate + AC_kalman_gain * (raw_rpm - predicted_estimate);
  AC_error_estimate = (1.0 - AC_kalman_gain) * predicted_error;

  // Update the global ACrpm with filtered value
  ACrpm = AC_estimate;

  return ACrpm;
}

void ACprintEncoderData() {

  if (ACnewDataReady) {
    long waktu_sekarang = millis() - waktu_awal_motor;
    float currentRpm = ACgetRPM();
    sendTaggedFloat(MSG_AC_SPEED, currentRpm);
    sendTaggedFloat(MSG_TIMESTAMP, waktu_sekarang);

    // Serial.print("AC RPM Data: ");
    Serial.print(waktu_sekarang);
    Serial.print(" , ");
    Serial.print(currentRpm);
    Serial.print(" , ");
    Serial.print(ACsetpoint);
    Serial.print(" , ");
    Serial.print(ACError);
    Serial.print(" , ");
    if (PIDMODE) {
      Serial.print(ACsignalPWM);
    } else {
      Serial.print(dacspeed);
    }
    Serial.print(" , ");
    Serial.print(ACkP);
    Serial.print(" , ");
    Serial.print(ACkI);
    Serial.print(" , ");
    Serial.println(ACkD);
  }
}

void ACmotorControl(bool direction, long speed, bool voltage, int mode) {
  dacspeed = constrain(speed, 0, 255);

  digitalWrite(AC_DAC_VOLTAGE_SELECT_PIN, voltage);

  switch (mode) {
  case 0:
    if (dacspeed >= 255) {
      dacDisable(AC_DAC1_PIN);
      pinMode(AC_DAC1_PIN, OUTPUT);
      digitalWrite(AC_DAC1_PIN, HIGH);
    } else if (dacspeed <= 0) {
      dacDisable(AC_DAC1_PIN);
      pinMode(AC_DAC1_PIN, OUTPUT);
      digitalWrite(AC_DAC1_PIN, LOW);
    } else
      dacWrite(AC_DAC1_PIN, dacspeed);

    break;
  case 1:
    ledcWrite(ACPWM_CHANNEL, speed);

    break;
  case 2:
    break;
  }
}

void DCmotorControl(bool direction, long speed) {
  long var_speed = constrain(speed, 0, 4095);
  digitalWrite(MOTOR_DIR_PIN, direction);
  ledcWrite(PWM_CHANNEL, abs(var_speed));
}

// Kalman Filter utility functions for AC motor
void ACsetKalmanParams(float process_noise, float measure_noise) {
  AC_process_noise = process_noise;
  AC_error_measure = measure_noise;
}

void ACresetKalmanFilter() {
  ACencoder = analogRead(AC_ENCODER_PIN);
  AC_estimate = (ACencoder * AC_READ_SCALING_FACTOR);
  AC_error_estimate = 1.0;
  AC_kalman_gain = 0.0;
}
