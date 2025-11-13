# Interface Board

Board kontrol interface untuk sistem escalator menggunakan ESP32/ESP8266 dengan LCD dan rotary encoder.

## Deskripsi

Interface board berfungsi sebagai control panel untuk mengatur dan memonitor sistem escalator. Menggunakan LCD ST7735 untuk display, rotary encoder untuk input, dan ESP-NOW untuk komunikasi wireless dengan Plant Board.

## Fitur

- **LCD Display**: ST7735 TFT Display untuk menampilkan menu dan status
- **Rotary Encoder**: Input kontrol dengan push button
- **ESP-NOW**: Komunikasi wireless dengan Plant Board
- **WiFi AP Mode**: Access point untuk koneksi jaringan
- **Menu System**: Sistem menu navigasi lengkap
- **Real-time Monitoring**: Monitor kecepatan dan status escalator

## Pin Configuration

### ESP32
| Komponen | Pin |
|----------|-----|
| TFT CS | GPIO 15 |
| TFT RST | GPIO 4 |
| TFT DC | GPIO 5 |
| Encoder CLK | GPIO 27 |
| Encoder DT | GPIO 26 |
| Encoder SW | GPIO 25 |
| LED Status | GPIO 13 |

### ESP8266
| Komponen | Pin |
|----------|-----|
| TFT CS | GPIO 15 |
| TFT RST | GPIO 4 |
| TFT DC | GPIO 5 |
| Encoder CLK | GPIO 12 |
| Encoder DT | GPIO 2 |
| Encoder SW | GPIO 16 |

## Struktur Menu

1. Status Monitor
2. Escalator Control
3. Motor AC Control
4. PID Settings
5. Network Status

## Build & Upload

```bash
# Build project
platformio run

# Upload ke ESP32/ESP8266
platformio run --target upload

# Monitor serial
platformio device monitor
```

## Konfigurasi

Edit `include/PinConfig.h` untuk:
- WiFi AP SSID & Password
- Plant Board MAC Address
- Pin configuration

## Testing

Tersedia test program encoder di `test/encoder_test.cpp` untuk debugging rotary encoder.

## Dependencies

- Adafruit GFX Library
- Adafruit ST7735 Library
- ESP-NOW (built-in)
