#include "NetworkManager.h"

// Initialize static instance
NetworkManager *NetworkManager::instance = nullptr;

// ========================================
// CONSTRUCTOR
// ========================================

NetworkManager::NetworkManager() {
  wifiInitialized = false;
  espnowInitialized = false;
  escalatorPeerAdded = false;
  motorACPeerAdded = false;

  // Clear MAC addresses
  memset(escalatorMAC, 0, 6);
  memset(motorACMAC, 0, 6);

  // Clear callbacks
  onEscalatorDataReceived = nullptr;
  onMotorACDataReceived = nullptr;
  onSendSuccess = nullptr;

  // Set static instance
  instance = this;
}

// ========================================
// WIFI INITIALIZATION
// ========================================

bool NetworkManager::beginWiFiAP(const char *ssid, const char *password) {
  Serial.println("[WiFi] Initializing Access Point...");

  // Use AP+STA mode for better ESP-NOW compatibility with ESP32
  WiFi.mode(WIFI_AP_STA);

  // Set channel to 1 for ESP-NOW compatibility
  bool success = WiFi.softAP(ssid, password, 1, 0,
                             4); // channel=1, hidden=0, max_connection=4

  if (success) {
    wifiInitialized = true;
    apSSID = String(ssid);
    apPassword = String(password);

    Serial.println("[WiFi] AP Started Successfully");
    Serial.println("[WiFi] Mode: AP+STA (ESP-NOW compatible)");
    Serial.print("[WiFi] SSID: ");
    Serial.println(ssid);
    Serial.print("[WiFi] Channel: 1");
    Serial.println();
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.softAPIP());
    Serial.print("[WiFi] MAC Address: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println("[WiFi] AP Failed to start!");
  }

  return success;
}

bool NetworkManager::beginWiFiSTA() {
  Serial.println("[WiFi] Initializing Station Mode...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  wifiInitialized = true;

  Serial.println("[WiFi] STA Mode Ready");
  Serial.print("[WiFi] MAC Address: ");
  Serial.println(WiFi.macAddress());

  return true;
}

// ========================================
// ESP-NOW INITIALIZATION
// ========================================

bool NetworkManager::beginESPNow() {
  Serial.println("[ESP-NOW] Initializing...");

#ifdef ESP32
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] Init Failed!");
    espnowInitialized = false;
    return false;
  }

  espnowInitialized = true;
  Serial.println("[ESP-NOW] Initialized Successfully");

  // Register callbacks
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

#elif defined(ESP8266)
  if (esp_now_init() != 0) {
    Serial.println("[ESP-NOW] Init Failed!");
    espnowInitialized = false;
    return false;
  }

  espnowInitialized = true;
  Serial.println("[ESP-NOW] Initialized Successfully");

  // Register callbacks for ESP8266
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);
#endif

  Serial.println("[ESP-NOW] Callbacks Registered");

  return true;
}

// ========================================
// SET MAC ADDRESSES
// ========================================

void NetworkManager::setEscalatorMAC(uint8_t mac[6]) {
  memcpy(escalatorMAC, mac, 6);
  Serial.print("[ESP-NOW] Escalator MAC set to: ");
  printMAC(escalatorMAC);
}

void NetworkManager::setMotorACMAC(uint8_t mac[6]) {
  memcpy(motorACMAC, mac, 6);
  Serial.print("[ESP-NOW] Motor AC MAC set to: ");
  printMAC(motorACMAC);
}

// ========================================
// ADD PEERS
// ========================================

bool NetworkManager::addPeer(uint8_t *peerMAC) {
  // Check if MAC is not empty
  bool isEmpty = true;
  for (int i = 0; i < 6; i++) {
    if (peerMAC[i] != 0) {
      isEmpty = false;
      break;
    }
  }

  if (isEmpty) {
    Serial.println("[ESP-NOW] Cannot add peer: MAC address is empty!");
    return false;
  }

#ifdef ESP32
  // Check if peer already exists
  if (esp_now_is_peer_exist(peerMAC)) {
    Serial.println("[ESP-NOW] Peer already exists");
    return true;
  }

  // Create peer info
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, peerMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.print("[ESP-NOW] Failed to add peer: ");
    printMAC(peerMAC);
    return false;
  }

#elif defined(ESP8266)
  // Check if peer already exists
  if (esp_now_is_peer_exist(peerMAC)) {
    Serial.println("[ESP-NOW] Peer already exists");
    return true;
  }

  // Add peer for ESP8266
  int result = esp_now_add_peer(peerMAC, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
  if (result != 0) {
    Serial.print("[ESP-NOW] Failed to add peer: ");
    printMAC(peerMAC);
    return false;
  }
#endif

  Serial.print("[ESP-NOW] Peer added successfully: ");
  printMAC(peerMAC);
  return true;
}

bool NetworkManager::addEscalatorPeer() {
  escalatorPeerAdded = addPeer(escalatorMAC);
  return escalatorPeerAdded;
}

bool NetworkManager::addMotorACPeer() {
  motorACPeerAdded = addPeer(motorACMAC);
  return motorACPeerAdded;
}

// ========================================
// CALLBACKS SETUP
// ========================================

void NetworkManager::onReceiveEscalatorData(
    void (*callback)(EscalatorDataPacket data)) {
  onEscalatorDataReceived = callback;
}

void NetworkManager::onReceiveMotorACData(
    void (*callback)(MotorACDataPacket data)) {
  onMotorACDataReceived = callback;
}

void NetworkManager::onSendComplete(void (*callback)(const uint8_t *mac,
                                                     bool success)) {
  onSendSuccess = callback;
}

// ========================================
// ESP-NOW STATIC CALLBACKS
// ========================================

#ifdef ESP32
void NetworkManager::onDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                                int data_len) {
#elif defined(ESP8266)
void NetworkManager::onDataRecv(uint8_t *mac_addr, uint8_t *data,
                                uint8_t data_len) {
#endif
  if (!instance)
    return;

  Serial.print("[ESP-NOW] Data received from: ");
  instance->printMAC(mac_addr);
  Serial.print(" | Size: ");
  Serial.println(data_len);

  // Check data type by size
  if (data_len == sizeof(EscalatorDataPacket)) {
    EscalatorDataPacket packet;
    memcpy(&packet, data, sizeof(packet));

    Serial.println("[ESP-NOW] Escalator data packet received");

    if (instance->onEscalatorDataReceived) {
      instance->onEscalatorDataReceived(packet);
    }
  } else if (data_len == sizeof(MotorACDataPacket)) {
    MotorACDataPacket packet;
    memcpy(&packet, data, sizeof(packet));

    Serial.println("[ESP-NOW] Motor AC data packet received");

    if (instance->onMotorACDataReceived) {
      instance->onMotorACDataReceived(packet);
    }
  } else {
    Serial.println("[ESP-NOW] Unknown packet type");
  }
}

#ifdef ESP32
void NetworkManager::onDataSent(const uint8_t *mac_addr,
                                esp_now_send_status_t status) {
#elif defined(ESP8266)
void NetworkManager::onDataSent(uint8_t *mac_addr, uint8_t status) {
#endif
  if (!instance)
    return;

  Serial.print("[ESP-NOW] Send Status to ");
  instance->printMAC(mac_addr);
  Serial.print(": ");
#ifdef ESP32
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAILED");
  if (instance->onSendSuccess) {
    instance->onSendSuccess(mac_addr, status == ESP_NOW_SEND_SUCCESS);
  }
#elif defined(ESP8266)
  Serial.println(status == 0 ? "SUCCESS" : "FAILED");
  if (instance->onSendSuccess) {
    instance->onSendSuccess(mac_addr, status == 0);
  }
#endif
}

// ========================================
// SEND COMMANDS
// ========================================

bool NetworkManager::sendEscalatorCommand(EscalatorCommandPacket cmd) {
  if (!espnowInitialized) {
    Serial.println("[ESP-NOW] Cannot send: ESP-NOW not initialized");
    return false;
  }

  if (!escalatorPeerAdded) {
    Serial.println("[ESP-NOW] Cannot send: Escalator peer not added");
    Serial.println("[ESP-NOW] Make sure to call addEscalatorPeer() first!");
    return false;
  }

#ifdef ESP32
  // Verify peer exists
  if (!esp_now_is_peer_exist(escalatorMAC)) {
    Serial.println("[ESP-NOW] ERROR: Peer does not exist!");
    Serial.print("[ESP-NOW] Target MAC: ");
    printMAC(escalatorMAC);
    Serial.println("[ESP-NOW] Try adding peer again...");
    return false;
  }
#elif defined(ESP8266)
  // Verify peer exists
  if (!esp_now_is_peer_exist(escalatorMAC)) {
    Serial.println("[ESP-NOW] ERROR: Peer does not exist!");
    Serial.print("[ESP-NOW] Target MAC: ");
    printMAC(escalatorMAC);
    Serial.println("[ESP-NOW] Try adding peer again...");
    return false;
  }
#endif

  cmd.timestamp = millis();

  Serial.print("[ESP-NOW] Sending to MAC: ");
  printMAC(escalatorMAC);
  Serial.printf("[ESP-NOW] Packet size: %d bytes\n", sizeof(cmd));

#ifdef ESP32
  esp_err_t result = esp_now_send(escalatorMAC, (uint8_t *)&cmd, sizeof(cmd));

  if (result == ESP_OK) {
    Serial.println("[ESP-NOW] Command queued successfully");
    return true;
  } else {
    Serial.print("[ESP-NOW] Send failed, error code: ");
    Serial.println(result);

    // Decode error
    switch (result) {
    case ESP_ERR_ESPNOW_NOT_INIT:
      Serial.println("  -> ESP-NOW not initialized");
      break;
    case ESP_ERR_ESPNOW_ARG:
      Serial.println("  -> Invalid argument");
      break;
    case ESP_ERR_ESPNOW_INTERNAL:
      Serial.println("  -> Internal error");
      break;
    case ESP_ERR_ESPNOW_NO_MEM:
      Serial.println("  -> Out of memory");
      break;
    case ESP_ERR_ESPNOW_NOT_FOUND:
      Serial.println("  -> Peer not found");
      break;
    case ESP_ERR_ESPNOW_IF:
      Serial.println("  -> Current WiFi interface doesn't match");
      break;
    default:
      Serial.println("  -> Unknown error");
      break;
    }
    return false;
  }
#elif defined(ESP8266)
  int result = esp_now_send(escalatorMAC, (uint8_t *)&cmd, sizeof(cmd));

  if (result == 0) {
    Serial.println("[ESP-NOW] Command queued successfully");
    return true;
  } else {
    Serial.print("[ESP-NOW] Send failed, error code: ");
    Serial.println(result);
    return false;
  }
#endif
}

bool NetworkManager::sendMotorACCommand(MotorACCommandPacket cmd) {
  if (!espnowInitialized) {
    Serial.println("[ESP-NOW] Cannot send: ESP-NOW not initialized");
    return false;
  }

  if (!motorACPeerAdded) {
    Serial.println("[ESP-NOW] Cannot send: Motor AC peer not added");
    return false;
  }

  cmd.timestamp = millis();

#ifdef ESP32
  esp_err_t result = esp_now_send(motorACMAC, (uint8_t *)&cmd, sizeof(cmd));

  if (result == ESP_OK) {
    Serial.println("[ESP-NOW] Motor AC command sent");
    return true;
  } else {
    Serial.print("[ESP-NOW] Send failed, error code: ");
    Serial.println(result);
    return false;
  }
#elif defined(ESP8266)
  int result = esp_now_send(motorACMAC, (uint8_t *)&cmd, sizeof(cmd));

  if (result == 0) {
    Serial.println("[ESP-NOW] Motor AC command sent");
    return true;
  } else {
    Serial.print("[ESP-NOW] Send failed, error code: ");
    Serial.println(result);
    return false;
  }
#endif
}

// ========================================
// STATUS GETTERS
// ========================================

String NetworkManager::getLocalIP() const {
  if (WiFi.getMode() == WIFI_STA) {
    return WiFi.localIP().toString();
  }
  return "N/A";
}

String NetworkManager::getAPIP() const {
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    return WiFi.softAPIP().toString();
  }
  return "N/A";
}

int NetworkManager::getConnectedClients() const {
  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    return WiFi.softAPgetStationNum();
  }
  return 0;
}

uint8_t *NetworkManager::getLocalMAC() {
  static uint8_t mac[6];
#ifdef ESP32
  esp_wifi_get_mac(WIFI_IF_STA, mac);
#elif defined(ESP8266)
  WiFi.macAddress(mac);
#endif
  return mac;
}

// ========================================
// UTILITY FUNCTIONS
// ========================================

void NetworkManager::printMAC(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    Serial.printf("%02X", mac[i]);
    if (i < 5)
      Serial.print(":");
  }
  Serial.println();
}

void NetworkManager::printStatus() {
  Serial.println("\n========================================");
  Serial.println("NETWORK STATUS");
  Serial.println("========================================");

  // WiFi Status
  Serial.println("\n[WiFi]");
  Serial.print("  Mode: ");
  switch (WiFi.getMode()) {
  case WIFI_OFF:
    Serial.println("OFF");
    break;
  case WIFI_STA:
    Serial.println("STATION");
    break;
  case WIFI_AP:
    Serial.println("ACCESS POINT");
    break;
  case WIFI_AP_STA:
    Serial.println("AP + STATION");
    break;
  }

  if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
    Serial.print("  AP SSID: ");
    Serial.println(apSSID);
    Serial.print("  AP IP: ");
    Serial.println(getAPIP());
    Serial.print("  Connected Clients: ");
    Serial.println(getConnectedClients());
  }

  Serial.print("  MAC Address: ");
  Serial.println(WiFi.macAddress());

  // ESP-NOW Status
  Serial.println("\n[ESP-NOW]");
  Serial.print("  Initialized: ");
  Serial.println(espnowInitialized ? "YES" : "NO");

  if (espnowInitialized) {
    Serial.print("  Escalator Peer: ");
    Serial.println(escalatorPeerAdded ? "ADDED" : "NOT ADDED");
    if (escalatorPeerAdded) {
      Serial.print("    MAC: ");
      printMAC(escalatorMAC);
    }

    Serial.print("  Motor AC Peer: ");
    Serial.println(motorACPeerAdded ? "ADDED" : "NOT ADDED");
    if (motorACPeerAdded) {
      Serial.print("    MAC: ");
      printMAC(motorACMAC);
    }
  }

  Serial.println("========================================\n");
}
