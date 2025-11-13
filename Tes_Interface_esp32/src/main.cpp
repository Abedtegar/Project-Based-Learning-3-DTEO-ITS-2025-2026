// ========================================
// CONTOH PENGGUNAAN CONTROL MENU SYSTEM
// Rename file ini menjadi main.cpp untuk menggunakannya
// ========================================

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_now.h>

#include "ControlMenuSystem.h"
#include "EncoderControl.h"
#include "NetworkManager.h"
#include "PinConfig.h"

// ========================================
// GLOBAL OBJECTS
// ========================================

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
ControlMenuSystem menuSystem(&tft);
EncoderControl encoder(ENCODER_CLK, ENCODER_DT, ENCODER_SW);
NetworkManager network;

// ========================================
// NETWORK CALLBACKS - Data dari PLANT
// ========================================

void onPlantDataReceived(EscalatorDataPacket data) {
  // Update data ke menu system
  Direction dir = (data.direction == 0) ? DIR_FORWARD : DIR_REVERSE;
  menuSystem.updateEscalatorData(data.currentSpeed, dir);

  // Log data yang diterima
  Serial.println("\n[PLANT DATA]");
  Serial.printf("  Speed: %.2f RPM\n", data.currentSpeed);
  Serial.printf("  Direction: %s\n", data.direction == 0 ? "FWD" : "REV");
  Serial.printf("  Mode: %s\n", data.mode == 0 ? "MANUAL" : "AUTO");
  Serial.printf("  Running: %s\n", data.isRunning ? "YES" : "NO");
}

void onMotorDataReceived(MotorACDataPacket data) {
  // Update data ke menu system
  menuSystem.updateMotorACData(data.currentSpeed);

  // Log data yang diterima
  Serial.println("\n[MOTOR DATA]");
  Serial.printf("  Speed: %.2f RPM\n", data.currentSpeed);
  Serial.printf("  Mode: %s\n", data.mode == 0 ? "MANUAL" : "AUTO");
  Serial.printf("  Running: %s\n", data.isRunning ? "YES" : "NO");
}

void onNetworkSendComplete(const uint8_t *mac, bool success) {
  if (success) {
    Serial.println("[SEND] Command sent successfully");
  } else {
    Serial.println("[SEND] Command failed to send");
  }
}

// ========================================
// ENCODER CALLBACKS
// ========================================

void onEncoderRotate(int direction) {
  menuSystem.navigate(direction);
  // LED feedback saat navigasi
  digitalWrite(LED_STATUS, HIGH);
  delay(10);
  digitalWrite(LED_STATUS, LOW);
}

void onEncoderClick() { menuSystem.select(); }

void onEncoderLongPress() { menuSystem.back(); }

// ========================================
// SETUP & LOOP
// ========================================

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);

  Serial.println("\n========================================");
  Serial.println("Control Interface v2.0");
  Serial.println("========================================");

  // Init LCD PERTAMA (sebelum pin lain)
  Serial.println("[LCD] Initializing display...");

  // Reset display
  pinMode(TFT_RST, OUTPUT);
  digitalWrite(TFT_RST, HIGH);
  delay(10);
  digitalWrite(TFT_RST, LOW);
  delay(10);
  digitalWrite(TFT_RST, HIGH);
  delay(150);

  // Init SPI dan LCD
  SPI.begin();
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(0); // 0 = portrait (vertical)
  tft.fillScreen(ST77XX_BLACK);

  Serial.println("[LCD] Display initialized");

  // Test LCD - tampilkan teks sederhana
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  tft.println("Initializing...");

  delay(500);

  // Init LED Status (sekarang sudah aman, tidak bentrok)
  pinMode(LED_STATUS, OUTPUT);
  digitalWrite(LED_STATUS, LOW);

  // Init Menu
  Serial.println("[MENU] Initializing menu system...");
  menuSystem.begin();

  // Init Encoder
  encoder.begin();
  encoder.onRotate(onEncoderRotate);
  encoder.onClick(onEncoderClick);
  encoder.onLongPress(onEncoderLongPress);

  // Init Network Manager
  Serial.println("\n[NETWORK] Initializing...");

  // Init WiFi AP Mode
  if (network.beginWiFiAP(WIFI_AP_SSID, WIFI_AP_PASSWORD)) {
    String ipAddr = network.getAPIP();
    menuSystem.updateNetworkStatus(true, WIFI_AP_SSID, ipAddr,
                                   network.getConnectedClients());
  }

  // Init ESP-NOW
  if (network.beginESPNow()) {
    menuSystem.updateESPNowStatus(true, 0);

    // Set Plant MAC Address dari PinConfig.h
    uint8_t plantMac[] = PLANT_MAC;
    network.setEscalatorMAC(plantMac);

    // Add Plant sebagai peer
    if (network.addEscalatorPeer()) {
      Serial.println("[NETWORK] Plant peer added");
      menuSystem.updateESPNowStatus(true, 1);
    }

    // Set callbacks untuk menerima data
    network.onReceiveEscalatorData(onPlantDataReceived);
    network.onReceiveMotorACData(onMotorDataReceived);
    network.onSendComplete(onNetworkSendComplete);
  }

  // Print network status
  network.printStatus();

  Serial.println("\n[SYSTEM] Setup Complete!");
  Serial.println("=========================================\n");
}

void loop() {
  // Update encoder dan menu
  encoder.update();
  menuSystem.update();

  // Update jumlah connected clients di menu
  static unsigned long lastNetworkUpdate = 0;
  if (millis() - lastNetworkUpdate > 2000) {
    int clients = network.getConnectedClients();
    menuSystem.updateNetworkStatus(true, WIFI_AP_SSID, network.getAPIP(),
                                   clients);
    lastNetworkUpdate = millis();
  }

  // ========================================
  // AUTO SEND COMMAND KE PLANT (OPTIONAL)
  // Comment block ini jika tidak ingin auto-send
  // ========================================
  /*
  static unsigned long lastCommandSend = 0;
  static bool commandEnabled = false; // Flag untuk enable sending

  // Enable sending setelah 10 detik (untuk memastikan Plant sudah siap)
  if (millis() > 10000 && !commandEnabled) {
    commandEnabled = true;
    Serial.println("[SEND] Command sending enabled");
  }

  if (commandEnabled && millis() - lastCommandSend > 10000) {
    // Cek apakah ESP-NOW ready
    if (!network.isESPNowReady()) {
      Serial.println("[SEND] ESP-NOW not ready, skipping...");
      lastCommandSend = millis();
      return;
    }

    // Contoh: Kirim command berdasarkan setting di menu
    EscalatorData settings = menuSystem.getEscalatorSettings();

    EscalatorCommandPacket cmd = {};
    cmd.commandType = 1; // set_speed
    cmd.mode = settings.mode;
    cmd.direction = settings.direction;
    cmd.targetSpeed = settings.targetSpeed;
    cmd.pidKp = settings.pid.kp;
    cmd.pidKi = settings.pid.ki;
    cmd.pidKd = settings.pid.kd;
    cmd.pidSetpoint = settings.pid.setPoint;

    // Kirim command ke Plant (sudah di-enable)
    Serial.println("[SEND] Sending command to Plant...");
    bool success = network.sendEscalatorCommand(cmd);

    if (!success) {
      Serial.println("[SEND] Failed! Check if Plant is online and MAC is
  correct.");
    }

    lastCommandSend = millis();
  }
  */
  // End of auto-send block

  // Simulasi data untuk testing (comment jika sudah terima data real)
  /*
  static unsigned long lastSim = 0;
  if (millis() - lastSim > 500) {
    static float escSpeed = 50;
    escSpeed += random(-5, 6);
    escSpeed = constrain(escSpeed, 0, 100);
    menuSystem.updateEscalatorData(escSpeed, DIR_FORWARD);

    static float motorSpeed = 1500;
    motorSpeed += random(-100, 101);
    motorSpeed = constrain(motorSpeed, 0, 3000);
    menuSystem.updateMotorACData(motorSpeed);

    lastSim = millis();
  }
  */

  delay(10);
}
