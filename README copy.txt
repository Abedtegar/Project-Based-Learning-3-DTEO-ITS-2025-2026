# SISTEM KONTROL SEDERHANA

## Eskalator Mini Motor DC dengan Kendali ESP32 menggunakan Kontrol PID

---

### 👥 Tim Pengembang

| No    | Nama                      | NRP        |
|-------|---------------------------|------------|
| 1     | Abednego Tegar Imanto     | 2040241044 |
| 2     | Muhammad Rifky Naufal     | 2040241032 |
| 3     | Piet Avin Dutabuana       | 2040241051 |
| 4     | Dhonan Rachma Dio Putra   | 2040241086 |
| 5     | Luna Aulia Afifah         | 2040241092 |
| 6     | Saniiyah Rif'atunnisa'    | 2040241099 |

**Proyek**: CBL Semester 3 - Departemen Teknik Elektro Otomasi  
**Institusi**: Institut Teknologi Sepuluh Nopember (ITS)  
**Tahun**: 2024/2025

---

## 📋 Deskripsi Proyek

Repository ini berisi seluruh program untuk sistem kontrol escalator berbasis ESP32/ESP8266 yang dikembangkan untuk proyek CBL Semester 3. Sistem ini menggunakan arsitektur terdistribusi dengan dua board utama yang berkomunikasi melalui protokol ESP-NOW untuk kontrol motor DC (escalator) dan motor AC dengan kontrol PID real-time.

## Struktur Project

```
Program/
├── ambil_data_encoder/     # Program testing & kalibrasi encoder motor DC
├── Interface_Board/        # HMI control panel dengan LCD TFT dan encoder
├── Plant_Board/           # Controller utama motor DC/AC dengan PID
├── Tes_Interface_esp32/   # Testing dan debugging interface
├── Design_3D/             # Model 3D box, eskalator, dan interface
├── PCB_Board/             # Desain PCB Interface Board & Plant Board
└── DOKUMEN/               # Data pengukuran motor AC (CSV)
```

## Deskripsi Project


### 1. Interface_Board (HMI - Human Machine Interface)
Board interface sebagai panel kontrol dan monitoring sistem. Menggunakan LCD TFT dengan menu grafis interaktif, rotary encoder sebagai input, dan komunikasi wireless ESP-NOW.

**Platform**: ESP32 / ESP8266  
**Fitur Utama**:

#### A. Sistem Menu Navigasi
- **Main Menu**: 3 menu utama (Escalator, Motor AC, WiFi Status)
- **Escalator Menu**: 
  - Graph Mode: Real-time oscilloscope-style graph dengan buffer 100 sample
  - Control Mode: Setting PID parameters, setpoint, direction, run/stop
- **Motor AC Menu**: 
  - Graph Mode: Real-time monitoring RPM AC motor
  - Control Mode: Konfigurasi PID AC, setpoint, dan control mode
- **WiFi Menu**: Status monitoring ESP-NOW dan WiFi connection

#### B. Sistem Input (Rotary Encoder)
- **Single Click**: Select/Enter menu atau toggle edit mode
- **Double Click**: Toggle encoder direction (UP/DOWN) atau quick motor run/stop di graph mode
- **Long Press (>1 detik)**: Back/Exit ke menu sebelumnya
- **Scroll**: Navigasi menu atau edit nilai parameter (debounced)
- **Encoder Mode**: Toggle UP (increment) atau DOWN (decrement) untuk kontrol yang lebih presisi

#### C. Display & Visualisasi (LCD ST7735 128x160)
- **Grafik Real-time**: 
  - Incremental pixel rendering (oscilloscope-style) untuk efisiensi
  - Buffer 116 pixel dengan mapping ke 100 sample
  - Setpoint line (red dashed)
  - Auto-scaling untuk DC (0-115 RPM) dan AC (0-1500 RPM)
  - Error calculation dan percentage display
- **Control Screen**: Live editing PID parameters (Kp, Ki, Kd, Setpoint)
- **Status Indicators**: Motor state (RUN/STOP), connection status
- **Splash Screen**: Startup animation dengan versi info

#### D. Komunikasi ESP-NOW
- **TX Messages**: Command dan parameter ke Plant Board
  - Motor control (DC/AC run/stop)
  - PID parameters (Kp, Ki, Kd, Setpoint)
  - Direction control (FWD/REV)
  - Speed request flags
- **RX Messages**: Data monitoring dari Plant Board
  - Real-time RPM (DC & AC)
  - Timestamp untuk sinkronisasi
  - PID parameters confirmation
  - Motor status feedback
- **Tagged Float Protocol**: 8-byte frame (4-byte typeID + 4-byte float value)

#### E. Sistem Monitoring PID
- **Response Analysis**: Buffer 500 sample untuk analisis settling time
- **Timestamp Tracking**: 100-entry circular buffer untuk grid waktu
- **Error Metrics**: Perhitungan average error 50 sample terakhir
- **Percentage Error**: Error relatif terhadap setpoint

### 2. Plant_Board (Controller & Actuator)
Controller utama yang menjalankan algoritma PID dan mengendalikan motor DC (escalator) dan motor AC. Board ini bertindak sebagai "otak" sistem dengan kontrol loop tertutup.

**Platform**: ESP32  
**Fitur Utama**:

#### A. Kontrol Motor DC (Escalator)
- **PWM Control**: 12-bit resolution (0-4095), 500 Hz frequency
- **Direction Control**: Pin dedicated untuk CW/CCW
- **Encoder Quadrature**: Channel A (primary) dengan interrupt FALLING edge
- **PPR**: 14 pulses per revolution
- **Sampling Rate**: 50ms (20 Hz) dengan hardware timer
- **Speed Calculation**: RPM = (pulseCount × 60000) / (interval × PPR)
- **Gearbox Ratio**: 200:1 untuk konversi motor RPM ke output RPM

#### B. Kontrol Motor AC
- **PWM DAC Control**: 12-bit PWM untuk DAC, 8-bit output (0-255)
- **Analog Encoder**: ADC 10-bit pada GPIO32
- **Kalman Filter**: 
  - Process noise: 0.01 (tunable)
  - Measurement noise: 3.0 (tunable)
  - Mengurangi noise sensor analog untuk pembacaan RPM yang stabil
- **Scaling Factor**: 2.304147465 untuk konversi ADC ke RPM
- **Voltage Select**: Pin control untuk pemilihan tegangan DAC
- **Sampling Rate**: 10ms (100 Hz)

#### C. PID Controller (Dual Independent)
- **DC PID**:
  - Default: Kp=1.5, Ki=0.1, Kd=0.05
  - Anti-Windup: Integral clamping (±1000)
  - Anti-Stall: Minimum PWM 800 untuk mencegah motor mati
  - Output saturation: ±4095 (12-bit PWM)
  - Sample time: 50ms (sesuai encoder timer)
  
- **AC PID**:
  - Default: Kp=2.0, Ki=0.08, Kd=0.04
  - Anti-Windup: Integral clamping (±1000)
  - Anti-Stall: Minimum PWM 800
  - Output saturation: 0-255 (8-bit DAC)
  - Sample time: 10ms (sesuai encoder timer)
  
- **PID Algorithm**:
  ```
  error = setpoint - measured
  P = Kp × error
  I = Ki × Σ(error × dt)    [with clamping]
  D = Kd × (error - prev_error) / dt
  output = P + I + D        [with saturation & anti-stall]
  ```

#### D. Data Persistence (NVS Storage)
- **Preferences Library**: Non-Volatile Storage di flash ESP32
- **Stored Parameters**:
  - DC PID: Kp, Ki, Kd, Setpoint, Direction
  - AC PID: Kp, Ki, Kd, Setpoint, Voltage
- **Auto-Load**: Parameter dimuat saat startup
- **Auto-Save**: Parameter disimpan saat diterima dari Interface Board
- **Reset Function**: Clear all NVS dan restart

#### E. LED Indicators (4 LED)
- **LED_WIFI (GPIO21)**: Status komunikasi ESP-NOW (ON saat TX success)
- **LED_PID (GPIO19)**: Mode PID aktif/non-aktif
- **LED_DC_CONTROL (GPIO18)**: Motor DC running
- **LED_AC_CONTROL (GPIO5)**: Motor AC running

#### F. Hardware Timer System
- **Timer 0**: DC encoder sampling (50ms)
- **Timer 1**: AC encoder sampling (10ms)
- **ISR Safety**: portENTER_CRITICAL / portEXIT_CRITICAL untuk shared variables
- **Brownout Disable**: WRITE_PERI_REG untuk stabilitas saat PWM tinggi

#### G. Komunikasi ESP-NOW
- **WiFi Mode**: STA (Station) mode
- **MAC Filtering**: Hanya menerima dari Interface Board MAC yang terdaftar
- **Callback Functions**:
  - OnDataSent: Konfirmasi transmisi (LED feedback)
  - receiveData: Parse command dan update parameter
- **Message Types**: 32 jenis message ID (DC/AC control, PID params, etc.)


## Arsitektur Sistem

### Diagram Blok Komunikasi

```
┌──────────────────────────────────────────────────────────────────────┐
│                         INTERFACE BOARD                              │
│                       (ESP32/ESP8266 - HMI)                          │
├──────────────────────────────────────────────────────────────────────┤
│  ┌─────────────┐    ┌──────────────┐    ┌──────────────┐             │
│  │  LCD ST7735 │    │    Rotary    │    │   ESP-NOW    │             │
│  │  128x160px  │◄───┤   Encoder    │◄───┤   Protocol   │             │
│  │  Real-time  │    │  + Button    │    │   (Tagged    │             │
│  │   Graphs    │    │ (Scroll/Clk) │    │    Float)    │             │
│  └─────────────┘    └──────────────┘    └───────┬──────┘             │
│                                                 │                    │
│  Features:                                      │                    │
│  - Menu Navigation (3 levels)                   │ TX: Commands       │
│  - PID Parameter Editor                         │     Parameters     │
│  - Oscilloscope-style Graph                     │     Setpoints      │
│  - Motor Control (Run/Stop)                     │                    │
│  - Real-time Monitoring                         │ RX: RPM Data       │
└─────────────────────────────────────────────────┼────────────────────┘
                                                  │
                              ESP-NOW Wireless    │
                            (2.4GHz, <250 bytes)  │
                                                  │
┌─────────────────────────────────────────────────┼──────────────────────┐
│                          PLANT BOARD            │                      │
│                    (ESP32 - Controller)         │                      │
├─────────────────────────────────────────────────┴──────────────────────┤
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │   ESP-NOW    │    │   Dual PID   │    │   Hardware   │              │
│  │   Protocol   ├───►│  Controller  ├───►│    Timers    │              │
│  │   Receiver   │    │   (DC & AC)  │    │  (50ms/10ms) │              │
│  └──────────────┘    └──────────────┘    └───────┬──────┘              │
│                                                  │                     │
│                      ┌───────────────────────────┴────────────┐        │
│                      │                                        │        │
│              ┌───────▼─────────┐                    ┌─────────▼───────┐│
│              │  DC MOTOR       │                    │   AC MOTOR      ││
│              │  + Encoder      │                    │   + Encoder     ││
│              ├─────────────────┤                    ├─────────────────┤│
│              │ - PWM 12-bit    │                    │ - PWM DAC 8-bit ││
│              │ - Quadrature    │                    │ - Analog ADC    ││
│              │ - 14 PPR        │                    │ - Kalman Filter ││
│              │ - Gearbox 200:1 │                    │ - 0-1500 RPM    ││
│              │ - 0-115 RPM out │                    │ - Voltage Sel   ││
│              │ - Direction Ctl │                    │                 ││
│              └─────────────────┘                    └─────────────────┘│
│                                                                        │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐              │
│  │  NVS Storage │    │  4x LED      │    │  Brownout    │              │
│  │  (PID Params)│    │  Indicators  │    │  Disable     │              │
│  └──────────────┘    └──────────────┘    └──────────────┘              │
│                                                                        │
│  Features:                                                             │
│  - Dual Independent PID (DC 50ms, AC 10ms)                             │
│  - Anti-Windup & Anti-Stall                                            │
│  - Kalman Filter untuk AC encoder                                      │
│  - Parameter Persistence (NVS Flash)                                   │
│  - Real-time Data Transmission                                         │
└────────────────────────────────────────────────────────────────────────┘
```

### Alur Komunikasi Data

**1. Startup Sequence:**
```
Plant Board:
1. Load PID parameters dari NVS Storage
2. Initialize LED indicators
3. Setup ESP-NOW (WiFi STA mode)
4. Initialize encoders (DC & AC)
5. Start PWM timers
6. Start hardware timers (50ms DC, 10ms AC)
7. Send parameter confirmation ke Interface

Interface Board:
1. Initialize LCD ST7735
2. Initialize rotary encoder + button
3. Setup ESP-NOW
4. Show splash screen (2 detik)
5. Load main menu
6. Request Plant Board restart (sync)
```

**2. Real-time Control Loop:**
```
Interface Board (Main Loop ~10ms):
1. Read encoder position (interrupt-based)
2. Detect button clicks (single/double/long)
3. Update menu state (navigate/edit/select)
4. Send commands via ESP-NOW (jika ada perubahan)
5. Receive RPM data dari Plant
6. Update display (incremental rendering)

Plant Board (Timer ISR):
DC Timer (50ms):
  1. Calculate pulse count
  2. Compute RPM = (pulseCount × 60000) / (50 × 14)
  3. Apply gearbox ratio (÷200)
  4. Run PID if enabled
  5. Set PWM output
  6. Send data via ESP-NOW

AC Timer (10ms):
  1. Read ADC value
  2. Apply Kalman filter
  3. Compute RPM = ADC × 2.304147465
  4. Run PID if enabled
  5. Set DAC output
  6. Send data via ESP-NOW
```

**3. PID Control Flow:**
```
┌─────────────────────────────────────────────────────────────┐
│                     PID CONTROL LOOP                        │
├─────────────────────────────────────────────────────────────┤
│  1. Read Setpoint (dari Interface Board)                    │
│  2. Read Current RPM (dari Encoder)                         │
│  3. Calculate Error = Setpoint - Current                    │
│  4. Calculate P = Kp × Error                                │
│  5. Calculate I = Ki × Σ(Error × dt)  [Anti-Windup]         │
│  6. Calculate D = Kd × (Error - PrevError) / dt             │
│  7. Output = P + I + D                                      │
│  8. Apply Saturation (0-4095 DC, 0-255 AC)                  │
│  9. Apply Anti-Stall (minimum PWM 800)                      │
│ 10. Write to PWM/DAC                                        │
│ 11. Store PrevError                                         │
│ 12. Send RPM & Error ke Interface                           │
└─────────────────────────────────────────────────────────────┘
```

**4. ESP-NOW Message Protocol:**
```
Frame Format: [TypeID: 4 bytes][Value: 4 bytes] = 8 bytes total

Message Types (Interface → Plant):
- MSG_DC_KP (11): Kp untuk DC PID
- MSG_DC_KI (12): Ki untuk DC PID
- MSG_DC_KD (13): Kd untuk DC PID
- MSG_DC_Setpoint (14): Target RPM DC motor
- MSG_DC_Control (15): Run/Stop DC motor (1/0)
- MSG_DC_Direction (16): Direction FWD/REV (0/1)
- MSG_AC_KP (21): Kp untuk AC PID
- MSG_AC_KI (22): Ki untuk AC PID
- MSG_AC_KD (23): Kd untuk AC PID
- MSG_AC_Setpoint (24): Target RPM AC motor
- MSG_AC_Control (25): Run/Stop AC motor (1/0)
- MSG_AC_Voltage (26): Voltage selection
- MSG_PID_MODE (30): Enable/Disable PID (1/0)
- MSG_SPD_REQUEST (31): Request speed data (1=DC, 2=AC, 0=off)
- ESP_RESTART (100): Restart Plant Board

Message Types (Plant → Interface):
- MSG_DC_SPEED (10): Current RPM DC motor
- MSG_AC_SPEED (20): Current RPM AC motor
- MSG_TIMESTAMP (32): Timestamp untuk sinkronisasi
- MSG_DC_KP/KI/KD: Confirmation parameter diterima
- MSG_AC_KP/KI/KD: Confirmation parameter diterima
```

## Requirements

### Hardware
- ESP32 Development Board 
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

## Konfigurasi Detail

### 1. Plant Board Configuration

**File: `Plant_Board/include/PlantConfig.h`**

#### A. Pin Configuration

```cpp
// DC Motor Control
#define MOTOR_PWM_PIN 13        // PWM output ke motor driver
#define MOTOR_DIR_PIN 23        // Direction control (CW/CCW)
#define PWM_CHANNEL 0           // LEDC channel
#define PWM_FREQ 500            // 500 Hz PWM frequency
#define PWM_RESOLUTION 12       // 12-bit (0-4095)

// DC Motor Encoder
#define ENCODER_A_PIN 34        // Channel A (primary interrupt)
#define ENCODER_B_PIN 35        // Channel B (direction detection)
#define PPR 14                  // Pulses Per Revolution
#define DCREAD_INTERVAL 50      // Sampling interval 50ms

// AC Motor Control
#define AC_DAC1_PIN 25          // DAC output 1
#define AC_DAC2_PIN 26          // DAC output 2
#define AC_DAC_SOURCE_PIN 27    // Source selection
#define AC_DAC_VOLTAGE_SELECT_PIN 12  // Voltage level selection
#define ACPWM_CHANNEL 1         // LEDC channel untuk DAC
#define ACPWM_FREQ 500          // PWM frequency
#define ACPWM_RESOLUTION 12     // 12-bit resolution

// AC Motor Encoder
#define AC_ENCODER_PIN 32       // Analog input (ADC)
#define ACREAD_INTERVAL 10      // Sampling interval 10ms
#define AC_READ_SCALING_FACTOR 2.304147465f  // ADC to RPM conversion

// LED Indicators
#define LED_WIFI 21             // ESP-NOW communication status
#define LED_PID 19              // PID mode active
#define LED_DC_CONTROL 18       // DC motor running
#define LED_AC_CONTROL 5        // AC motor running
```

#### B. Network Configuration

```cpp
// MAC Address Interface Board (dapatkan dari Serial Monitor Interface)
static const uint8_t INTERFACE_MAC[] = {
    0x50, 0x02, 0x91, 0x78, 0x72, 0xA7  // SESUAIKAN DENGAN MAC ANDA
};

// Data transmission interval
#define DATA_SEND_INTERVAL 1000  // Kirim data setiap 1000ms
```

#### C. PID Tuning Parameters

**File: `Plant_Board/src/PID.cpp`**

```cpp
// DC Motor PID (Escalator)
volatile float DCkP = 1.5f;      // Proportional gain
volatile float DCkI = 0.1f;      // Integral gain
volatile float DCkD = 0.05f;     // Derivative gain
volatile float DCsetpoint = 50.0f;  // Target RPM (output shaft)

// AC Motor PID
volatile float ACkP = 2.0f;      // Proportional gain
volatile float ACkI = 0.08f;     // Integral gain
volatile float ACkD = 0.04f;     // Derivative gain
volatile float ACsetpoint = 0.0f;   // Target RPM

// Anti-Windup Limits
#define DC_INTEGRAL_MAX 1000.0   // Max integral accumulation
#define DC_INTEGRAL_MIN -1000.0
#define AC_INTEGRAL_MAX 1000.0
#define AC_INTEGRAL_MIN -1000.0

// Anti-Stall Minimum PWM
#define DC_PWM_MIN 800.0f        // Minimum PWM untuk mencegah stall
#define AC_PWM_MIN 800.0f
```

#### D. Kalman Filter Tuning (AC Motor)

**File: `Plant_Board/src/Plant.cpp`**

```cpp
// Kalman Filter Parameters
float AC_process_noise = 0.01;   // Process noise covariance (Q)
float AC_error_measure = 4.0;    // Measurement noise covariance (R)

// Adjust di setup():
ACsetKalmanParams(0.01, 3.0);   // (process_noise, measure_noise)
// - Turunkan process_noise untuk response lebih lambat tapi smooth
// - Turunkan measure_noise jika sensor akurat
```

### 2. Interface Board Configuration

**File: `Interface_Board/include/PinConfig.h`**

#### A. Pin Configuration (ESP32)

```cpp
// LCD ST7735 (SPI)
#define TFT_CS 15               // Chip Select
#define TFT_RST 4               // Reset
#define TFT_DC 5                // Data/Command
// Hardware SPI: MOSI=23, SCK=18

// Rotary Encoder
#define ENCODER_CLK 27          // Clock (Channel A) - NOT USED in interrupt
#define ENCODER_DT 26           // Data (Channel B) - PRIMARY interrupt
#define ENCODER_SW 25           // Switch button (polling)
```

#### B. Pin Configuration (ESP8266)

```cpp
// LCD ST7735 (SPI)
#define TFT_CS 15               // GPIO15
#define TFT_RST 4               // GPIO4
#define TFT_DC 5                // GPIO5
// Hardware SPI: MOSI=GPIO13, SCK=GPIO14

// Rotary Encoder
#define ENCODER_CLK 12          // GPIO12
#define ENCODER_DT 2            // GPIO2 - PRIMARY interrupt
#define ENCODER_SW 16           // GPIO16
```

#### C. Network Configuration

```cpp
// WiFi AP (Optional - currently not used)
#define WIFI_AP_SSID "GameBoard_AP"
#define WIFI_AP_PASSWORD "12345678"

// MAC Address Plant Board (dapatkan dari Serial Monitor Plant)
static const uint8_t PLANT_MAC[6] = {
    0x7C, 0x9E, 0xBD, 0x48, 0x85, 0xB4  // SESUAIKAN DENGAN MAC ANDA
};
```

#### D. Display Configuration

**File: `Interface_Board/src/src/ControlMenuSystem_simple.cpp`**

```cpp
// Graph Scaling
constexpr float ESC_GRAPH_MAX = 115.0f;   // Max RPM DC (output shaft)
constexpr float AC_GRAPH_MAX = 1500.0f;   // Max RPM AC

// History Buffer
#define MAX_SAMPLES 100          // Jumlah sample dalam buffer
#define GRAPH_WIDTH 116          // Pixel width grafik

// Timing
#define DEBOUNCE_TIME 50         // Button debounce (ms)
#define LONG_PRESS_TIME 1000     // Long press detection (ms)
#define DOUBLE_CLICK_TIME 400    // Double click window (ms)
```

### 3. Cara Mendapatkan MAC Address

#### Plant Board:

```bash
cd Plant_Board
platformio device monitor -b 115200
```

Output akan menampilkan:
```
=== PLANT BOARD STARTUP ===
ESP-NOW MAC Address: 7C:9E:BD:48:85:B4
```

Salin MAC address ini ke `Interface_Board/include/PinConfig.h`

#### Interface Board:

```bash
cd Interface_Board
platformio device monitor -b 115200
```

Output akan menampilkan MAC address ESP8266/ESP32.
Salin ke `Plant_Board/include/PlantConfig.h`

### 4. PID Tuning Guide

#### Metode Ziegler-Nichols (Manual Tuning)

1. **Set Ki = 0, Kd = 0**
2. **Naikkan Kp** hingga sistem oscillate (bergetar stabil)
3. **Catat Ku (Kp critical)** dan **Tu (periode oscillasi)**
4. **Hitung parameter**:
   - Kp = 0.6 × Ku
   - Ki = 1.2 × Ku / Tu
   - Kd = 0.075 × Ku × Tu

#### Fine Tuning:

**Kp (Proportional)**:
- Terlalu kecil: Response lambat, steady-state error besar
- Terlalu besar: Overshoot, oscillation
- Optimal: Response cepat tanpa overshoot berlebih

**Ki (Integral)**:
- Terlalu kecil: Steady-state error tidak hilang
- Terlalu besar: Integral windup, slow settling
- Optimal: Eliminasi steady-state error dalam waktu wajar

**Kd (Derivative)**:
- Terlalu kecil: Overshoot besar
- Terlalu besar: Noise amplification, sistem tidak stabil
- Optimal: Damping overshoot tanpa noise berlebih

#### Tuning Melalui Interface:

1. Pilih menu **Escalator** atau **Motor AC**
2. Pilih **Control** (single click di graph mode)
3. Navigate ke parameter yang ingin diedit
4. Single click untuk enter edit mode
5. Putar encoder untuk adjust nilai
6. Single click lagi untuk save & transmit ke Plant

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

### 1. ESP-NOW Tidak Terkoneksi

**Gejala:**
- LED_WIFI pada Plant Board tidak menyala
- Status ESP-NOW di menu WiFi menampilkan "FAIL"
- Tidak ada data RPM yang diterima Interface Board

**Solusi:**

A. **Verifikasi MAC Address**
```bash
# Plant Board
platformio device monitor -b 115200
# Lihat output: "ESP-NOW MAC Address: XX:XX:XX:XX:XX:XX"

# Pastikan MAC ini sama dengan yang di Interface_Board/include/PinConfig.h
```

B. **Cek WiFi Mode**
```cpp
// Plant Board: WiFi.mode(WIFI_STA) - harus STA mode
// Interface Board: WiFi.mode(WIFI_STA) atau WIFI_AP_STA
```

C. **Restart Sequence**
1. Upload Plant Board → tunggu hingga "ESP-NOW siap"
2. Upload Interface Board → auto send restart command
3. Monitor Serial kedua board untuk konfirmasi

D. **Cek Channel WiFi**
- ESP-NOW harus di channel yang sama (default: 0 = auto)
- Jika ada WiFi router nearby, bisa conflict

**Debug Command:**
```cpp
// Tambahkan di Plant_Board/src/PlantESPNow.cpp
Serial.print("Peer add result: ");
Serial.println(esp_now_add_peer(&peerInfo));  // Harus return 0 (ESP_OK)
```

### 2. Encoder Tidak Terbaca / Count Tidak Akurat

**Gejala:**
- RPM selalu 0 atau tidak berubah
- Count encoder tidak increment saat motor berputar
- Arah putaran terbalik

**Solusi:**

A. **Cek Koneksi Hardware**
```
Encoder Pin:
- Channel A (CLK) → ESP32 GPIO34/35 (input only pins)
- Channel B (DT)  → ESP32 GPIO35/34
- VCC → 3.3V (JANGAN 5V!)
- GND → GND
- Pull-up resistor: 10kΩ ke VCC (jika encoder tidak ada internal pull-up)
```

B. **Test Encoder Standalone**
```bash
cd ambil_data_encoder
platformio run --target upload
platformio device monitor

# Putar encoder manual, lihat output count
```

C. **Verifikasi Interrupt**
```cpp
// Interface Board: Hanya attach ENCODER_DT dengan FALLING edge
attachInterrupt(digitalPinToInterrupt(ENCODER_DT), DChandleEncoder, FALLING);
// JANGAN attach CLK untuk mencegah double trigger

// Plant Board: Attach ENCODER_A_PIN dengan CHANGE
attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), DChandleEncoderA, CHANGE);
```

D. **Direction Terbalik**
- Swap Channel A dan B di konfigurasi
- Atau toggle `DCDirection` di logika

E. **Noise / Bouncing**
- Tambahkan capacitor 100nF antara setiap pin encoder dan GND
- Kurangi panjang kabel encoder
- Gunakan shielded cable untuk encoder

### 3. LCD Tidak Menyala / Blank Screen

**Gejala:**
- LCD putih atau blank setelah power on
- Backlight nyala tapi tidak ada gambar
- Splash screen tidak muncul

**Solusi:**

A. **Cek Koneksi SPI**
```
LCD ST7735 Pin → ESP32/ESP8266:
- VCC → 3.3V atau 5V (sesuai spesifikasi LCD)
- GND → GND
- CS  → GPIO15 (TFT_CS)
- RST → GPIO4 (TFT_RST)
- DC  → GPIO5 (TFT_DC)
- MOSI → GPIO23 (ESP32) atau GPIO13 (ESP8266)
- SCK → GPIO18 (ESP32) atau GPIO14 (ESP8266)
- LED → 3.3V (backlight)
```

B. **Test Koneksi dengan Multimeter**
- Cek continuity dari ESP pin ke LCD pin
- Cek tegangan VCC = 3.3V atau 5V
- Cek GND tidak short dengan VCC

C. **Manual Reset LCD**
```cpp
// Tambahkan di setup() sebelum tft.initR()
digitalWrite(TFT_RST, LOW);
delay(100);
digitalWrite(TFT_RST, HIGH);
delay(100);
```

D. **Cek Inisialisasi LCD**
```cpp
// Ganti INITR_BLACKTAB dengan yang lain jika blank
tft.initR(INITR_BLACKTAB);   // Coba: INITR_GREENTAB, INITR_REDTAB
tft.setRotation(0);           // Coba: 0, 1, 2, 3
```

E. **Power Supply Tidak Cukup**
- LCD butuh ~50-100mA saat backlight ON
- Gunakan power supply minimal 500mA
- Jangan powered dari USB komputer yang lemah

### 4. PID Tidak Stabil / Oscillating

**Gejala:**
- Motor bergetar (hunting)
- RPM overshoot berlebihan
- Tidak bisa mencapai setpoint

**Solusi:**

A. **Tuning PID Terlalu Agresif**
```cpp
// Mulai dari konservatif:
DCkP = 0.5;   // Kurangi Kp hingga oscillation hilang
DCkI = 0.01;  // Mulai dari Ki sangat kecil
DCkD = 0.0;   // Set Kd = 0 dulu

// Naikkan Kp bertahap hingga response cepat tapi tidak oscillate
// Tambahkan Ki untuk eliminasi steady-state error
// Tambahkan Kd jika masih overshoot
```

B. **Anti-Windup Tidak Efektif**
```cpp
// Pastikan integral clamping aktif
if (DCintegralSum > DC_INTEGRAL_MAX)
    DCintegralSum = DC_INTEGRAL_MAX;
else if (DCintegralSum < DC_INTEGRAL_MIN)
    DCintegralSum = DC_INTEGRAL_MIN;
```

C. **Sampling Rate Terlalu Lambat**
- DC: 50ms (20 Hz) - OK untuk most applications
- AC: 10ms (100 Hz) - Bisa turunkan jika masih oscillate
- Semakin cepat motor, perlu sampling rate lebih tinggi

D. **Motor Stall (Anti-Stall Aktif Terlalu Sering)**
```cpp
// Turunkan DC_PWM_MIN jika motor sering "jump"
#define DC_PWM_MIN 600.0f  // Default 800, coba turunkan
```

E. **Mechanical Load Berubah**
- PID tuning valid untuk specific load
- Jika load berubah drastis, perlu re-tune atau adaptive PID

### 5. Motor Tidak Berputar / Stuck

**Gejala:**
- PWM output sudah benar tapi motor diam
- Motor bergetar tapi tidak rotate
- Motor berputar hanya di PWM tinggi

**Solusi:**

A. **Cek Motor Driver**
```
Driver Pin:
- PWM signal OK? (cek dengan oscilloscope atau LED)
- Direction signal OK?
- Enable pin HIGH?
- Power supply driver cukup? (minimal 1A untuk motor kecil)
```

B. **Cek Power Supply Motor**
- Tegangan motor sesuai rating (6V, 12V, 24V, etc.)
- Current rating cukup (motor butuh 2-5A saat start)
- Cek voltage drop saat motor start

C. **PWM Frequency Terlalu Rendah/Tinggi**
```cpp
// Coba adjust PWM frequency
#define PWM_FREQ 500   // Default, coba 1000 atau 20000
```

D. **Dead Zone / Starting Torque**
```cpp
// Motor perlu minimum PWM untuk overcome starting torque
#define DC_PWM_MIN 1000.0f  // Naikkan dari 800
```

E. **Motor Rusak / Bearing Macet**
- Coba putar motor manual → harus smooth
- Cek dengan multimeter: resistance coil motor normal

### 6. Data Serial Monitor Corrupt / Tidak Terbaca

**Gejala:**
- Character acak di serial monitor
- Data tidak lengkap atau putus-putus

**Solusi:**

A. **Cek Baud Rate**
```bash
# Pastikan 115200 di semua board
platformio device monitor -b 115200
```

B. **Driver USB-Serial**
- Install driver CH340 atau CP2102
- Windows: Device Manager → Ports → update driver
- Linux: `sudo usermod -a -G dialout $USER` (logout-login)

C. **Kabel USB Rusak**
- Ganti dengan kabel USB yang bagus (data + power)
- Jangan gunakan charging-only cable

D. **Buffer Overflow**
```cpp
// Kurangi Serial.print() di dalam ISR
// Gunakan flag dan print di main loop
```

### 7. ESP32 Brownout / Restart Terus

**Gejala:**
- ESP32 restart sendiri saat motor start
- Serial output: "Brownout detector was triggered"

**Solusi:**

A. **Disable Brownout Detector** (temporary)
```cpp
// Di setup() Plant_Board
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
```

B. **Power Supply Tidak Cukup** (root cause)
- ESP32 butuh minimal 500mA
- Motor + ESP32 total bisa 3-5A
- JANGAN powered motor dari ESP32 VCC
- Gunakan power supply terpisah untuk motor dan ESP32
- Common ground antara power supply motor dan ESP32

C. **Decoupling Capacitor**
- Tambahkan 100µF capacitor di VCC ESP32
- Tambahkan 0.1µF ceramic capacitor di setiap VCC pin

### 8. Kalman Filter AC Motor Tidak Smooth

**Gejala:**
- RPM AC masih noisy
- Filter tidak efektif

**Solusi:**

A. **Tuning Parameter Kalman**
```cpp
// Naikkan measurement noise jika sensor sangat noisy
ACsetKalmanParams(0.01, 5.0);  // process_noise, measure_noise

// Turunkan process noise untuk smoother tapi slower response
ACsetKalmanParams(0.005, 3.0);
```

B. **Moving Average Tambahan**
```cpp
// Tambahkan simple moving average sebelum Kalman
#define MA_SIZE 5
float ma_buffer[MA_SIZE];
float ma_sum = 0;
int ma_index = 0;

// Update buffer
ma_sum -= ma_buffer[ma_index];
ma_buffer[ma_index] = raw_rpm;
ma_sum += raw_rpm;
ma_index = (ma_index + 1) % MA_SIZE;
float filtered = ma_sum / MA_SIZE;
```

C. **Hardware Filter**
- Tambahkan RC filter di output sensor analog
- R = 10kΩ, C = 100nF → cutoff ~160 Hz

## Serial Monitor

Baud rate untuk semua project: **115200**

```bash
# Monitor dengan PlatformIO
platformio device monitor -b 115200

# Atau gunakan serial monitor lain
# Arduino IDE, PuTTY, dll.
```

## Advanced Features & Technical Details

### 1. Real-time Graph Rendering (Oscilloscope-Style)

Interface Board menggunakan teknik **incremental pixel rendering** untuk update grafik secara efisien tanpa full redraw.

**Algoritma:**
```cpp
// Buffer Y-position untuk setiap kolom pixel (116 kolom)
static int escGraphY[116];

// Setiap data baru masuk:
1. Shift buffer kanan ke kiri (escGraphY[i] = escGraphY[i-1])
2. Hapus pixel lama di posisi yang berubah
3. Gambar pixel baru di posisi baru
4. Update setpoint line (red dashed)
5. Update metrics (speed, error, percent error)

// Hasil: Smooth scrolling graph tanpa flicker
```

**Metrics Calculation:**
- **Current Speed**: Nilai RPM terbaru dari Plant Board
- **Setpoint**: Target RPM (red line)
- **Error**: `Current - Setpoint`
- **Percent Error**: Average absolute error 50 sample terakhir / setpoint × 100%

### 2. Encoder Quadrature Decoding

**Plant Board (DC Motor):**
```cpp
// Single-edge interrupt pada Channel A dengan pembacaan Channel B
void IRAM_ATTR DChandleEncoderA() {
    int stateA = digitalRead(ENCODER_A_PIN);
    int stateB = digitalRead(ENCODER_B_PIN);
    
    if (stateA == HIGH) {
        if (stateB == LOW) DCencoder++;  // CW rotation
        else DCencoder--;                 // CCW rotation
    } else {
        if (stateB == HIGH) DCencoder++;
        else DCencoder--;
    }
}

// Keuntungan: 
// - Deteksi arah otomatis
// - Resolution 4x (jika attach kedua channel)
// - Debouncing hardware dengan interrupt CHANGE
```

**Interface Board (Rotary Encoder):**
```cpp
// Mode-based increment/decrement
if (encoderMode == ENCODER_UP) {
    DCencoder++;   // Increment mode
} else {
    DCencoder--;   // Decrement mode
}

// Keuntungan:
// - User bisa toggle arah scroll
// - Lebih intuitif untuk menu navigation
// - Single interrupt pada DT pin (less noise)
```

### 3. ESP-NOW Protocol Efficiency

**Frame Structure:**
```
Byte 0-3: TypeID (int32_t)  → Identifier jenis data
Byte 4-7: Value (float)      → Payload data

Total: 8 bytes per message
```

**Keuntungan:**
- **Compact**: Hanya 8 bytes (vs JSON ~50-100 bytes)
- **Fast**: Parsing langsung dengan memcpy, no string parsing
- **Type-safe**: TypeID mencegah misinterpretation
- **Scalable**: Mudah tambah message type baru

**Latency Analysis:**
- ESP-NOW TX time: ~1-2ms
- Processing time: <1ms
- Total latency: <5ms (real-time control)

### 4. NVS Storage Implementation

**Persistence Layer:**
```cpp
Preferences preferences;

// Save (Plant Board)
preferences.begin("plant_config", false);  // namespace
preferences.putFloat("dc_kp", DCkP);
preferences.putFloat("dc_ki", DCkI);
// ... dst

// Load (Plant Board startup)
DCkP = preferences.getFloat("dc_kp", 1.5f);  // default 1.5
DCkI = preferences.getFloat("dc_ki", 0.1f);
```

**Benefits:**
- **Non-volatile**: Parameter tersimpan meski power off
- **Fast**: Flash read <1ms
- **Reliable**: Wear leveling otomatis oleh ESP32
- **Simple API**: Key-value store seperti database

**Use Case:**
1. User tuning PID via Interface Board
2. Interface send parameter ke Plant Board
3. Plant Board auto-save ke NVS
4. Next power-on: Load parameter terakhir

### 5. Kalman Filter Implementation (AC Motor)

**Algorithm:**
```cpp
// Prediction step
predicted_estimate = estimate;
predicted_error = error_estimate + process_noise;

// Update step
kalman_gain = predicted_error / (predicted_error + error_measure);
estimate = predicted_estimate + kalman_gain * (measurement - predicted_estimate);
error_estimate = (1.0 - kalman_gain) * predicted_error;
```

**Tuning Guide:**
- **process_noise** (Q): Ketidakpastian model (default 0.01)
  - Kecil → filter percaya model, smooth tapi slow
  - Besar → filter percaya measurement, fast tapi noisy
  
- **error_measure** (R): Noise sensor (default 3.0)
  - Kecil → filter percaya sensor sangat akurat
  - Besar → filter assume sensor noisy, lebih smooth

**Performance:**
- Execution time: <0.5ms per sample
- Memory: ~20 bytes (5 float variables)
- Noise reduction: ~70-80% vs raw ADC

### 6. Anti-Windup & Anti-Stall Mechanisms

**Anti-Windup (Integral Clamping):**
```cpp
// Problem: Integral term membesar terus saat error persist
// Solution: Clamp integral sum dalam range aman

integralSum += error * dt;
if (integralSum > INTEGRAL_MAX) integralSum = INTEGRAL_MAX;
if (integralSum < INTEGRAL_MIN) integralSum = INTEGRAL_MIN;

// Effect: Prevent integral windup, faster recovery dari saturation
```

**Anti-Stall (Minimum PWM):**
```cpp
// Problem: Motor stuck karena PWM terlalu kecil (below starting torque)
// Solution: Set minimum PWM threshold

if (output > 0.0 && output < PWM_MIN) 
    output = PWM_MIN;

// Effect: Motor selalu dapat cukup torque untuk bergerak
```

## Performance Metrics

### Timing Analysis

**Interface Board:**
- Main loop: ~10ms cycle time
- Menu render: <5ms (incremental)
- Full redraw: ~50ms (only on menu change)
- Button detection: <1ms
- ESP-NOW TX: ~2ms

**Plant Board:**
- DC PID loop: 50ms (20 Hz)
- AC PID loop: 10ms (100 Hz)
- Encoder ISR: <10µs
- ESP-NOW RX callback: <1ms
- NVS write: ~5ms (non-blocking)

### Memory Usage

**Interface Board (ESP8266/ESP32):**
- Program: ~150KB
- RAM: ~15KB
  - Display buffer: 4KB
  - Speed history: 400 bytes × 2 = 800 bytes
  - PID monitoring: 4KB
  - Stack: ~8KB

**Plant Board (ESP32):**
- Program: ~200KB
- RAM: ~20KB
  - Encoder variables: <1KB
  - PID state: <1KB
  - NVS cache: ~2KB
  - Stack: ~15KB

### Communication Bandwidth

**ESP-NOW (2.4GHz):**
- Max throughput: ~250 kbps (teoritis)
- Actual usage: ~1-2 kbps (8 bytes @ 100 Hz)
- Utilization: <1% (very efficient)
- Range: ~50-100m (line of sight)

## Data Logging & Analysis

### Serial Output Format

**Plant Board (DC Motor):**
```
waktu(ms), encoder_count, rpm, setpoint, error, pwm, kP, kI, kD
1000, 2800, 45.5, 50.0, 4.5, 2048, 1.5, 0.1, 0.05
```

**Plant Board (AC Motor):**
```
waktu(ms), rpm, setpoint, error, dac, kP, kI, kD
1000, 1250.3, 1200.0, -50.3, 180, 2.0, 0.08, 0.04
```

### Data Collection untuk PID Tuning

```bash
# Redirect serial output ke CSV file
platformio device monitor -b 115200 > data_pengukuran.csv

# Buka di Excel/Spreadsheet untuk plot graph
# Kolom: Time, RPM, Setpoint, Error, Output
# Analisis: Rise time, settling time, overshoot, steady-state error
```

## Best Practices

### 1. Development Workflow

```
1. Test hardware standalone (encoder, motor, LCD)
2. Test communication (ESP-NOW ping-pong)
3. Implement control loop (PID) dengan serial debug
4. Integrate dengan interface board
5. Fine-tune PID parameters
6. Add safety features (brownout, overcurrent, timeout)
```

### 2. Safety Considerations

- **Brownout Protection**: Disable detector untuk stabilitas
- **Watchdog Timer**: (Optional) Auto-reset jika system hang
- **Motor Overcurrent**: Limit max PWM jika motor overheat
- **Encoder Sanity Check**: Reset jika count anomali
- **Communication Timeout**: Stop motor jika ESP-NOW disconnect >1s

### 3. Code Organization

```
Plant_Board/
├── include/           # Header files
│   ├── PlantConfig.h  # Pin & constants (UBAH INI)
│   ├── Plant.h        # Motor control functions
│   ├── PID.h          # PID algorithm
│   └── PlantESPNow.h  # Communication protocol
├── src/              # Implementation
│   ├── main.cpp      # Setup & loop
│   ├── Plant.cpp     # Motor & encoder logic
│   ├── PID.cpp       # PID implementation
│   └── PlantESPNow.cpp  # ESP-NOW handlers
└── platformio.ini    # Build config (platform, board, libs)
```

### 4. Debugging Tips

**Serial Debug Levels:**
```cpp
#define DEBUG_LEVEL 2

#if DEBUG_LEVEL >= 1
  Serial.println("Basic info");
#endif

#if DEBUG_LEVEL >= 2
  Serial.print("Detailed: "); Serial.println(value);
#endif

#if DEBUG_LEVEL >= 3
  Serial.println("Verbose ISR debug");
#endif
```

**Performance Profiling:**
```cpp
unsigned long t_start = micros();
// ... code to profile ...
unsigned long t_end = micros();
Serial.print("Execution time: ");
Serial.print(t_end - t_start);
Serial.println(" us");
```

## Future Enhancements

### Possible Improvements

1. **Adaptive PID**: Auto-tuning berdasarkan system response
2. **Web Dashboard**: HTTP server untuk monitoring via browser
3. **Data Logger**: SD card untuk long-term data recording
4. **Multi-Motor**: Kontrol lebih dari 2 motor dengan priority scheduling
5. **Safety Interlocks**: Sensor posisi, limit switch, emergency stop
6. **MQTT Integration**: IoT cloud monitoring (Thingsboard, Blynk)
7. **OTA Update**: Wireless firmware update untuk field deployment
8. **LCD Touch**: Ganti rotary encoder dengan touchscreen
9. **Voice Control**: Integrasi dengan Google Assistant / Alexa
10. **Machine Learning**: Predictive maintenance based on vibration/current

## FAQ (Frequently Asked Questions)

**Q: Kenapa menggunakan ESP-NOW instead of WiFi/MQTT?**
A: ESP-NOW lebih cepat (latency <5ms vs ~50-100ms), tidak perlu router, dan lebih reliable untuk real-time control.

**Q: Bisa ganti ESP8266 dengan ESP32 atau sebaliknya?**
A: Bisa, tapi perlu adjust pin configuration dan library (ESP-NOW API sedikit beda).

**Q: Berapa jarak maksimal ESP-NOW?**
A: ~50-100m line of sight, ~10-30m indoor (tergantung obstacle).

**Q: Kalau MAC address berubah setelah flash?**
A: Kemungkinan ganti board atau flash erase. Cek ulang MAC dan update di config.

**Q: PID sudah optimal, tapi masih oscillate. Kenapa?**
A: Kemungkinan mechanical issue (bearing, load, friction), bukan tuning problem.

**Q: Bisa kontrol motor stepper?**
A: Bisa, tapi perlu ganti library kontrol motor (AccelStepper) dan logika encoder.

**Q: Encoder count selalu reset ke 0. Kenapa?**
A: Kemungkinan ISR overhead atau variable corruption. Gunakan `volatile` dan `portENTER_CRITICAL`.

**Q: LCD kadang glitch saat motor start. Solusi?**
A: Power supply issue. Gunakan capacitor besar (1000µF) di VCC atau power terpisah.

## Development Timeline

- **Fase 1**: Hardware setup & standalone testing (encoder, motor, LCD)
- **Fase 2**: ESP-NOW communication development & testing
- **Fase 3**: PID controller implementation dengan tuning
- **Fase 4**: Interface Board menu system dengan real-time graph
- **Fase 5**: Integration testing & bug fixing
- **Fase 6**: NVS storage & parameter persistence
- **Fase 7**: Kalman filter untuk AC motor
- **Fase 8**: Final optimization & documentation

## References & Resources

### Documentation
- [ESP32 Official Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
- [ESP-NOW Guide](https://randomnerdtutorials.com/esp-now-esp32-arduino-ide/)
- [PID Control Theory](https://en.wikipedia.org/wiki/PID_controller)
- [Kalman Filter Explained](https://www.kalmanfilter.net/default.aspx)

### Libraries
- [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit ST7735 Library](https://github.com/adafruit/Adafruit-ST7735-Library)
- [ESP32 Preferences](https://github.com/espressif/arduino-esp32/tree/master/libraries/Preferences)

### Tools
- [PlatformIO](https://platformio.org/) - Build system & IDE
- [Serial Plot](https://hackaday.io/project/5334-serialplot-realtime-plotting-software) - Real-time data visualization
- [Logic Analyzer](https://www.saleae.com/) - Debug encoder signals

## Kontributor

Proyek CBL Semester 3 - ITS 2024/2025

**Team Members:**
- [Nama] - Plant Board Development
- [Nama] - Interface Board Development
- [Nama] - PID Tuning & Testing
- [Nama] - Hardware Design & Integration

## Lisensi

Educational Project Pjbl3-8 - Institut Teknologi Sepuluh Nopember (ITS) 2024

---

**Last Updated**: December 5, 2025
**Version**: 1.0
**Dokumentasi**: README.md
