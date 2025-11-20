#include "DataStorage.h"
#include "LedControl.h"
#include "PID.h"
#include "Plant.h"
#include "PlantConfig.h"
#include "PlantESPNow.h"
#include <Arduino.h>
#include <nvs_flash.h>

int x = 10;
void setup() {
  Serial.begin(115200);
  delay(500);

  // Inisialisasi NVS
 
  // Inisialisasi komponen
  initLedControl();
  DCinitEncoder();
  DCstartEncoderTimer();
  DCstartMotorTimer();
  ACinitEncoder();
  ACstartEncoderTimer();
  ACstartMotorTimer();

  // Load konfigurasi PID
  DataStorage_INIT();
  PrintStoredData();

  // Inisialisasi ESP-NOW
  espnow_init();

  // Setup pin AC motor
  pinMode(AC_DAC_SOURCE_PIN, OUTPUT);
  pinMode(AC_DAC_VOLTAGE_SELECT_PIN, OUTPUT);
  digitalWrite(AC_DAC_SOURCE_PIN, LOW);
  digitalWrite(AC_DAC_VOLTAGE_SELECT_PIN, HIGH);
}
void loop() {
  ACprintEncoderData();
  DCprintEncoderData();
}