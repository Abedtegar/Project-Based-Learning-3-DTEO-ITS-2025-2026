#include "NetworkManager.h"
#include <Arduino.h>
#include <ControlMenuSystem.h>

float DCspeed = 0;

float ACspeed = 0;
float plantDCKP = 0;
float plantDCKI = 0;
float plantDCKD = 0;
float plantACKP = 0;
float plantACKI = 0;
float plantACKD = 0;
float plantACSetpoint = 0;
float plantDCSetpoint = 0;
float DCError = 0;
float ACError = 0;
long timestamp = 0;
// Monitoring buffers for PID response analysis
const int MON_BUF = 500;
// DC monitoring
long dc_ts[MON_BUF];
float dc_vals[MON_BUF];
int dc_idx = 0;
bool dc_monitoring = false;
float dc_target = 0.0f;
long dc_start_ts = 0;
// AC monitoring
long ac_ts[MON_BUF];
float ac_vals[MON_BUF];
int ac_idx = 0;
bool ac_monitoring = false;
float ac_target = 0.0f;
long ac_start_ts = 0;
// Tambahkan buffer untuk menyimpan timestamp
const int TIMESTAMP_BUF = 100; // Buffer untuk 100 timestamp
long timestamp_buffer[TIMESTAMP_BUF];
int timestamp_idx = 0;
// Simpan MAC target dalam array mutable
static uint8_t g_plantMAC[6] = {0};

void onDataSent(uint8_t *mac, uint8_t status) {
  Serial.print("Status kirim: ");
  Serial.println(status == 0 ? "OK" : "FAIL");
  if (status == 0) {
    espconnected = true;
  } else {
    espconnected = false;
  }
}

void receiveData(uint8_t *mac, uint8_t *data, uint8_t len) {
  // 8-byte tagged frame: int32 typeId + float value
  if (len == 8) {
    int32_t typeId;
    float value;
    memcpy(&typeId, &data[0], 4);
    memcpy(&value, &data[4], 4);
    switch (typeId) {
    case MSG_DC_SPEED:
      updateEscalatorSpeed(value);
      Serial.print("Time: ,");
      Serial.print(timestamp);
      Serial.print(", DC SPEED: , ");
      Serial.println(value, 4);


      break;
    case MSG_AC_SPEED:
      updateMotorSpeed(value);
      Serial.print("Time: ,");
      Serial.print(timestamp);
      Serial.print(", AC SPEED: ,");
      Serial.println(value, 4);
      // record sample if monitoring active
      break;
    case MSG_DC_KP:
      g_dcKp = value;
      Serial.print("RX PID.KP: ");
      break;
    case MSG_DC_KI:
      g_dcKi = value;
      Serial.print("RX PID.KI: ");
      break;
    case MSG_DC_KD:
      g_dcKd = value;
      Serial.print("RX PID.KD: ");
      break;
    case MSG_AC_KP:
      g_acKp = value;
      Serial.print("RX PID.KP: ");
      break;
    case MSG_AC_KI:
      g_acKi = value;
      Serial.print("RX PID.KI: ");
      break;
    case MSG_AC_KD:
      g_acKd = value;
      Serial.print("RX PID.KD: ");
      break;
    case MSG_AC_Setpoint:
      g_acSetpoint = value;
      Serial.print("RX AC SETPOINT: ");
      ac_target = value;
      break;
    case MSG_DC_Setpoint:
      g_dcSetpoint = value;
      Serial.print("RX DC SETPOINT: ");
      // hanya update target, jangan reset monitoring
      dc_target = value;
      break;
    case MSG_DC_Control:
      Serial.print("RX DC_Control: ");
      if (value > 0.5f) {
        // Motor ON - start fresh monitoring dengan setpoint sekarang
        dc_monitoring = true;
        dc_idx = 0;
        dc_target = g_dcSetpoint;
        dc_start_ts = timestamp ? timestamp : millis();
        Serial.println("DC Motor ON - monitoring dimulai");
      } else {
        // Motor OFF - stop monitoring dan reset metrics
        dc_monitoring = false;
        dc_idx = 0;
        Serial.println("DC Motor OFF - metrics di-reset");
      }
      break;
    case MSG_AC_Control:
      Serial.print("RX AC_Control: ");
      if (value > 0.5f) {
        // Motor ON - start fresh monitoring dengan setpoint sekarang
        ac_monitoring = true;
        ac_idx = 0;
        ac_target = g_acSetpoint;
        ac_start_ts = timestamp ? timestamp : millis();
        Serial.println("AC Motor ON - monitoring dimulai");
      } else {
        // Motor OFF - stop monitoring dan reset metrics
        ac_monitoring = false;
        ac_idx = 0;
        Serial.println("AC Motor OFF - metrics di-reset");
      }
      break;
    case MSG_TIMESTAMP:
      // Serial.print("RX TIMESTAMP: ");
      timestamp = (long)value;
      addTimestamp(timestamp); // Simpan timestamp ke buffer
      break;
    default:
      Serial.print("RX TYPE ");
      Serial.print(typeId);
      Serial.print(": ");
      break;
    }
    // Serial.println(value, 4);
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

// Fungsi untuk menambahkan timestamp ke buffer
void addTimestamp(long ts) {
  timestamp_buffer[timestamp_idx] = ts;
  timestamp_idx = (timestamp_idx + 1) % TIMESTAMP_BUF;
}

// Fungsi untuk mendapatkan timestamp dari buffer
long getTimestamp(int index) {
  if (index < 0 || index >= TIMESTAMP_BUF) {
    return -1; // Nilai invalid jika index di luar rentang
  }
  return timestamp_buffer[(timestamp_idx + index) % TIMESTAMP_BUF];
}

void espnow_init() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  Serial.print("ESP-NOW MAC Address: ");
  Serial.println(WiFi.macAddress());

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