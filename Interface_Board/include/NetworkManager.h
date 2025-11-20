#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include "PinConfig.h" // pastikan PLANT_MAC tersedia
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#define MSG_SPEED 1
#define MSG_PID_KP 11
#define MSG_PID_KI 12
#define MSG_PID_KD 13
#define ID_SPEED 200

void espnow_init();          // inisialisasi ESP-NOW
void sendSpeed(float speed); // kirim 4 byte float
void receiveData(uint8_t *mac, uint8_t *data, uint8_t len); // callback RX
void onDataSent(uint8_t *mac, uint8_t status);              // callback TX
void sendTaggedFloat(int32_t typeId, float value);

#endif