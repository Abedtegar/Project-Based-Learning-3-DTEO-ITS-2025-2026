# Plant Board

Board kontrol utama untuk sistem escalator dengan kontrol motor DC, motor AC, PID controller, dan komunikasi ESP-NOW.

## Deskripsi

Plant Board adalah controller utama yang mengendalikan motor DC dan AC pada escalator. Board ini menerima command dari Interface Board melalui ESP-NOW dan mengirimkan data sensor kembali untuk monitoring.

## Fitur

- **DC Motor Control**: Kontrol kecepatan motor DC dengan PWM
- **AC Motor Control**: Kontrol motor AC dengan DAC
- **Encoder Reading**: Pembacaan encoder untuk feedback kecepatan
- **PID Controller**: Kontrol kecepatan motor dengan PID
- **ESP-NOW Communication**: Komunikasi wireless dengan Interface Board
- **LED Indicators**: Indikator status untuk berbagai fungsi
- **Sensor Support**: Support untuk sensor suhu dan arus

## Pin Configuration

### Motor DC
| Komponen | Pin |
|----------|-----|
| Motor PWM | GPIO 13 |
| Motor DIR | GPIO 22 |
| Encoder A | GPIO 34 |
| Encoder B | GPIO 35 |

### Motor AC
| Komponen | Pin |
|----------|-----|
| DAC 1 | GPIO 25 |
| DAC 2 | GPIO 26 |
| DAC Source | GPIO 27 |
| Voltage Select | GPIO 12 |
| AC Encoder | GPIO 32 |

### LED Indicators
| LED | Pin | Fungsi |
|-----|-----|--------|
| WiFi | GPIO 21 | Status WiFi |
| PID | GPIO 19 | PID Active |
| DC Control | GPIO 18 | DC Motor Control |
| AC Control | GPIO 5 | AC Motor Control |

### Sensors
| Sensor | Pin |
|--------|-----|
| Speed Sensor | GPIO 36 |
| Temp Sensor | GPIO 39 |
| Current Sensor | GPIO 33 |

## Parameter Motor

- **PPR (Pulses Per Revolution)**: 14
- **DC Read Interval**: 5ms
- **AC Read Interval**: 5ms
- **PWM Frequency**: 5000 Hz
- **PWM Resolution**: 8-bit (0-255)

## PID Settings

- Default PID Interval: 5ms
- Tunable parameters: Kp, Ki, Kd

## Network Configuration

- **Interface MAC**: `{0x50, 0x02, 0x91, 0x78, 0x72, 0xA7}`
- **Data Send Interval**: 1000ms (1 detik)

## Build & Upload

```bash
# Build project
platformio run

# Upload ke ESP32
platformio run --target upload

# Monitor serial
platformio device monitor
```

## Konfigurasi

Edit `include/PlantConfig.h` untuk:
- Pin assignments
- Motor parameters
- PID settings
- Network configuration
- Interface Board MAC Address

## Class Structure

- **Plant**: Main control class
- **PlantESPNow**: ESP-NOW communication handler
- **PID**: PID controller
- **LedControl**: LED indicator management

## Dependencies

- ESP-NOW (built-in)
- Arduino Framework
