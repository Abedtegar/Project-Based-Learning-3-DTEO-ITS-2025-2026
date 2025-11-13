#ifndef PLANT_CONFIG_H
#define PLANT_CONFIG_H

// ========================================
// PIN CONFIGURATION PLANT BOARD
// ========================================

// DC Motor Control Pins
#define MOTOR_PWM_PIN 13
#define MOTOR_DIR_PIN 22 // changed
// PWM
#define PWM_CHANNEL 0
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// DC Motor Encoder Pins
#define ENCODER_A_PIN 34
#define ENCODER_B_PIN 35
#define PPR 14
#define DCREAD_INTERVAL 5 // ms
// AC Motor Encoder Pins
#define AC_ENCODER_PIN 32
#define ACREAD_INTERVAL 5 // ms

// AC Motor Control Pins
#define AC_DAC1_PIN 25
#define AC_DAC2_PIN 26
#define AC_DAC_SOURCE_PIN 27
#define AC_DAC_VOLTAGE_SELECT_PIN 12 // changed

// LED Indicators
#define LED_WIFI 21
#define LED_PID 19
#define LED_DC_CONTROL 18
#define LED_AC_CONTROL 5

// PWM Settings
#define LEDC_CHANNEL 0  // Channel PWM (0–15)
#define LEDC_TIMER 0    // Timer (0–3)
#define LEDC_FREQ 10000 // Frekuensi PWM (Hz)
#define LEDC_RES 12     // Resolusi PWM (bit) → 12 bit = 0–4095

// Serial
#define SERIAL_BAUD 115200
#define TX_ENABLE_PIN 4
//
#define PID_INTERVAL 5 // ms
// ========================================
// NETWORK CONFIGURATION
// ========================================

// Interface MAC Address (sesuaikan dengan MAC Interface Anda)
// Cek di Serial Monitor Interface saat startup
#define INTERFACE_MAC                                                          \
  {0x50, 0x02, 0x91, 0x78, 0x72, 0xA7} // MAC dari Interface_Board ESP8266

// Data sending interval
#define DATA_SEND_INTERVAL 1000 // Send data setiap 10ms

#endif // PLANT_CONFIG_H
