# Ambil Data Encoder

Program untuk mengambil dan membaca data dari encoder motor DC menggunakan ESP32.

## Deskripsi

Program ini membaca pulsa encoder motor DC dengan resolusi quadrature (4x) dan menghitung kecepatan motor dalam RPM. Data ditampilkan melalui Serial Monitor.

## Fitur

- Pembacaan encoder quadrature (Pin A & B)
- Perhitungan RPM real-time
- Interrupt-based encoder reading
- Timer-based sampling (10ms interval)
- Kontrol PWM motor DC

## Pin Configuration

| Komponen | Pin |
|----------|-----|
| Encoder A | GPIO 34 |
| Encoder B | GPIO 35 |
| Motor PWM | GPIO 13 |
| Motor DIR | GPIO 22 |
| LED WiFi | GPIO 21 |

## Output Serial

Format: `encoder_count, pulse_count, rpm`

Contoh:
```
1024 , 14 , 120.5
1038 , 14 , 120.5
```

## Build & Upload

```bash
platformio run --target upload
platformio device monitor
```

## Parameter

- **PPR**: 14 (Pulses Per Revolution)
- **Sampling Rate**: 10ms
- **Baud Rate**: 115200
