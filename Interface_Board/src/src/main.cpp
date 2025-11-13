

// // ========================================
// // CONTOH PENGGUNAAN CONTROL MENU SYSTEM
// // Rename file ini menjadi main.cpp untuk menggunakannya
// // ========================================

// #include <Adafruit_GFX.h>
// #include <Adafruit_ST7735.h>
// #include <Arduino.h>
// #include <SPI.h>

// // Platform-specific includes
// #ifdef ESP32
// #include <WiFi.h>
// #include <esp_now.h>
// #elif defined(ESP8266)
// #include <ESP8266WiFi.h>
// #include <espnow.h>
// #endif

// #include "ControlMenuSystem.h"
// #include "EncoderControl.h"
// #include "NetworkManager.h"
// #include "PinConfig.h"

// // ========================================
// // GLOBAL OBJECTS
// // ========================================

// Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
// ControlMenuSystem menuSystem(&tft);
// EncoderControl encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW);
// NetworkManager network;

// // ========================================
// // NETWORK CALLBACKS - Data dari PLANT
// // ========================================

// void onPlantDataReceived(EscalatorDataPacket data) {
//   // Update data ke menu system (full packet)
//   menuSystem.updateEscalatorFromPacket(data);

//   // Log data yang diterima
//   Serial.println("\n[PLANT DATA]");
//   Serial.printf("  Speed: %.2f RPM\n", data.currentSpeed);
//   Serial.printf("  Direction: %s\n", data.direction == 0 ? "FWD" : "REV");
//   Serial.printf("  Mode: %s\n", data.mode == 0 ? "MANUAL" : "AUTO");
//   Serial.printf("  Running: %s\n", data.isRunning ? "YES" : "NO");
// }

// void onMotorDataReceived(MotorACDataPacket data) {
//   // Update data ke menu system
//   menuSystem.updateMotorACData(data.currentSpeed);

//   // Log data yang diterima
//   Serial.println("\n[MOTOR DATA]");
//   Serial.printf("  Speed: %.2f RPM\n", data.currentSpeed);
//   Serial.printf("  Mode: %s\n", data.mode == 0 ? "MANUAL" : "AUTO");
//   Serial.printf("  Running: %s\n", data.isRunning ? "YES" : "NO");
// }

// void onNetworkSendComplete(const uint8_t *mac, bool success) {
//   if (success) {
//     Serial.println("[SEND] Command sent successfully");
//   } else {
//     Serial.println("[SEND] Command failed to send");
//   }
// }

// // ========================================
// // ENCODER CALLBACKS
// // ========================================

// void onEncoderRotate(int direction) {
//   menuSystem.navigate(direction);
//   // LED feedback saat navigasi
//   digitalWrite(LED_STATUS, HIGH);
//   delay(10);
//   digitalWrite(LED_STATUS, LOW);
// }

// void onEncoderClick() { menuSystem.select(); }

// void onEncoderLongPress() { menuSystem.back(); }

// // ========================================
// // SETUP & LOOP
// // ========================================

// void setup() {
//   Serial.begin(SERIAL_BAUD);
//   delay(1000);

//   Serial.println("\n========================================");
//   Serial.println("Control Interface v2.0");
//   Serial.println("========================================");

//   // Init LCD PERTAMA (sebelum pin lain)
//   Serial.println("[LCD] Initializing display...");

//   // Init SPI dan LCD (simplified untuk ESP8266)
//   tft.initR(INITR_BLACKTAB); // Init ST7735S chip, black tab

// #ifdef ESP8266
//   tft.setRotation(0); // 3 = landscape mode untuk ESP8266
// #else
//   tft.setRotation(0); // 0 = portrait (vertical) untuk ESP32
// #endif

//   tft.fillScreen(ST77XX_BLACK);

//   Serial.println("[LCD] Display initialized");

//   // Test LCD - tampilkan teks sederhana
//   tft.setTextColor(ST77XX_WHITE);
//   tft.setTextSize(1);
//   tft.setCursor(10, 10);
//   tft.println("Initializing...");

//   delay(500);

//   // Init LED Status (sekarang sudah aman, tidak bentrok)
//   pinMode(LED_STATUS, OUTPUT);
//   digitalWrite(LED_STATUS, LOW);

//   // Init Menu
//   Serial.println("[MENU] Initializing menu system...");
//   menuSystem.begin();

//   // Provide NetworkManager to menu system so it can send commands
//   menuSystem.setNetworkManager(&network);

//   // Init Encoder
//   encoder.begin();
//   encoder.onRotate(onEncoderRotate);
//   encoder.onClick(onEncoderClick);
//   encoder.onLongPress(onEncoderLongPress);

//   // Init Network Manager
//   Serial.println("\n[NETWORK] Initializing...");

//   // Print local MAC address FIRST
//   Serial.println("========================================");
//   Serial.println("INTERFACE BOARD MAC ADDRESS");
//   Serial.println("========================================");
//   Serial.print("MAC Address: ");
//   Serial.println(WiFi.macAddress());
//   Serial.println("========================================");
//   Serial.println("Copy this MAC to PlantConfig.h INTERFACE_MAC!");
//   Serial.println("========================================\n");

//   // Init WiFi AP Mode
//   if (network.beginWiFiAP(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
//     String ipAddr = network.getAPIP();
//     menuSystem.updateNetworkStatus(true, WIFI_AP_SSID, ipAddr,
//                                    network.getConnectedClients());
//   }

//   // Init ESP-NOW
//   if (network.beginESPNow()) {
//     menuSystem.updateESPNowStatus(true, 0);

//     // Set Plant MAC Address dari PinConfig.h
//     uint8_t plantMac[] = PLANT_MAC;
//     network.setEscalatorMAC(plantMac);

//     // Add Plant sebagai peer
//     if (network.addEscalatorPeer()) {
//       Serial.println("[NETWORK] Plant peer added");
//       menuSystem.updateESPNowStatus(true, 1);
//     }

//     // Set callbacks untuk menerima data
//     network.onReceiveEscalatorData(onPlantDataReceived);
//     network.onReceiveMotorACData(onMotorDataReceived);
//     network.onSendComplete(onNetworkSendComplete);
//   }

//   // Print network status
//   network.printStatus();

//   Serial.println("\n[SYSTEM] Setup Complete!");
//   Serial.println("=========================================\n");
// }

// void loop() {
//   // Update encoder dan menu
//   encoder.update();
//   menuSystem.update();

//   // Update jumlah connected clients di menu
//   static unsigned long lastNetworkUpdate = 0;
//   if (millis() - lastNetworkUpdate > 2000) {
//     int clients = network.getConnectedClients();
//     menuSystem.updateNetworkStatus(true, WIFI_AP_SSID, network.getAPIP(),
//                                    clients);
//     lastNetworkUpdate = millis();
//   }

//   // ========================================
//   // AUTO SEND COMMAND KE PLANT (OPTIONAL)
//   // Comment block ini jika tidak ingin auto-send
//   // ========================================
//   /*
//   static unsigned long lastCommandSend = 0;
//   static bool commandEnabled = false; // Flag untuk enable sending

//   // Enable sending setelah 10 detik (untuk memastikan Plant sudah siap)
//   if (millis() > 10000 && !commandEnabled) {
//     commandEnabled = true;
//     Serial.println("[SEND] Command sending enabled");
//   }

//   if (commandEnabled && millis() - lastCommandSend > 10000) {
//     // Cek apakah ESP-NOW ready
//     if (!network.isESPNowReady()) {
//       Serial.println("[SEND] ESP-NOW not ready, skipping...");
//       lastCommandSend = millis();
//       return;
//     }

//     // Contoh: Kirim command berdasarkan setting di menu
//     EscalatorData settings = menuSystem.getEscalatorSettings();

//     EscalatorCommandPacket cmd = {};
//     cmd.commandType = 1; // set_speed
//     cmd.mode = settings.mode;
//     cmd.direction = settings.direction;
//     cmd.targetSpeed = settings.targetSpeed;
//     cmd.pidKp = settings.pid.kp;
//     cmd.pidKi = settings.pid.ki;
//     cmd.pidKd = settings.pid.kd;
//     cmd.pidSetpoint = settings.pid.setPoint;

//     // Kirim command ke Plant (sudah di-enable)
//     Serial.println("[SEND] Sending command to Plant...");
//     bool success = network.sendEscalatorCommand(cmd);

//     if (!success) {
//       Serial.println("[SEND] Failed! Check if Plant is online and MAC is
//   correct.");
//     }

//     lastCommandSend = millis();
//   }
//   */
//   // End of auto-send block

//   // Simulasi data untuk testing (comment jika sudah terima data real)
//   /*
//   static unsigned long lastSim = 0;
//   if (millis() - lastSim > 500) {
//     static float escSpeed = 50;
//     escSpeed += random(-5, 6);
//     escSpeed = constrain(escSpeed, 0, 100);
//     menuSystem.updateEscalatorData(escSpeed, DIR_FORWARD);

//     static float motorSpeed = 1500;
//     motorSpeed += random(-100, 101);
//     motorSpeed = constrain(motorSpeed, 0, 3000);
//     menuSystem.updateMotorACData(motorSpeed);

//     lastSim = millis();
//   }
//   */

//   delay(10);
// }
// */

// ========================================
// ENCODER DEBUG & LOGGING TEST PROGRAM
// Upload this to test encoder reading
// ========================================

#include "PinConfig.h"
#include <Arduino.h>

// Encoder pins
const int CLK_PIN = ENCODER_CLK;
const int DT_PIN = ENCODER_DT;
const int SW_PIN = ENCODER_SW;

// Encoder state
volatile int encoderPos = 0;
volatile int triggerCount = 0;
volatile unsigned long lastTriggerTime = 0;

// Logging variables
int lastPrintedPos = 0;
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 100; // Print every 100ms

// ========================================
// SIMPLE ENCODER ISR
// ========================================

void IRAM_ATTR encoderISR() {
  unsigned long now = millis();

  // Read pins
  int clkState = digitalRead(CLK_PIN);
  int dtState = digitalRead(DT_PIN);

  // Count triggers for debugging
  triggerCount++;

  // Debounce: minimum 2ms between valid reads
  if (now - lastTriggerTime < 2) {
    return;
  }

  // Simple edge detection
  static int lastCLK = HIGH;

  if (lastCLK == HIGH && clkState == LOW) {
    // Falling edge on CLK
    if (dtState == HIGH) {
      encoderPos++;
      // Serial.print("+"); // Visual feedback
    } else {
      encoderPos--;
      // Serial.print("-"); // Visual feedback
    }
    lastTriggerTime = now;
  }

  int clkRaw = digitalRead(CLK_PIN);
  int dtRaw = digitalRead(DT_PIN);
  int swRaw = digitalRead(SW_PIN);
  Serial.print("encoder");
  Serial.print(encoderPos);
  Serial.printf("Pin States: CLK=%d, DT=%d, SW=%d\n", clkRaw, dtRaw, swRaw);
  lastCLK = clkState;
}

// ========================================
// BUTTON ISR
// ========================================

void IRAM_ATTR buttonISR() {
  static unsigned long lastButtonTime = 0;
  unsigned long now = millis();

  if (now - lastButtonTime > 200) {
    bool pressed = !digitalRead(SW_PIN);
    if (pressed) {
      // Serial.println("\n[BUTTON PRESSED]");
      encoderPos = 0; // Reset position on button press
      // Serial.println("Position RESET to 0");
    }
    lastButtonTime = now;
  }
}

// ========================================
// SETUP
// ========================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("ENCODER DEBUG & LOGGING TEST");
  Serial.println("========================================");
  Serial.printf("CLK Pin: %d\n", CLK_PIN);
  Serial.printf("DT Pin: %d\n", DT_PIN);
  Serial.printf("SW Pin: %d\n", SW_PIN);
  Serial.println("========================================");
  Serial.println("Rotate encoder to test...");
  Serial.println("Press button to reset position");
  Serial.println("+ = Clockwise, - = Counter-clockwise");
  Serial.println("========================================\n");

  // Setup pins
  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP);

  delay(50);

  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, RISING);
  attachInterrupt(digitalPinToInterrupt(SW_PIN), buttonISR, RISING);

  Serial.println("Ready! Start rotating encoder...\n");
}

// ========================================
// LOOP
// ========================================

void loop() {
  unsigned long now = millis();

  // // Periodic detailed logging
  // if (now - lastPrintTime >= PRINT_INTERVAL) {
  //   // Only print if position changed
  //   if (encoderPos != lastPrintedPos) {
  //     int diff = encoderPos - lastPrintedPos;

  //     // Serial.println(); // New line after +/- symbols
  //     // Serial.println("----------------------------------------");
  //     // Serial.printf("Time: %lu ms\n", now);
  //     // Serial.printf("Position: %d (Change: %+d)\n", encoderPos, diff);
  //     // Serial.printf("Total Triggers: %d\n", triggerCount);
  //     // Serial.printf("Trigger Rate: %.1f Hz\n", triggerCount / (now /
  //     1000.0));

  //     // Read raw pin states

  //     // Serial.println("----------------------------------------\n");

  //     lastPrintedPos = encoderPos;
  //   }

  //   lastPrintTime = now;
  // }

  // Continuous raw pin monitoring (optional, for very detailed debug)
  // Uncomment below to see raw pin changes in real-time
  /*
  static int lastCLK = -1, lastDT = -1, lastSW = -1;
  int clk = digitalRead(CLK_PIN);
  int dt = digitalRead(DT_PIN);
  int sw = digitalRead(SW_PIN);

  if (clk != lastCLK || dt != lastDT || sw != lastSW) {
    Serial.printf("RAW: CLK=%d DT=%d SW=%d Time=%lu\n", clk, dt, sw, millis());
    lastCLK = clk;
    lastDT = dt;
    lastSW = sw;
  }
  */

  delay(10);
}
