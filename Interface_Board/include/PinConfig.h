#ifndef PIN_CONFIG_H
#define PIN_CONFIG_H

// Ensure fixed-width integer types like uint8_t are available
#include <Arduino.h>

// ========================================
// KONFIGURASI PIN - ESP32 & ESP8266 Compatible
// File ini berisi deklarasi semua pin yang digunakan
// ========================================

#ifdef ESP32
// ========================================
// ESP32 PIN CONFIGURATION
// ========================================

// LCD ST7735 Pins (SPI)
#define TFT_CS 15 // Chip select
#define TFT_RST 4 // Reset
#define TFT_DC 5  // Data/Command
// SPI pins (hardware): MOSI=23, SCK=18

// Rotary Encoder Pins
#define ENCODER_CLK 27 // CLK_PIN / Channel A
#define ENCODER_DT 26  // DT_PIN / Channel B
#define ENCODER_SW 25  // SW_PIN / Switch button

// Motor Driver Pins
#define MOTOR_IN1 12 // Motor input 1
#define MOTOR_IN2 14 // Motor input 2

// LED Status Pin
#define LED_STATUS 13 // LED status

// Pin untuk Sensor (optional)
#define SENSOR_TEMP_PIN 35
#define SENSOR_LDR_PIN 36

// Pin untuk Motor (optional)
#define MOTOR1_PIN_A 12
#define MOTOR1_PIN_B 13
#define MOTOR2_PIN_A 14
#define MOTOR2_PIN_B 15

#elif defined(ESP8266)
// ========================================
// ESP8266 PIN CONFIGURATION
// ========================================

// LCD ST7735 Pins (SPI)
#define TFT_CS 15              // GPIO15 - Chip select
#define TFT_RST 4              // GPIO4 - Reset
#define TFT_DC 5               // GPIO5 - Data/Command
// SPI pins (hardware): MOSI=GPIO13, SCK=GPIO14

// Rotary Encoder Pins
#define ENCODER_CLK 12         // GPIO12 - CLK_PIN / Channel A
#define ENCODER_DT 2           // GPIO2 - DT_PIN / Channel B
#define ENCODER_SW 16          // GPIO16 - SW_PIN / Switch button

// Motor Driver Pins
#define MOTOR_IN1 12           // GPIO12 - Motor input 1
#define MOTOR_IN2 13           // GPIO13 - Motor input 2

// LED Status Pin
#define LED_STATUS LED_BUILTIN // Built-in LED (GPIO2)

// Pin untuk Sensor (optional)
#define SENSOR_TEMP_PIN A0     // Analog pin
#define SENSOR_LDR_PIN A0      // Analog pin (shared)

// Pin untuk Motor (optional)
#define MOTOR1_PIN_A 12        // GPIO12
#define MOTOR1_PIN_B 13        // GPIO13
#define MOTOR2_PIN_A 14        // GPIO14
#define MOTOR2_PIN_B 15        // GPIO15

#endif

// ========================================
// COMMON CONSTANTS
// ========================================

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
// SignBox/Plant MAC Address
static const uint8_t PLANT_MAC[6] = {0xD4, 0xD4, 0xDA, 0x5C, 0xFB, 0x88};

// Contoh MAC Address lainnya (uncomment jika diperlukan)

#endif // PIN_CONFIG_H
