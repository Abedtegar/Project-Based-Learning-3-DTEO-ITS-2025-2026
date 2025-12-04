#include "PlantESPNow.h"
#include "DataStorage.h"
#include "PID.h"
#include "Plant.h"
#include "PlantConfig.h"
#include <LedControl.h>
bool DCMode = true;
bool ACMode = false;
bool PIDMODE = false;
bool espnowReady = false;
bool speedRequest = false;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.print("Status Pengiriman: ");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sukses" : "Gagal");
  if (status == ESP_NOW_SEND_SUCCESS) {
    setLedState(LED_WIFI, true);
  } else {
    setLedState(LED_WIFI, false);
  }
}

void espnow_init() {
  // Set WiFi mode sebelum inisialisasi ESP-NOW
  Serial.println("espnow: set wifi mode to STA");
  WiFi.mode(WIFI_STA);
  delay(100);

  Serial.println("espnow: calling esp_now_init");
  esp_err_t rc = esp_now_init();
  if (rc != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.println("espnow: esp_now_init ok");

  Serial.print("ESP-NOW MAC Address: ");
  Serial.println(WiFi.macAddress());

  // Register callbacks
  esp_now_register_recv_cb(receiveData);
  esp_now_register_send_cb(OnDataSent);

  // Tambahkan peer ESP8266 (Interface Board) untuk balasan
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, INTERFACE_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_err_t peerRes = esp_now_add_peer(&peerInfo);
  if (peerRes != ESP_OK) {

    Serial.println("Failed to add peer (INTERFACE)");
    return;
  }
  Serial.println("espnow: peer added");

  espnowReady = true;
  Serial.println("ESP-NOW siap RX/TX (Plant Board).");
}

void sendData(float data) {
  esp_now_send(INTERFACE_MAC, (uint8_t *)&data, sizeof(data)); // 4 byte
}

void receiveData(const uint8_t *mac, const uint8_t *data, int len) {
  if (len == 8) {
    int32_t typeId;
    float value;
    memcpy(&typeId, &data[0], 4);
    memcpy(&value, &data[4], 4);

    switch (typeId) {

    case MSG_DC_KP:
      StoreData("pid_dc", "kP", value);
      break;
    case MSG_DC_KI:
      StoreData("pid_dc", "kI", value);
      break;
    case MSG_DC_KD:
      StoreData("pid_dc", "kD", value);
      break;
    case MSG_DC_Setpoint:
      StoreData("pid_dc", "setpoint", value);
      break;
    case MSG_AC_KP:
      StoreData("pid_ac", "kP", value);
      break;
    case MSG_AC_KI:
      StoreData("pid_ac", "kI", value);
      break;
    case MSG_AC_KD:
      StoreData("pid_ac", "kD", value);
      break;
    case MSG_AC_Setpoint:
      StoreData("pid_ac", "setpoint", value);
      break;
    case MSG_DC_Control:
      if (value == 1 && !DCMode) {
        DCintegralSum = 0;
        DCpreviousError = 0;
      }
      if (value == 1) {
        DCMode = true;
        ACMode = false;
        setLedState(LED_DC_CONTROL, true);
        DClastEncoder = DCencoder;
        waktu_awal_motor = millis();
        sendTaggedFloat(MSG_DC_Control, 1);
      } else if (value == 0) {
        DCMode = false;
        ACMode = false;
        setLedState(LED_DC_CONTROL, false);
        sendTaggedFloat(MSG_DC_Control, 0);
      }
      break;
    case MSG_AC_Control:
      if (value == 1) {
        ACMode = true;
        DCMode = false;
        setLedState(LED_AC_CONTROL, true);
        waktu_awal_motor = millis();
        sendTaggedFloat(MSG_AC_Control, 1);
      } else if (value == 0) {
        ACMode = false;
        DCMode = false;
        setLedState(LED_AC_CONTROL, false);
        sendTaggedFloat(MSG_AC_Control, 0);
      }
      break;
    case MSG_AC_Voltage:
      StoreData("pid_ac", "acvoltage", value);
      break;
    case MSG_DC_Direction:
      if (value == 1) {
        StoreData("pid_dc", "dcdirection", 1);
      } else if (value == 0) {
        StoreData("pid_dc", "dcdirection", 0);
      }
      break;

    case MSG_PID_MODE:
      UpdatePIDParam();
      if (value == 1) {
        PIDMODE = true;
        setLedState(LED_PID, true);
      } else if (value == 0) {
        PIDMODE = false;
        setLedState(LED_PID, false);
      }
      break;
      case ESP_RESTART:
      if (value == 1)
      ESP.restart();
      break;
      default:
      Serial.print("RX TYPE ");
      Serial.print(typeId);
      Serial.print(": ");
      break;
    case MSG_SPD_REQUEST:
      if (value == 1) {
        dcspeedRequest = true;
        acspeedRequest = false;
      } else if(value == 2) {
        acspeedRequest = true;
        dcspeedRequest = false;
      }
      else {
        dcspeedRequest = false;
        acspeedRequest = false;
      }
      break;
    }
    Serial.print("type id: ");
    Serial.print(typeId);
    Serial.print(" || ESPNOW RX VALUE: ");
    Serial.println(value, 4);
    return;
  }

  if (len == 4) {
    float v;
    memcpy(&v, data, 4);
    Serial.print("RX FLOAT (fallback): ");
    Serial.println(v, 4);
    return;
  }

  Serial.print("Panjang data tidak sesuai: ");
  Serial.println(len);
}

void sendTaggedFloat(int32_t typeId, float value) {
  if (!espnowReady)
    return;
  uint8_t buf[8];
  memcpy(&buf[0], &typeId, 4);
  memcpy(&buf[4], &value, 4);
  esp_now_send(INTERFACE_MAC, buf, sizeof(buf));
}