#include <Arduino.h>
#include "DataStorage.h"
#include "LedControl.h"
#include "PID.h"
#include "Plant.h"
#include "PlantConfig.h"
#include "PlantESPNow.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc.h"

int x = 10;
void disableBrownout() { WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); }

void setup() {
  Serial.begin(115200);
  delay(500);

  disableBrownout();

  Serial.println("\n\n=== PLANT BOARD STARTUP ===");

  // 1. Inisialisasi LED
  initLedControl();

  // 2. Inisialisasi ESP-NOW DULU (harus sebelum timer)
  espnow_init();
  Serial.println("espnow init ok");

  // 3. Inisialisasi storage
  Serial.println("data init");
  DataStorage_INIT();
  Serial.println("data init ok");

  PrintStoredData();

  // 4. Inisialisasi encoder dan motor
  DCinitEncoder();
  DCstartMotorTimer();
  ACinitEncoder();
  ACstartMotorTimer();

  // 5. Start timers TERAKHIR (setelah ESP-NOW siap)
  DCstartEncoderTimer();
  ACstartEncoderTimer();
  UpdatePIDParam();

  Serial.println("=== SETUP COMPLETE ===");
  Serial.println();
}
void loop() {
  // Send data via ESP-NOW (safe in main loop, not in ISR)
  static unsigned long lastSend = 0;
  if (millis() - lastSend > DCREAD_INTERVAL) { // Send every 100ms
    lastSend = millis();
    if (DCMode) {
      sendTaggedFloat(MSG_DC_SPEED, DCGearboxRPM);
      DCprintEncoderData();
    }
    if (ACMode) {
      sendTaggedFloat(MSG_AC_SPEED, ACrpm);
      ACprintEncoderData();
    }
  }

}