# Tes Interface ESP32

Project testing untuk Interface Board menggunakan ESP32 dengan fokus pada debugging dan validasi komponen.

## Deskripsi

Project ini berisi kode testing dan contoh implementasi untuk Interface Board. Berguna untuk debugging komponen seperti encoder, LCD, dan network sebelum implementasi final.

## Fitur Testing

- **Encoder Debug**: Logging detail untuk rotary encoder
- **LCD Test**: Test display ST7735
- **Network Test**: Test ESP-NOW dan WiFi AP
- **Menu System Demo**: Contoh implementasi menu system
- **Pin State Monitoring**: Real-time monitoring status pin

## File Penting

- `main.cpp`: Program encoder debug dan logging
- `main_example.cpp.txt`: Contoh implementasi lengkap menu system
- `network_example.cpp.txt`: Contoh implementasi network
- `ControlMenuSystem.cpp`: Implementasi sistem menu
- `EncoderControl.cpp`: Handler rotary encoder
- `NetworkManager.cpp`: Manager komunikasi network

## Program Encoder Debug

Program utama (`main.cpp`) untuk testing encoder dengan fitur:
- Interrupt-based encoder reading
- Real-time position tracking
- Trigger counting
- Debouncing
- Button reset

### Output Debug

```
ENCODER DEBUG & LOGGING TEST
========================================
CLK Pin: 27
DT Pin: 26
SW Pin: 25
========================================
encoder0Pin States: CLK=1, DT=1, SW=1
encoder1Pin States: CLK=0, DT=1, SW=1
```

## Pin Configuration

Sama dengan Interface Board (lihat `include/PinConfig.h`):

| Komponen | Pin ESP32 |
|----------|-----------|
| Encoder CLK | GPIO 27 |
| Encoder DT | GPIO 26 |
| Encoder SW | GPIO 25 |
| TFT CS | GPIO 15 |
| TFT RST | GPIO 4 |
| TFT DC | GPIO 5 |

## Cara Menggunakan

### 1. Test Encoder
```bash
# Upload program default (encoder debug)
platformio run --target upload
platformio device monitor
```

Putar encoder dan lihat output di serial monitor.

### 2. Test Full Menu System

Rename file untuk testing:
```bash
# Backup main.cpp
mv src/main.cpp src/main_debug.cpp

# Gunakan example
cp src/main_example.cpp.txt src/main.cpp

# Upload
platformio run --target upload
```

### 3. Test Network Only

```bash
cp src/network_example.cpp.txt src/main.cpp
platformio run --target upload
```

## Build & Upload

```bash
# Build
platformio run

# Upload
platformio run --target upload

# Monitor
platformio device monitor -b 115200
```

## Debugging Tips

1. **Encoder tidak terbaca**: Cek koneksi pullup dan debouncing
2. **LCD blank**: Cek pin RST dan power supply
3. **ESP-NOW gagal**: Cek MAC address di PinConfig.h
4. **Serial tidak muncul**: Pastikan baud rate 115200

## Dependencies

- Adafruit GFX Library
- Adafruit ST7735 Library
- WiFi (built-in)
- ESP-NOW (built-in)

## Notes

Project ini adalah versi development/testing. Untuk implementasi final, gunakan project `Interface_Board`.
