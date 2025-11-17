#include "PlantConfig.h"
#include <Arduino.h>
#include "LedControl.h"
#include "PID.h"
#include "Plant.h"
#include "PlantESPNow.h"
#include "DataStorage.h"


void setup() {
  Serial.begin(115200);
  initLedControl();
  DCinitEncoder();
  DCstartEncoderTimer();
  DCstartMotorTimer();
  ACinitEncoder();
  ACstartEncoderTimer();
  ACstartMotorTimer();
  DCsetpoint = 50;
  ACPID = true;
  // DCPID = true;
  DataStorage_INIT();
  PrintStoredData();

  pinMode(AC_DAC_SOURCE_PIN, OUTPUT);
  pinMode(AC_DAC_VOLTAGE_SELECT_PIN, OUTPUT);
  digitalWrite(AC_DAC_SOURCE_PIN, LOW);
  digitalWrite(AC_DAC_VOLTAGE_SELECT_PIN, HIGH);
  StoreData("pid_dc", "kP", 2.5);
  PrintStoredData();
}
void loop() {
  // delay(10);
  // DCprintEncoderData();
//  ACprintEncoderData();
}