#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// ========================================
// KONFIGURASI PIN ESP32
// File ini berisi deklarasi semua pin yang digunakan
// ========================================

// LCD ST7735 Pins (SPI)
#define TFT_CS 15 // Chip select
#define TFT_RST 4 // Reset
#define TFT_DC 2  // Data/Command
// SPI pins (hardware): MOSI=23, SCK=18

// Rotary Encoder Pins
#define ENCODER_CLK 27 // CLK_PIN / Channel A
#define ENCODER_DT 26  // DT_PIN / Channel B
#define ENCODER_SW 25  // SW_PIN / Switch button

// Motor Driver Pins
#define MOTOR_IN1 12 // Motor input 1
#define MOTOR_IN2 14 // Motor input 2

// Pin untuk LED Status (JANGAN gunakan pin 2, bentrok dengan TFT_DC!)
// #define LED_STATUS 2 // BENTROK dengan TFT_DC!
#define LED_STATUS 13 // Gunakan pin lain untuk LED status

// Pin untuk Sensor (optional - untuk project sebelumnya)
#define SENSOR_TEMP_PIN 35
#define SENSOR_LDR_PIN 36

// Pin untuk Motor (optional - untuk project sebelumnya)
#define MOTOR1_PIN_A 12
#define MOTOR1_PIN_B 13
#define MOTOR2_PIN_A 14
#define MOTOR2_PIN_B 15

// Konstanta lainnya
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8
#define SERIAL_BAUD 115200

// ========================================
// NETWORK CONFIGURATION
// ========================================

// WiFi AP Configuration
#define WIFI_AP_SSID "GameBoard_AP"
#define WIFI_AP_PASSWORD "12345678"

// ESP-NOW Configuration
// MAC Address untuk komunikasi (sesuaikan dengan device kalian)
// SignBox MAC Address
#define PLANT_MAC {0x20, 0xE7, 0xC8, 0x59, 0x6D, 0x00}

// Contoh MAC Address lainnya (uncomment jika diperlukan)
// #define ESCALATOR_MAC {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x01}
// #define MOTOR_AC_MAC {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0x02}

#endif // PIN_CONFIG_H
