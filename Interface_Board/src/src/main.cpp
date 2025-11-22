#include "ControlMenuSystem.h"
#include "NetworkManager.h"
#include "PinConfig.h"
#include <Arduino.h>
#include <EncoderControl.h>

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {

  Serial.begin(115200);
  espnow_init();
  DCinitEncoder();
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0);
  menuInit(&tft);
  sendTaggedFloat(ESP_RESTART, 1);
  Serial.println("\n=== ESP-NOW Ready (Interface_Board) ===");
}

void loop() {
  static long lastEncoder = 0;

  // Handle encoder scroll
  long currentEncoder = DCgetEncoderCount();
  if (currentEncoder != lastEncoder) {
    Serial.print("Encoder Count: ");
    Serial.print(currentEncoder);
    Serial.print(" | Mode: ");
    Serial.println(getEncoderMode() == ENCODER_UP ? "UP" : "DOWN");

    int direction = (currentEncoder > lastEncoder) ? 1 : -1;
    menuNavigate(direction);
    lastEncoder = currentEncoder;
    delay(50); // Debounce scroll
  }

  // Handle button clicks (single, double, long)
  ButtonClick click = getButtonClick();

  switch (click) {
  case CLICK_SINGLE:
    // Single click = SELECT
    menuSelect();
    delay(100);
    break;

  case CLICK_DOUBLE:
    // Double click = Toggle encoder mode (UP/DOWN) or motor run/stop in graph
    if ((g_currentMenu == 1 && g_escSubMenu == 0) ||
        (g_currentMenu == 2 && g_motorSubMenu == 0)) {
      // In graph mode: toggle motor run/stop
      menuDoubleClick();
    } else {
      // Other modes: toggle encoder direction
      toggleEncoderMode();
    }
    delay(100);
    break;

  case CLICK_LONG:
    // Long click = BACK
    menuBack();
    while (buttonPressed()) {
      delay(10);
    } // Wait release
    delay(100);
    break;

  case CLICK_NONE:
  default:
    // No action
    break;
  }

  menuUpdate();
  delay(10); // Prevent excessive polling
}
