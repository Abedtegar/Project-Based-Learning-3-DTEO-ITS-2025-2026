#include "PlantConfig.h"
#include "PlantESPNow.h"
#include <Arduino.h>
#include "LedControl.h"


void setup() {
  initLedControl();
}
void loop() {
  blinkLed(LED_WIFI, 1000);

}