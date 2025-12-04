#ifndef PLANT_ESPNOW_H
#define PLANT_ESPNOW_H

#include <Arduino.h>
#include <PlantConfig.h>
#include <WiFi.h>
#include <esp_now.h>

#define MSG_DC_SPEED 10
#define MSG_DC_KP 11
#define MSG_DC_KI 12
#define MSG_DC_KD 13
#define MSG_DC_Setpoint 14
#define MSG_DC_Control 15
#define MSG_DC_Direction 16

#define MSG_AC_SPEED 20
#define MSG_AC_KP 21
#define MSG_AC_KI 22
#define MSG_AC_KD 23
#define MSG_AC_Setpoint 24
#define MSG_AC_Control 25
#define MSG_AC_Voltage 26
#define MSG_AC_Direction 27

#define MSG_PID_MODE 30
#define MSG_SPD_REQUEST 31
#define MSG_TIMESTAMP 32
#define ESP_RESTART 100

extern bool espnowReady;
extern bool dcspeedRequest;
extern bool acspeedRequest;
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
void espnow_init();
void sendData(float data);
void receiveData(const uint8_t *mac_addr, const uint8_t *data, int len);
void sendTaggedFloat(int32_t typeId, float value);

#endif