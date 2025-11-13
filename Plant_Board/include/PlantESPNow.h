#ifndef PLANT_ESPNOW_H
#define PLANT_ESPNOW_H

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// ========================================
// DATA STRUCTURES - Harus sama dengan GameBoard!
// ========================================

// Data yang dikirim ke GameBoard
struct EscalatorDataPacket {
  float currentSpeed; // RPM atau m/s
  uint8_t direction;  // 0=forward, 1=reverse
  uint8_t mode;       // 0=manual, 1=auto
  float pidOutput;    // Output PID
  bool isRunning;
  unsigned long timestamp;
};

// Command yang diterima dari GameBoard
struct EscalatorCommandPacket {
  uint8_t commandType; // 0=set_mode, 1=set_speed, 2=set_pid, 3=set_direction
  uint8_t mode;        // 0=manual, 1=auto
  uint8_t direction;   // 0=forward, 1=reverse
  float targetSpeed;
  float pidKp;
  float pidKi;
  float pidKd;
  float pidSetpoint;
  unsigned long timestamp;
};

// ========================================
// PLANT ESP-NOW MANAGER
// ========================================

class PlantESPNow {
private:
  uint8_t gameboardMAC[6];
  bool espnowInitialized;
  bool peerAdded;

  // Callbacks
  void (*onCommandReceived)(EscalatorCommandPacket cmd);
  void (*onSendComplete)(bool success);

  // Static instance untuk callback
  static PlantESPNow *instance;

  // ESP-NOW callbacks (static)
  static void onDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                         int data_len);
  static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

  void printMAC(const uint8_t *mac);

public:
  PlantESPNow();

  // Initialization
  bool begin();
  bool setGameboardMAC(uint8_t mac[6]);
  bool addGameboardPeer();

  // Set callbacks
  void onReceiveCommand(void (*callback)(EscalatorCommandPacket cmd));
  void onSendStatus(void (*callback)(bool success));

  // Send data
  bool sendData(EscalatorDataPacket data);

  // Status
  bool isReady() const { return espnowInitialized && peerAdded; }
  void printStatus();
};

#endif // PLANT_ESPNOW_H
