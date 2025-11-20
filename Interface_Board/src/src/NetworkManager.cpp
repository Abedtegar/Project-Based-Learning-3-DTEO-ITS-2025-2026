#include "NetworkManager.h"

// Message ID definitions (tag values) – keep in sync with Plant side
#ifndef MSG_SPEED
#define MSG_SPEED 1
#endif
#ifndef MSG_PID_KP
#define MSG_PID_KP 11
#endif
#ifndef MSG_PID_KI
#define MSG_PID_KI 12
#endif
#ifndef MSG_PID_KD
#define MSG_PID_KD 13
#endif

// Simpan MAC target dalam array mutable
static uint8_t g_plantMAC[6] = {0};

void onDataSent(uint8_t *mac, uint8_t status) {
  Serial.print("Status kirim: ");
  Serial.println(status == 0 ? "OK" : "FAIL");
}

// Callback ESP-NOW (ESP8266 signature)
void receiveData(uint8_t *mac, uint8_t *data, uint8_t len) {
  // 8-byte tagged frame: int32 typeId + float value
  if (len == 8) {
    int32_t typeId;
    float value;
    memcpy(&typeId, &data[0], 4);
    memcpy(&value, &data[4], 4);
    switch (typeId) {
    case MSG_SPEED:
      Serial.print("RX SPEED: ");
      break;
    case MSG_PID_KP:
      Serial.print("RX PID.KP: ");
      break;
    case MSG_PID_KI:
      Serial.print("RX PID.KI: ");
      break;
    case MSG_PID_KD:
      Serial.print("RX PID.KD: ");
      break;
    default:
      Serial.print("RX TYPE ");
      Serial.print(typeId);
      Serial.print(": ");
      break;
    }
    Serial.println(value, 4);
    return;
  }
  if (len == 4) {
    float v;
    memcpy(&v, data, 4);
    Serial.print("RX FLOAT: ");
    Serial.println(v, 4);
    return;
  }
}

void espnow_init() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init gagal");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(receiveData);

  memcpy(g_plantMAC, PLANT_MAC, 6);

  if (!esp_now_is_peer_exist(g_plantMAC)) {
    if (esp_now_add_peer(g_plantMAC, ESP_NOW_ROLE_COMBO, 0, nullptr, 0) != 0) {
      Serial.println("Tambah peer gagal");
    }
  }
  Serial.println("ESP-NOW siap (ESP8266 mode, 4/8 byte parsing).");
}

void sendData(float data) { esp_now_send(g_plantMAC, (uint8_t *)&data, 4); }


void sendTaggedFloat(int32_t typeId, float value) {
  uint8_t buf[8];
  memcpy(&buf[0], &typeId, 4);
  memcpy(&buf[4], &value, 4);
  esp_now_send(g_plantMAC, buf, sizeof(buf));
}