#include "PlantESPNow.h"
#include "DataStorage.h"
#include "PID.h"
#include "Plant.h"
#include "PlantConfig.h"

bool DCMode = true;
bool ACMode = false;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("Status Pengiriman: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Sukses" : "Gagal");
}

void espnow_init() {
  // Set WiFi mode sebelum inisialisasi ESP-NOW
  WiFi.mode(WIFI_STA);
  delay(100);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register callbacks
  esp_now_register_recv_cb(receiveData);
  esp_now_register_send_cb(OnDataSent);

  // Tambahkan peer ESP8266 (Interface Board) untuk balasan
  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, INTERFACE_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer (INTERFACE)");
    return;
  }

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
      } else if (value == 0) {
        DCMode = false;
        ACMode = false;
      }
      break;
    case MSG_AC_Control:
      if (value == 1) {
        ACMode = true;
        DCMode = false;
      } else if (value == 0) {
        ACMode = false;
        DCMode = false;
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

    default:
      Serial.print("RX TYPE ");
      Serial.print(typeId);
      Serial.print(": ");
      break;
    }
    Serial.print("ESPNOW RX VALUE: ");
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
  uint8_t buf[8];
  memcpy(&buf[0], &typeId, 4);
  memcpy(&buf[4], &value, 4);
  esp_now_send(INTERFACE_MAC, buf, sizeof(buf));
}