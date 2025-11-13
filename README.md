# Program Sistem Kontrol Escalator

Repository ini berisi seluruh program untuk sistem kontrol escalator berbasis ESP32/ESP8266 yang dikembangkan untuk proyek CBL Semester 3.

## Struktur Project

```
Program/
├── ambil_data_encoder/     # Program pembacaan encoder motor DC
├── Interface_Board/        # Control panel dengan LCD dan encoder
├── Plant_Board/           # Controller utama motor dan sensor
└── Tes_Interface_esp32/   # Testing dan debugging interface
```

## Deskripsi Project

### 1. ambil_data_encoder
Program standalone untuk testing dan kalibrasi encoder motor DC. Membaca pulsa encoder dan menghitung RPM secara real-time.

**Platform**: ESP32

### 2. Interface_Board
Board interface untuk kontrol dan monitoring sistem. Dilengkapi LCD TFT, rotary encoder, dan komunikasi ESP-NOW.

**Platform**: ESP32 / ESP8266

**Fitur**:
- Menu navigasi dengan LCD ST7735
- Rotary encoder untuk input
- WiFi Access Point
- ESP-NOW untuk komunikasi dengan Plant
- Real-time monitoring

### 3. Plant_Board
Controller utama yang mengendalikan motor DC dan AC pada escalator. Menerima command dari Interface Board dan mengirim data sensor.

**Platform**: ESP32

**Fitur**:
- Kontrol motor DC dengan PWM
- Kontrol motor AC dengan DAC
- PID Controller
- Encoder reading (DC & AC)
- ESP-NOW communication
- LED indicators

### 4. Tes_Interface_esp32
Project testing untuk debugging komponen Interface Board seperti encoder, LCD, dan network.

**Platform**: ESP32

## Arsitektur Sistem

```
┌─────────────────┐           ESP-NOW          ┌─────────────────┐
│ Interface Board │ ◄────────────────────────► │  Plant Board    │
│  (ESP32/8266)   │     Command/Status         │    (ESP32)      │
│                 │                             │                 │
│ - LCD Display   │                             │ - DC Motor      │
│ - Rotary Enc    │                             │ - AC Motor      │
│ - WiFi AP       │                             │ - Encoders      │
└─────────────────┘                             │ - PID Control   │
                                                 └─────────────────┘
```

## Requirements

### Hardware
- ESP32 Development Board (2-3 unit)
- ESP8266 NodeMCU (opsional, untuk Interface)
- LCD ST7735 TFT Display (128x160)
- Rotary Encoder dengan push button
- Motor DC dengan encoder
- Motor AC (opsional)
- Motor Driver

### Software
- [PlatformIO](https://platformio.org/)
- VS Code dengan PlatformIO extension
- Driver CH340/CP2102 (untuk ESP32/ESP8266)

### Libraries
- Adafruit GFX Library
- Adafruit ST7735 Library
- ESP-NOW (built-in)
- WiFi (built-in)

## Quick Start

### 1. Install PlatformIO
```bash
# Install PlatformIO CLI atau gunakan VS Code extension
pip install platformio
```

### 2. Clone/Download Repository
```bash
cd "d:\Kuliah\ITS\SMT3\CBL\Program"
```

### 3. Build & Upload

#### Plant Board (Upload pertama)
```bash
cd Plant_Board
platformio run --target upload
platformio device monitor
# Catat MAC address yang muncul di serial monitor
```

#### Interface Board
```bash
cd Interface_Board
# Edit include/PinConfig.h, sesuaikan PLANT_MAC dengan MAC Plant Board
platformio run --target upload
platformio device monitor
```

## Konfigurasi

### Plant Board
Edit `Plant_Board/include/PlantConfig.h`:
```cpp
// Sesuaikan dengan MAC address Interface Board
#define INTERFACE_MAC {0x50, 0x02, 0x91, 0x78, 0x72, 0xA7}
```

### Interface Board
Edit `Interface_Board/include/PinConfig.h`:
```cpp
// Sesuaikan dengan MAC address Plant Board
#define PLANT_MAC {0x58, 0xBF, 0x25, 0x1A, 0xF7, 0x6C}

// WiFi AP Settings
#define WIFI_AP_SSID "GameBoard_AP"
#define WIFI_AP_PASSWORD "12345678"
```

## Testing

### Test Encoder
```bash
cd Tes_Interface_esp32
platformio run --target upload
platformio device monitor
# Putar encoder dan lihat output
```

### Test Komunikasi ESP-NOW
1. Upload Plant_Board
2. Upload Interface_Board
3. Monitor serial pada kedua board
4. Cek status ESP-NOW di menu Interface

## Troubleshooting

### ESP-NOW tidak terkoneksi
- Pastikan MAC address sudah benar di kedua sisi
- Cek WiFi mode (harus dalam mode STA atau AP+STA)
- Restart kedua board

### Encoder tidak terbaca
- Cek koneksi pin CLK, DT, dan SW
- Pastikan pullup resistor terpasang
- Test dengan program `Tes_Interface_esp32`

### LCD tidak menyala
- Cek koneksi SPI (MOSI, SCK, CS, DC, RST)
- Pastikan power supply cukup (3.3V atau 5V sesuai modul)
- Test pin RST dengan toggle manual

## Serial Monitor

Baud rate untuk semua project: **115200**

```bash
# Monitor dengan PlatformIO
platformio device monitor -b 115200

# Atau gunakan serial monitor lain
# Arduino IDE, PuTTY, dll.
```

## Development Timeline

- **Fase 1**: Development encoder reading dan motor control
- **Fase 2**: Implementation ESP-NOW communication
- **Fase 3**: Interface Board dengan LCD dan menu system
- **Fase 4**: Integration testing dan debugging
- **Fase 5**: Final testing dan deployment

## Tim Pengembang

Proyek CBL Semester 3 - ITS

## Lisensi

Educational Project - ITS 2024
