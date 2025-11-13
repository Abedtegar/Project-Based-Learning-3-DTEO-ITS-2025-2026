#include "PlantESPNow.h"

// Initialize static instance
PlantESPNow *PlantESPNow::instance = nullptr;

// ========================================
// CONSTRUCTOR
// ========================================

PlantESPNow::PlantESPNow() {
  espnowInitialized = false;
  peerAdded = false;

  memset(gameboardMAC, 0, 6);

  onCommandReceived = nullptr;
  onSendComplete = nullptr;

  instance = this;
}

// ========================================
// INITIALIZATION
// ========================================

bool PlantESPNow::begin() {
  Serial.println("[ESP-NOW] Initializing...");

  // Set WiFi mode to STA
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100); // Give time for WiFi to initialize

  // Set WiFi channel to 1 (same as ESP8266 AP)
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  Serial.print("[ESP-NOW] Plant MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("[ESP-NOW] WiFi Channel: 1");

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init Failed!");
    return false;
  }

  espnowInitialized = true;
  Serial.println("[ESP-NOW] Initialized Successfully");

  // Register callbacks
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  return true;
}

bool PlantESPNow::setGameboardMAC(uint8_t mac[6]) {
  memcpy(gameboardMAC, mac, 6);
  Serial.print("[ESP-NOW] GameBoard MAC set to: ");
  printMAC(gameboardMAC);
  return true;
}

bool PlantESPNow::addGameboardPeer() {
  // Check if MAC is valid
  bool isEmpty = true;
  for (int i = 0; i < 6; i++) {
    if (gameboardMAC[i] != 0xFF && gameboardMAC[i] != 0x00) {
      isEmpty = false;
      break;
    }
  }

  if (isEmpty) {
    Serial.println("[ESP-NOW] ERROR: GameBoard MAC not set or invalid!");
    Serial.println("[ESP-NOW] Please update INTERFACE_MAC in PlantConfig.h");
    return false;
  }

  // Check if peer already exists
  if (esp_now_is_peer_exist(gameboardMAC)) {
    Serial.println("[ESP-NOW] Peer already exists");
    peerAdded = true;
    return true;
  }

  // Add peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, gameboardMAC, 6);
  peerInfo.channel = 1; // Set to channel 1 to match ESP8266 AP
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA; // Explicitly set interface

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ESP-NOW] Failed to add peer!");
    return false;
  }

  Serial.println("[ESP-NOW] GameBoard peer added successfully");
  Serial.println("[ESP-NOW] Peer channel: 1");
  peerAdded = true;
  return true;
}

// ========================================
// CALLBACKS SETUP
// ========================================

void PlantESPNow::onReceiveCommand(
    void (*callback)(EscalatorCommandPacket cmd)) {
  onCommandReceived = callback;
}

void PlantESPNow::onSendStatus(void (*callback)(bool success)) {
  onSendComplete = callback;
}

// ========================================
// ESP-NOW STATIC CALLBACKS
// ========================================

void PlantESPNow::onDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                             int data_len) {
  if (!instance)
    return;

  Serial.print("[ESP-NOW] Data received from GameBoard, size: ");
  Serial.println(data_len);

  // Check if it's a command packet
  if (data_len == sizeof(EscalatorCommandPacket)) {
    EscalatorCommandPacket cmd;
    memcpy(&cmd, data, sizeof(cmd));

    Serial.println("[ESP-NOW] Command packet received:");
    Serial.printf("  Type: %d\n", cmd.commandType);
    Serial.printf("  Mode: %s\n", cmd.mode == 0 ? "MANUAL" : "AUTO");
    Serial.printf("  Target Speed: %.2f\n", cmd.targetSpeed);

    if (instance->onCommandReceived) {
      instance->onCommandReceived(cmd);
    }
  } else {
    Serial.println("[ESP-NOW] Unknown packet type");
  }
}

void PlantESPNow::onDataSent(const uint8_t *mac_addr,
                             esp_now_send_status_t status) {
  if (!instance)
    return;

  bool success = (status == ESP_NOW_SEND_SUCCESS);

  // Simplified logging
  if (!success) {
    Serial.println("[ESP-NOW] Send FAILED");
  }

  if (instance->onSendComplete) {
    instance->onSendComplete(success);
  }
}

// ========================================
// SEND DATA
// ========================================

bool PlantESPNow::sendData(EscalatorDataPacket data) {
  if (!espnowInitialized) {
    Serial.println("[ESP-NOW] ERROR: Not initialized!");
    return false;
  }

  if (!peerAdded) {
    Serial.println("[ESP-NOW] ERROR: Peer not added!");
    return false;
  }

  data.timestamp = millis();

  esp_err_t result = esp_now_send(gameboardMAC, (uint8_t *)&data, sizeof(data));

  if (result == ESP_OK) {
    return true;
  } else {
    // Only print detailed error occasionally to avoid spam
    static unsigned long lastErrorPrint = 0;
    if (millis() - lastErrorPrint > 1000) {
      Serial.print("[ESP-NOW] Send error code: ");
      Serial.print(result);

      // Decode error
      switch (result) {
      case ESP_ERR_ESPNOW_NOT_INIT:
        Serial.println(" - ESP-NOW not initialized");
        break;
      case ESP_ERR_ESPNOW_ARG:
        Serial.println(" - Invalid argument");
        break;
      case ESP_ERR_ESPNOW_INTERNAL:
        Serial.println(" - Internal error");
        break;
      case ESP_ERR_ESPNOW_NO_MEM:
        Serial.println(" - Out of memory");
        break;
      case ESP_ERR_ESPNOW_NOT_FOUND:
        Serial.println(" - Peer not found");
        Serial.print("   Target MAC: ");
        printMAC(gameboardMAC);
        break;
      case ESP_ERR_ESPNOW_IF:
        Serial.println(" - WiFi interface mismatch");
        break;
      default:
        Serial.println(" - Unknown error");
        break;
      }
      lastErrorPrint = millis();
    }
    return false;
  }
}

// ========================================
// UTILITY
// ========================================

void PlantESPNow::printMAC(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5)
      Serial.print(":");
  }
  Serial.println();
}

void PlantESPNow::printStatus() {
  Serial.println("\n========================================");
  Serial.println("PLANT ESP-NOW STATUS");
  Serial.println("========================================");
  Serial.print("ESP-NOW Initialized: ");
  Serial.println(espnowInitialized ? "YES" : "NO");
  Serial.print("Peer Added: ");
  Serial.println(peerAdded ? "YES" : "NO");
  Serial.print("GameBoard MAC: ");
  printMAC(gameboardMAC);
  Serial.print("Ready to Send: ");
  Serial.println(isReady() ? "YES" : "NO");
  Serial.println("========================================\n");
}
