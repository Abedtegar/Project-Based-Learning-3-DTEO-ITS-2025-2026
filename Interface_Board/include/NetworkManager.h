#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "PinConfig.h" // pastikan PLANT_MAC tersedia
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

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

#define ESP_RESTART 100

extern bool espconnected;
extern float DCspeed;
extern float ACspeed;
void espnow_init();          // inisialisasi ESP-NOW
void sendSpeed(float speed); // kirim 4 byte float
void receiveData(uint8_t *mac, uint8_t *data, uint8_t len); // callback RX
void onDataSent(uint8_t *mac, uint8_t status);              // callback TX
void sendTaggedFloat(int32_t typeId, float value);

#endif