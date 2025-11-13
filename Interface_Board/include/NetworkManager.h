#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <Arduino.h>

// Platform-specific includes
#ifdef ESP32
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <espnow.h>
#endif

// ========================================
// DATA STRUCTURES untuk ESP-NOW
// ========================================

// Struktur data yang diterima dari Escalator
struct EscalatorDataPacket {
  float currentSpeed; // RPM atau m/s
  uint8_t direction;  // 0=forward, 1=reverse
  uint8_t mode;       // 0=manual, 1=auto
  float pidOutput;    // Output PID
  bool isRunning;
  unsigned long timestamp;
};

// Struktur data yang diterima dari Motor AC
struct MotorACDataPacket {
  float currentSpeed; // RPM
  uint8_t mode;       // 0=manual, 1=auto
  float pidOutput;
  bool isRunning;
  unsigned long timestamp;
};

// Struktur command untuk mengirim ke Escalator
struct EscalatorCommandPacket {
  uint8_t commandType; // 0=set_mode, 1=set_speed, 2=set_pid, 3=set_direction
  uint8_t mode;        // MODE_MANUAL or MODE_AUTO
  uint8_t direction;   // DIR_FORWARD or DIR_REVERSE
  float targetSpeed;
  float pidKp;
  float pidKi;
  float pidKd;
  float pidSetpoint;
  unsigned long timestamp;
};

// Struktur command untuk mengirim ke Motor AC
struct MotorACCommandPacket {
  uint8_t commandType; // 0=set_mode, 1=set_speed, 2=set_pid, 3=set_control_type
  uint8_t mode;
  uint8_t controlType; // DAC/PWM/Modbus
  uint8_t voltageType; // 5V/10V
  float targetSpeed;
  float pidKp;
  float pidKi;
  float pidKd;
  float pidSetpoint;
  unsigned long timestamp;
};

// ========================================
// NETWORK MANAGER CLASS
// ========================================

class NetworkManager {
private:
  // WiFi settings
  String apSSID;
  String apPassword;
  bool wifiInitialized;
  bool espnowInitialized;

  // ESP-NOW peer MAC addresses
  uint8_t escalatorMAC[6];
  uint8_t motorACMAC[6];
  bool escalatorPeerAdded;
  bool motorACPeerAdded;

  // Callbacks untuk data yang diterima
  void (*onEscalatorDataReceived)(EscalatorDataPacket data);
  void (*onMotorACDataReceived)(MotorACDataPacket data);
  void (*onSendSuccess)(const uint8_t *mac, bool success);

  // Static instance untuk callback ESP-NOW
  static NetworkManager *instance;

  // ESP-NOW callbacks (static) - Platform specific
#ifdef ESP32
  static void onDataRecv(const uint8_t *mac_addr, const uint8_t *data,
                         int data_len);
  static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
#elif defined(ESP8266)
  static void onDataRecv(uint8_t *mac_addr, uint8_t *data, uint8_t data_len);
  static void onDataSent(uint8_t *mac_addr, uint8_t status);
#endif

  // Helper functions
  bool addPeer(uint8_t *peerMAC);
  void printMAC(const uint8_t *mac);

public:
  NetworkManager();

  // Initialization
  bool beginWiFiAP(const char *ssid, const char *password);
  bool beginWiFiSTA();
  bool beginESPNow();

  // Set MAC addresses
  void setEscalatorMAC(uint8_t mac[6]);
  void setMotorACMAC(uint8_t mac[6]);

  // Add peers
  bool addEscalatorPeer();
  bool addMotorACPeer();

  // Set callbacks
  void onReceiveEscalatorData(void (*callback)(EscalatorDataPacket data));
  void onReceiveMotorACData(void (*callback)(MotorACDataPacket data));
  void onSendComplete(void (*callback)(const uint8_t *mac, bool success));

  // Send commands
  bool sendEscalatorCommand(EscalatorCommandPacket cmd);
  bool sendMotorACCommand(MotorACCommandPacket cmd);

  // Status getters
  bool isWiFiConnected() const { return wifiInitialized; }
  bool isESPNowReady() const { return espnowInitialized; }
  String getLocalIP() const;
  String getAPIP() const;
  int getConnectedClients() const;
  uint8_t *getLocalMAC();

  // Utility
  void printStatus();
};

#endif // NETWORK_MANAGER_H
