# Flowchart dan Logika Algoritma Sistem Plant Board & Interface Board

## 1. SISTEM PLANT BOARD

### 1.1 Flowchart Utama Plant Board

```
┌─────────────────────┐
│   START (Setup)     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────────┐
│ Disable Brownout        │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Init LED Control        │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Init ESP-NOW            │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Init Data Storage       │
│ (Load PID Parameters)   │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Init DC Motor           │
│ - Encoder               │
│ - Timer                 │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Init AC Motor           │
│ - Encoder               │
│ - Timer                 │
│ - Kalman Filter         │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Update PID Parameters   │
│ to Interface Board      │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│    MAIN LOOP            │
└──────────┬──────────────┘
           │
           ▼
       ┌───┴───┐
       │       │
       ▼       ▼
   ┌─────┐ ┌─────┐
   │ DC  │ │ AC  │
   │Speed│ │Speed│
   │Req? │ │Req? │
   └──┬──┘ └──┬──┘
      │       │
      ▼       ▼
   ┌─────┐ ┌─────┐
   │DC   │ │AC   │
   │PID  │ │PID  │
   │Proc.│ │Proc.│
   └──┬──┘ └──┬──┘
      │       │
      └───┬───┘
          │
          ▼
      (Loop Back)
```

### 1.2 Algoritma DC Motor Control dengan PID

```
ALGORITMA: DC_Motor_PID_Control
INPUT: DCsetpoint (target RPM), DCMode (ON/OFF), PIDMODE (PID/Manual)
OUTPUT: PWM signal ke motor DC

MULAI
1. Timer Interrupt setiap DCREAD_INTERVAL (50ms)
   - Set flag DCnewDataReady = TRUE
   - Jika dcspeedRequest = FALSE, matikan motor

2. JIKA DCnewDataReady = TRUE
   a. Baca encoder:
      - DCpulseCount = DCencoder - DClastEncoder
      - DClastEncoder = DCencoder
   
   b. Hitung RPM:
      - DCrpm = (abs(DCpulseCount) * 60000) / (DCREAD_INTERVAL * PPR)
      - DCGearboxRPM = DCrpm / 200  // Gear ratio 200:1
   
   c. PILIH mode kontrol:
      
      JIKA (DCMode = TRUE) DAN (PIDMODE = TRUE):
         ├─ Hitung Error:
         │  Error = DCsetpoint - DCGearboxRPM
         │
         ├─ Hitung Proportional:
         │  P = DCkP × Error
         │
         ├─ Hitung Integral (dengan anti-windup):
         │  DCintegralSum += Error × dt
         │  JIKA DCintegralSum > DC_INTEGRAL_MAX:
         │     DCintegralSum = DC_INTEGRAL_MAX
         │  JIKA DCintegralSum < DC_INTEGRAL_MIN:
         │     DCintegralSum = DC_INTEGRAL_MIN
         │  I = DCkI × DCintegralSum
         │
         ├─ Hitung Derivative:
         │  D = DCkD × (Error - DCpreviousError) / dt
         │  DCpreviousError = Error
         │
         ├─ Total Output:
         │  DCsignalPWM = (P + I + D) × 10
         │
         ├─ Saturasi Output (dengan anti-stall):
         │  JIKA DCsignalPWM > 4095:
         │     DCsignalPWM = 4095
         │  JIKA DCsignalPWM < -4095:
         │     DCsignalPWM = -4095
         │  JIKA 0 < DCsignalPWM < DC_PWM_MIN (800):
         │     DCsignalPWM = DC_PWM_MIN  // Anti-stall
         │  JIKA -DC_PWM_MIN < DCsignalPWM < 0:
         │     DCsignalPWM = -DC_PWM_MIN
         │
         └─ Kirim PWM ke motor:
            DCmotorControl(DCDirection, DCsignalPWM)
      
      JIKA (DCMode = TRUE) DAN (PIDMODE = FALSE):
         ├─ Manual control (tanpa PID):
         │  PWM = map(DCsetpoint, 0, 115, 0, 4095)
         └─ DCmotorControl(DCDirection, PWM)
      
      JIKA (DCMode = FALSE):
         └─ Matikan motor:
            DCmotorControl(0, 0)
   
   d. Kirim data via ESP-NOW:
      - Kirim DCGearboxRPM ke Interface Board
      - Kirim timestamp

3. DCnewDataReady = FALSE

SELESAI
```

### 1.3 Algoritma AC Motor Control dengan PID

```
ALGORITMA: AC_Motor_PID_Control
INPUT: ACsetpoint (target RPM), ACMode (ON/OFF), PIDMODE (PID/Manual)
OUTPUT: PWM signal ke motor AC

MULAI
1. Timer Interrupt setiap ACREAD_INTERVAL
   - Set flag ACnewDataReady = TRUE
   - Jika acspeedRequest = FALSE, matikan motor

2. JIKA ACnewDataReady = TRUE
   a. Baca encoder dengan Kalman Filter:
      - Raw measurement dari encoder
      - Kalman Filter untuk smoothing:
        * Predict: estimate = previous estimate
        * error_estimate = previous error_estimate + process_noise
        * Update: kalman_gain = error_estimate / (error_estimate + error_measure)
        * estimate = estimate + kalman_gain × (measurement - estimate)
        * error_estimate = (1 - kalman_gain) × error_estimate
      - ACrpm = filtered estimate
   
   b. PILIH mode kontrol:
      
      JIKA (ACMode = TRUE) DAN (PIDMODE = TRUE):
         ├─ Hitung Error:
         │  ACError = ACsetpoint - ACrpm
         │
         ├─ Hitung Proportional:
         │  P = ACkP × ACError
         │
         ├─ Hitung Integral (dengan anti-windup):
         │  ACintegralSum += ACError × dt
         │  JIKA ACintegralSum > AC_INTEGRAL_MAX:
         │     ACintegralSum = AC_INTEGRAL_MAX
         │  JIKA ACintegralSum < AC_INTEGRAL_MIN:
         │     ACintegralSum = AC_INTEGRAL_MIN
         │  I = ACkI × ACintegralSum
         │
         ├─ Hitung Derivative:
         │  D = ACkD × (ACError - ACpreviousError) / dt
         │  ACpreviousError = ACError
         │
         ├─ Total Output:
         │  ACsignalPWM = P + I + D
         │
         ├─ Saturasi Output (dengan anti-stall):
         │  JIKA ACsignalPWM > 255:
         │     ACsignalPWM = 255
         │  JIKA ACsignalPWM < 0:
         │     ACsignalPWM = 0
         │  JIKA 0 < ACsignalPWM < AC_PWM_MIN (800):
         │     ACsignalPWM = AC_PWM_MIN  // Anti-stall
         │
         └─ Kirim PWM ke motor:
            ACmotorControl(true, ACsignalPWM, true, 0)
      
      JIKA (ACMode = TRUE) DAN (PIDMODE = FALSE):
         ├─ Manual control (tanpa PID):
         │  PWM = map(ACsetpoint, 0, 1500, 0, 255)
         └─ ACmotorControl(true, PWM, true, 0)
      
      JIKA (ACMode = FALSE):
         └─ Matikan motor dan reset PID:
            ACmotorControl(false, 0, true, 0)
            ACintegralSum = 0
            ACpreviousError = 0

3. ACnewDataReady = FALSE

SELESAI
```

### 1.4 Algoritma Komunikasi ESP-NOW (Plant Board)

```
ALGORITMA: ESP-NOW_Communication_Plant
FUNGSI: Terima perintah dari Interface Board

MULAI

1. INIT ESP-NOW:
   - Set WiFi mode = WIFI_STA
   - Init ESP-NOW
   - Register callback: OnDataSent, OnDataReceived
   - Add peer (Interface Board MAC address)

2. CALLBACK OnDataReceived(mac, data, len):
   - Parse message: typeId (4 bytes) + value (4 bytes)
   
   PILIH berdasarkan typeId:
   
   CASE MSG_DC_KP (11):
      ├─ DCkP = value
      └─ Simpan ke EEPROM
   
   CASE MSG_DC_KI (12):
      ├─ DCkI = value
      └─ Simpan ke EEPROM
   
   CASE MSG_DC_KD (13):
      ├─ DCkD = value
      └─ Simpan ke EEPROM
   
   CASE MSG_DC_Setpoint (14):
      └─ DCsetpoint = value
   
   CASE MSG_DC_Control (15):
      └─ DCMode = value (ON/OFF)
   
   CASE MSG_DC_Direction (16):
      └─ DCDirection = value (0/1)
   
   CASE MSG_AC_KP (21):
      ├─ ACkP = value
      └─ Simpan ke EEPROM
   
   CASE MSG_AC_KI (22):
      ├─ ACkI = value
      └─ Simpan ke EEPROM
   
   CASE MSG_AC_KD (23):
      ├─ ACkD = value
      └─ Simpan ke EEPROM
   
   CASE MSG_AC_Setpoint (24):
      └─ ACsetpoint = value
   
   CASE MSG_AC_Control (25):
      └─ ACMode = value (ON/OFF)
   
   CASE MSG_PID_MODE (30):
      └─ PIDMODE = value (PID/Manual)
   
   CASE MSG_SPD_REQUEST (31):
      ├─ Parse value:
      │  dcspeedRequest = (value == 1 atau value == 3)
      │  acspeedRequest = (value == 2 atau value == 3)
      └─ JIKA value == 1 atau 3:
            waktu_awal_motor = millis()
   
   CASE ESP_RESTART (100):
      └─ ESP.restart()

3. CALLBACK OnDataSent(mac, status):
   - Log status pengiriman (success/fail)

4. FUNGSI sendTaggedFloat(typeId, value):
   - Buat paket: [typeId(4B), value(4B)]
   - Kirim via esp_now_send()

SELESAI
```

---

## 2. SISTEM INTERFACE BOARD

### 2.1 Flowchart Utama Interface Board

```
┌─────────────────────┐
│   START (Setup)     │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────────┐
│ Init Serial (115200)    │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Init ESP-NOW            │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Init Rotary Encoder     │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Init TFT Display        │
│ (ST7735)                │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Menu System Init        │
│ (Show Splash Screen)    │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Send ESP_RESTART        │
│ to Plant Board          │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│    MAIN LOOP            │
└──────────┬──────────────┘
           │
           ▼
┌─────────────────────────┐
│ Baca Rotary Encoder     │
│ (Cek perubahan posisi)  │
└──────────┬──────────────┘
           │
           ▼
      ┌────┴────┐
      │ Posisi  │ NO
      │Berubah? ├──────┐
      └────┬────┘      │
          YES          │
           │           │
           ▼           │
┌─────────────────────┐│
│ menuNavigate()      ││
│ (Scroll menu)       ││
└─────────┬───────────┘│
          │            │
          └────────────┤
                       │
           ┌───────────┘
           │
           ▼
┌─────────────────────────┐
│ Baca Button Click       │
└──────────┬──────────────┘
           │
      ┌────┴────────────┐
      │                 │
      ▼                 ▼
┌──────────┐      ┌──────────┐
│ Single   │      │ Double   │
│ Click?   │      │ Click?   │
└────┬─────┘      └────┬─────┘
     │YES              │YES
     ▼                 ▼
┌──────────┐      ┌──────────────┐
│menuSelect│      │menuDoubleClick│
│(Enter)   │      │(Toggle Run/  │
└────┬─────┘      │Encoder Mode) │
     │            └────┬─────────┘
     │                 │
     └────────┬────────┘
              │
              ▼
         ┌─────────┐
         │  Long   │
         │ Click?  │
         └────┬────┘
             YES
              │
              ▼
         ┌─────────┐
         │menuBack │
         │(Kembali)│
         └────┬────┘
              │
              └────────┐
                       │
           ┌───────────┘
           │
           ▼
┌─────────────────────────┐
│ menuUpdate()            │
│ (Update display)        │
└──────────┬──────────────┘
           │
           ▼
       (Loop Back)
```

### 2.2 Algoritma Menu Navigation System

```
ALGORITMA: Menu_Navigation_System
TUJUAN: Navigasi dan kontrol menu pada TFT display

MULAI

STRUKTUR MENU:
Menu Level 0 (Root):
├─ [0] Escalator (DC Motor)
├─ [1] Motor AC
└─ [2] WiFi Status

Menu Level 1 (Escalator):
├─ [0] Graph View (Real-time monitoring)
└─ [1] Control View (Parameter setting)

Menu Level 1 (Motor AC):
├─ [0] Graph View (Real-time monitoring)
└─ [1] Control View (Parameter setting)


1. FUNGSI menuNavigate(direction):
   ├─ JIKA g_currentMenu == 0 (Root Menu):
   │  ├─ Update g_selectedIndex (0-2)
   │  └─ Gambar ulang menu
   │
   ├─ JIKA g_currentMenu == 1 (Escalator):
   │  ├─ JIKA g_escSubMenu == 0 (Graph):
   │  │  └─ (Tidak ada navigasi di graph)
   │  │
   │  └─ JIKA g_escSubMenu == 1 (Control):
   │     ├─ Update g_selectedIndex (0-5):
   │     │  [0] Kp, [1] Ki, [2] Kd
   │     │  [3] Setpoint, [4] Run/Stop, [5] Direction
   │     └─ Gambar ulang menu
   │
   └─ JIKA g_currentMenu == 2 (Motor AC):
      ├─ JIKA g_motorSubMenu == 0 (Graph):
      │  └─ (Tidak ada navigasi di graph)
      │
      └─ JIKA g_motorSubMenu == 1 (Control):
         ├─ Update g_selectedIndex (0-4):
         │  [0] Kp, [1] Ki, [2] Kd
         │  [3] Setpoint, [4] Run/Stop
         └─ Gambar ulang menu


2. FUNGSI menuSelect():
   ├─ JIKA g_currentMenu == 0 (Root Menu):
   │  ├─ JIKA g_selectedIndex == 0:
   │  │  ├─ g_currentMenu = 1 (Escalator)
   │  │  ├─ g_escSubMenu = 0 (Graph)
   │  │  └─ Minta speed request ke Plant Board
   │  │
   │  ├─ JIKA g_selectedIndex == 1:
   │  │  ├─ g_currentMenu = 2 (Motor AC)
   │  │  ├─ g_motorSubMenu = 0 (Graph)
   │  │  └─ Minta speed request ke Plant Board
   │  │
   │  └─ JIKA g_selectedIndex == 2:
   │     ├─ g_currentMenu = 3 (WiFi Status)
   │     └─ Tampilkan status koneksi
   │
   ├─ JIKA g_currentMenu == 1 (Escalator):
   │  ├─ JIKA g_escSubMenu == 0 (Graph):
   │  │  └─ g_escSubMenu = 1 (Switch ke Control)
   │  │
   │  └─ JIKA g_escSubMenu == 1 (Control):
   │     ├─ JIKA g_isEditing == FALSE:
   │     │  └─ g_isEditing = TRUE (Mulai edit parameter)
   │     │
   │     └─ JIKA g_isEditing == TRUE:
   │        ├─ Simpan nilai parameter
   │        ├─ Kirim via ESP-NOW ke Plant Board:
   │        │  • sendTaggedFloat(MSG_DC_KP, g_dcKp)
   │        │  • sendTaggedFloat(MSG_DC_KI, g_dcKi)
   │        │  • sendTaggedFloat(MSG_DC_KD, g_dcKd)
   │        │  • sendTaggedFloat(MSG_DC_Setpoint, g_dcSetpoint)
   │        │  • sendTaggedFloat(MSG_DC_Control, g_escRunning)
   │        │  • sendTaggedFloat(MSG_DC_Direction, g_escDirection)
   │        └─ g_isEditing = FALSE
   │
   └─ JIKA g_currentMenu == 2 (Motor AC):
      ├─ JIKA g_motorSubMenu == 0 (Graph):
      │  └─ g_motorSubMenu = 1 (Switch ke Control)
      │
      └─ JIKA g_motorSubMenu == 1 (Control):
         ├─ JIKA g_isEditing == FALSE:
         │  └─ g_isEditing = TRUE (Mulai edit parameter)
         │
         └─ JIKA g_isEditing == TRUE:
            ├─ Simpan nilai parameter
            ├─ Kirim via ESP-NOW ke Plant Board:
            │  • sendTaggedFloat(MSG_AC_KP, g_acKp)
            │  • sendTaggedFloat(MSG_AC_KI, g_acKi)
            │  • sendTaggedFloat(MSG_AC_KD, g_acKd)
            │  • sendTaggedFloat(MSG_AC_Setpoint, g_acSetpoint)
            │  • sendTaggedFloat(MSG_AC_Control, g_motorRunning)
            └─ g_isEditing = FALSE


3. FUNGSI menuBack():
   ├─ JIKA g_isEditing == TRUE:
   │  └─ g_isEditing = FALSE (Cancel edit)
   │
   ├─ JIKA g_escSubMenu == 1 ATAU g_motorSubMenu == 1:
   │  ├─ Reset ke submenu 0 (Graph)
   │  └─ Stop speed request
   │
   └─ JIKA g_currentMenu > 0:
      ├─ g_currentMenu = 0 (Kembali ke Root)
      └─ g_selectedIndex = 0


4. FUNGSI menuDoubleClick():
   ├─ JIKA di Graph View:
   │  └─ Toggle motor Run/Stop:
   │     ├─ g_escRunning = !g_escRunning
   │     └─ Kirim MSG_DC_Control atau MSG_AC_Control
   │
   └─ JIKA di mode lain:
      └─ Toggle encoder direction (UP/DOWN)


5. FUNGSI menuUpdate():
   ├─ Cek flag g_escSpeedUpdated:
   │  └─ JIKA TRUE:
   │     ├─ Update graph dengan data baru
   │     ├─ Redraw incremental (oscilloscope style)
   │     └─ g_escSpeedUpdated = FALSE
   │
   ├─ Cek flag g_motorSpeedUpdated:
   │  └─ JIKA TRUE:
   │     ├─ Update graph dengan data baru
   │     ├─ Redraw incremental (oscilloscope style)
   │     └─ g_motorSpeedUpdated = FALSE
   │
   └─ Update status indicators (connection, run state)

SELESAI
```

### 2.3 Algoritma Real-time Graph Display

```
ALGORITMA: Real_Time_Graph_Display
TUJUAN: Menampilkan grafik real-time dengan gaya oscilloscope

MULAI

KONSTANTA:
- innerWidth = 116 pixels
- maxSamples = 100 data points
- ESC_GRAPH_MAX = 115.0 (DC Motor)
- AC_GRAPH_MAX = 1500.0 (AC Motor)

1. FUNGSI drawEscGraph(fullRedraw):
   ├─ JIKA fullRedraw ATAU !escGraphInit:
   │  ├─ Clear graph area
   │  ├─ Draw border (5, 25, 118, 70)
   │  ├─ Draw setpoint line (red, dashed):
   │  │  setpointY = 94 - (g_dcSetpoint × 69 / ESC_GRAPH_MAX)
   │  │  Draw horizontal line at setpointY
   │  │
   │  ├─ Draw time grid (vertical lines per 1 second)
   │  │
   │  ├─ Plot all historical data:
   │  │  UNTUK col = 0 sampai innerWidth:
   │  │     ├─ Interpolate data index dari history buffer
   │  │     ├─ idx = (g_escHistoryIndex - 1 - offset) % maxSamples
   │  │     ├─ value = g_escSpeedHistory[idx]
   │  │     ├─ y = 94 - (value × 69 / ESC_GRAPH_MAX)
   │  │     ├─ Clamp y ke range [25, 94]
   │  │     ├─ Draw pixel(col + leftX, y, YELLOW)
   │  │     └─ escGraphY[col] = y
   │  │
   │  └─ escGraphInit = TRUE
   │
   └─ JIKA incremental update:
      ├─ Shift graph 1 pixel ke kiri:
      │  UNTUK col = 0 sampai innerWidth-2:
      │     ├─ oldY = escGraphY[col]
      │     ├─ newY = escGraphY[col+1]
      │     ├─ Erase pixel(col + leftX, oldY, BLACK)
      │     ├─ Draw pixel(col + leftX, newY, YELLOW)
      │     └─ escGraphY[col] = newY
      │
      └─ Draw newest data di rightmost column:
         ├─ value = g_escSpeedHistory[g_escHistoryIndex-1]
         ├─ y = 94 - (value × 69 / ESC_GRAPH_MAX)
         ├─ Clamp y
         ├─ Draw pixel(rightX, y, YELLOW)
         └─ escGraphY[innerWidth-1] = y


2. FUNGSI updateEscalatorSpeed(speed):
   ├─ Simpan ke circular buffer:
   │  g_escSpeedHistory[g_escHistoryIndex] = speed
   │  g_escHistoryIndex = (g_escHistoryIndex + 1) % maxSamples
   │
   ├─ Update current speed display:
   │  g_escSpeed = speed
   │
   └─ Set flag untuk update graph:
      g_escSpeedUpdated = TRUE


3. FUNGSI drawMotorGraph(fullRedraw):
   ├─ Sama seperti drawEscGraph() tetapi dengan:
   │  ├─ AC_GRAPH_MAX = 1500.0
   │  ├─ Data dari g_motorSpeedHistory[]
   │  └─ Setpoint dari g_acSetpoint
   │
   └─ (Implementasi identik dengan penyesuaian variabel)


4. FUNGSI updateMotorSpeed(speed):
   ├─ Simpan ke circular buffer:
   │  g_motorSpeedHistory[g_motorHistoryIndex] = speed
   │  g_motorHistoryIndex = (g_motorHistoryIndex + 1) % maxSamples
   │
   ├─ Update current speed display:
   │  g_motorSpeed = speed
   │
   └─ Set flag untuk update graph:
      g_motorSpeedUpdated = TRUE

SELESAI
```

### 2.4 Algoritma Encoder Input Processing

```
ALGORITMA: Rotary_Encoder_Input
TUJUAN: Membaca rotary encoder untuk navigasi menu

MULAI

VARIABEL:
- lastEncoder = posisi encoder sebelumnya
- encoderMode = ENCODER_UP atau ENCODER_DOWN
- buttonState = status tombol encoder

1. FUNGSI DCgetEncoderCount():
   ├─ Baca posisi encoder saat ini
   └─ Return: long encoder count

2. FUNGSI getEncoderMode():
   ├─ Return: ENCODER_UP atau ENCODER_DOWN
   └─ (Menentukan arah increment/decrement)

3. FUNGSI toggleEncoderMode():
   ├─ JIKA encoderMode == ENCODER_UP:
   │  └─ encoderMode = ENCODER_DOWN
   │
   └─ JIKA encoderMode == ENCODER_DOWN:
      └─ encoderMode = ENCODER_UP

4. FUNGSI getButtonClick():
   ├─ Baca state tombol encoder
   │
   ├─ Deteksi Single Click:
   │  ├─ Button pressed lalu released dalam < 500ms
   │  └─ Return: CLICK_SINGLE
   │
   ├─ Deteksi Double Click:
   │  ├─ 2x click dalam < 300ms
   │  └─ Return: CLICK_DOUBLE
   │
   ├─ Deteksi Long Press:
   │  ├─ Button held > 1000ms
   │  └─ Return: CLICK_LONG
   │
   └─ JIKA tidak ada event:
      └─ Return: CLICK_NONE

5. MAIN LOOP Processing:
   ├─ currentEncoder = DCgetEncoderCount()
   │
   ├─ JIKA currentEncoder != lastEncoder:
   │  ├─ direction = (currentEncoder > lastEncoder) ? 1 : -1
   │  ├─ JIKA g_isEditing == TRUE:
   │  │  ├─ Update parameter value:
   │  │  │  JIKA encoderMode == ENCODER_UP:
   │  │  │     value += increment × direction
   │  │  │  JIKA encoderMode == ENCODER_DOWN:
   │  │  │     value -= increment × direction
   │  │  └─ Redraw parameter display
   │  │
   │  └─ JIKA g_isEditing == FALSE:
   │     └─ menuNavigate(direction)
   │
   └─ lastEncoder = currentEncoder

SELESAI
```

### 2.5 Algoritma Komunikasi ESP-NOW (Interface Board)

```
ALGORITMA: ESP-NOW_Communication_Interface
FUNGSI: Terima data dari Plant Board dan kirim perintah kontrol

MULAI

1. INIT ESP-NOW:
   - Set WiFi mode = WIFI_STA
   - Init ESP-NOW
   - Register callback: OnDataSent, OnDataReceived
   - Add peer (Plant Board MAC address)

2. CALLBACK receiveData(mac, data, len):
   - Parse message: typeId (4 bytes) + value (4 bytes)
   
   PILIH berdasarkan typeId:
   
   CASE MSG_DC_SPEED (10):
      ├─ DCspeed = value
      ├─ updateEscalatorSpeed(value)
      ├─ Update g_escConnected = TRUE
      └─ Set flag g_escSpeedUpdated = TRUE
   
   CASE MSG_DC_KP (11):
      └─ g_dcKp = value (update tampilan)
   
   CASE MSG_DC_KI (12):
      └─ g_dcKi = value
   
   CASE MSG_DC_KD (13):
      └─ g_dcKd = value
   
   CASE MSG_DC_Setpoint (14):
      └─ g_dcSetpoint = value
   
   CASE MSG_AC_SPEED (20):
      ├─ ACspeed = value
      ├─ updateMotorSpeed(value)
      ├─ Update g_motorConnected = TRUE
      └─ Set flag g_motorSpeedUpdated = TRUE
   
   CASE MSG_AC_KP (21):
      └─ g_acKp = value
   
   CASE MSG_AC_KI (22):
      └─ g_acKi = value
   
   CASE MSG_AC_KD (23):
      └─ g_acKd = value
   
   CASE MSG_AC_Setpoint (24):
      └─ g_acSetpoint = value
   
   CASE MSG_TIMESTAMP (32):
      ├─ timestamp = value
      └─ addTimestamp(timestamp)  // Simpan ke buffer untuk graph
   
   DEFAULT:
      └─ Log unknown message type

3. FUNGSI sendTaggedFloat(typeId, value):
   ├─ Buat paket: [typeId(4B), value(4B)]
   ├─ esp_now_send(PLANT_MAC, paket, 8)
   └─ Log pengiriman

4. FUNGSI onDataSent(mac, status):
   ├─ JIKA status == ESP_NOW_SEND_SUCCESS:
   │  └─ Log sukses
   │
   └─ JIKA status == FAIL:
      ├─ Log error
      └─ Update g_espnowInit = FALSE

5. TIMING & BUFFER MANAGEMENT:
   ├─ addTimestamp(ts):
   │  ├─ timestamp_buffer[timestamp_idx] = ts
   │  ├─ timestamp_idx = (timestamp_idx + 1) % TIMESTAMP_BUF
   │  └─ (Circular buffer untuk 100 timestamp)
   │
   └─ getTimestamp(index):
      ├─ Ambil timestamp dari buffer
      └─ Return: timestamp atau -1 jika tidak ada data

SELESAI
```

---

## 3. DIAGRAM INTERAKSI SISTEM

```
┌─────────────────────────────────────────────────────────────┐
│                    INTERFACE BOARD                          │
│  ┌────────────┐  ┌──────────────┐  ┌──────────────────┐   │
│  │  Rotary    │  │  TFT Display │  │   ESP-NOW TX     │   │
│  │  Encoder   │  │  (ST7735)    │  │   (WiFi)         │   │
│  │  + Button  │  │              │  │                  │   │
│  └─────┬──────┘  └──────┬───────┘  └────────┬─────────┘   │
│        │                │                    │              │
│        ▼                ▼                    ▼              │
│  ┌───────────────────────────────────────────────────┐     │
│  │          Control & Menu System                    │     │
│  │  • Menu Navigation                                │     │
│  │  • Parameter Editing                              │     │
│  │  • Real-time Graph Display                        │     │
│  │  • Button Click Handler                           │     │
│  └───────────────────────┬───────────────────────────┘     │
└────────────────────────────┼─────────────────────────────────┘
                             │ ESP-NOW
                             │ Communication
                             │
          ┌──────────────────┼──────────────────┐
          │                  │                  │
          ▼                  ▼                  ▼
    ┌─────────────┐   ┌─────────────┐   ┌─────────────┐
    │  Setpoint   │   │  PID Param  │   │  Control    │
    │  Commands   │   │  Update     │   │  ON/OFF     │
    └─────────────┘   └─────────────┘   └─────────────┘
          │                  │                  │
          └──────────────────┼──────────────────┘
                             │ ESP-NOW
                             │ Communication
                             ▼
┌─────────────────────────────────────────────────────────────┐
│                      PLANT BOARD                            │
│  ┌───────────────────────────────────────────────────┐     │
│  │          ESP-NOW RX & Data Processing             │     │
│  │  • Receive Commands                               │     │
│  │  • Update PID Parameters                          │     │
│  │  • Store to EEPROM                                │     │
│  └───────────────────────┬───────────────────────────┘     │
│                          │                                  │
│        ┌─────────────────┴─────────────────┐               │
│        │                                   │               │
│        ▼                                   ▼               │
│  ┌──────────────────┐            ┌──────────────────┐     │
│  │  DC Motor PID    │            │  AC Motor PID    │     │
│  │  Controller      │            │  Controller      │     │
│  │  • Read Encoder  │            │  • Read Encoder  │     │
│  │  • Calculate PID │            │  • Kalman Filter │     │
│  │  • PWM Output    │            │  • Calculate PID │     │
│  │                  │            │  • PWM Output    │     │
│  └────────┬─────────┘            └────────┬─────────┘     │
│           │                               │               │
│           ▼                               ▼               │
│  ┌──────────────────┐            ┌──────────────────┐     │
│  │   DC Motor       │            │   AC Motor       │     │
│  │   (Escalator)    │            │   (Conveyor)     │     │
│  │   + Encoder      │            │   + Encoder      │     │
│  └──────────────────┘            └──────────────────┘     │
│           │                               │               │
│           └───────────┬───────────────────┘               │
│                       │ Speed Data                        │
│                       │ (Real-time)                       │
│                       ▼                                   │
│  ┌───────────────────────────────────────────────────┐   │
│  │          ESP-NOW TX (Feedback)                    │   │
│  │  • Send DC RPM                                    │   │
│  │  • Send AC RPM                                    │   │
│  │  • Send Timestamp                                 │   │
│  └───────────────────────┬───────────────────────────┘   │
└────────────────────────────┼─────────────────────────────┘
                             │ ESP-NOW
                             │ Communication
                             ▼
┌─────────────────────────────────────────────────────────────┐
│                    INTERFACE BOARD                          │
│  ┌───────────────────────────────────────────────────┐     │
│  │          ESP-NOW RX (Data Reception)              │     │
│  │  • Receive Speed Data                             │     │
│  │  • Update Graph Buffer                            │     │
│  │  • Set Update Flags                               │     │
│  └───────────────────────┬───────────────────────────┘     │
│                          │                                  │
│                          ▼                                  │
│  ┌───────────────────────────────────────────────────┐     │
│  │       Real-time Graph Display Update              │     │
│  │  • Incremental Pixel Redraw                       │     │
│  │  • Oscilloscope Style                             │     │
│  │  • Setpoint Line Overlay                          │     │
│  └───────────────────────────────────────────────────┘     │
└─────────────────────────────────────────────────────────────┘
```

---

## 4. TIMING DIAGRAM

```
Time (ms):  0      50     100    150    200    250    300
            │      │      │      │      │      │      │
Plant DC:   ├──────┼──────┼──────┼──────┼──────┼──────┤
Encoder     │ Read │ Read │ Read │ Read │ Read │ Read │
Timer       │ PID  │ PID  │ PID  │ PID  │ PID  │ PID  │
(50ms)      │ TX   │ TX   │ TX   │ TX   │ TX   │ TX   │
            │      │      │      │      │      │      │
Plant AC:   ├──────┼──────┼──────┼──────┼──────┼──────┤
Encoder     │ Read │ Read │ Read │ Read │ Read │ Read │
Timer       │ PID  │ PID  │ PID  │ PID  │ PID  │ PID  │
(varies)    │ TX   │ TX   │ TX   │ TX   │ TX   │ TX   │
            │      │      │      │      │      │      │
Interface:  ├──────┼──────┼──────┼──────┼──────┼──────┤
Main Loop   │ RX   │ RX   │ RX   │ RX   │ RX   │ RX   │
(~10ms)     │Graph │Graph │Graph │Graph │Graph │Graph │
            │Input │Input │Input │Input │Input │Input │
            │      │      │      │      │      │      │
User Input: │      │      │Encoder       │      │Button│
Events      │      │      │Turn          │      │Click │
            └──────┴──────┴──────┴──────┴──────┴──────┘
```

---

## 5. STATE MACHINE DIAGRAM (Menu System)

```
                    ┌──────────────┐
                    │  INIT/SPLASH │
                    └──────┬───────┘
                           │
                           ▼
              ┌────────────────────────┐
              │     ROOT MENU          │◄──────────────┐
              │  [0] Escalator         │               │
              │  [1] Motor AC          │               │
              │  [2] WiFi Status       │               │
              └─────┬──────────┬───────┘               │
                    │          │                       │
         Select [0] │          │ Select [1]        Long Click
                    │          │                   (Back)
                    ▼          ▼                       │
      ┌──────────────────┐  ┌──────────────────┐     │
      │  ESCALATOR MENU  │  │  MOTOR AC MENU   │     │
      │  ├─[0] Graph     │  │  ├─[0] Graph     │     │
      │  └─[1] Control   │  │  └─[1] Control   │     │
      └─────┬────────┬───┘  └─────┬────────┬───┘     │
            │        │            │        │         │
    Select  │        │ Select     │        │ Select   │
    [0]     │        │ [1]        │        │          │
            │        │            │        │          │
            ▼        ▼            ▼        ▼          │
    ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ │
    │   ESC   │ │   ESC   │ │   AC    │ │   AC    │ │
    │  GRAPH  │ │ CONTROL │ │  GRAPH  │ │ CONTROL │ │
    │         │ │         │ │         │ │         │ │
    │Real-time│ │Edit Kp, │ │Real-time│ │Edit Kp, │ │
    │Plotting │ │Ki, Kd,  │ │Plotting │ │Ki, Kd,  │ │
    │         │ │Setpoint,│ │         │ │Setpoint,│ │
    │         │ │Run/Stop │ │         │ │Run/Stop │ │
    └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ │
         │           │           │           │       │
         │           │           │           │       │
         └───────────┴───────────┴───────────┴───────┘
                   Long Click (Back to Root)
```

---

## 6. DATA FLOW SUMMARY

### 6.1 Command Flow (Interface → Plant)
```
User Input
   ↓
Encoder/Button Detection
   ↓
Menu System Processing
   ↓
Parameter Update/Control Command
   ↓
sendTaggedFloat(typeId, value)
   ↓
ESP-NOW Transmission
   ↓
Plant Board Reception
   ↓
Parameter Update / Motor Control
   ↓
EEPROM Storage (PID parameters)
```

### 6.2 Feedback Flow (Plant → Interface)
```
Motor Running
   ↓
Encoder Interrupt (CHANGE edge)
   ↓
Timer Interrupt (50ms for DC, varies for AC)
   ↓
Pulse Count & RPM Calculation
   ↓
PID Calculation (if enabled)
   ↓
PWM Output to Motor
   ↓
sendTaggedFloat(MSG_DC_SPEED/MSG_AC_SPEED, rpm)
   ↓
ESP-NOW Transmission
   ↓
Interface Board Reception
   ↓
Update Speed History Buffer
   ↓
Set Update Flag (g_escSpeedUpdated / g_motorSpeedUpdated)
   ↓
menuUpdate() in Main Loop
   ↓
Incremental Graph Redraw
   ↓
Display Updated on TFT
```

---

## 7. KRITERIA DAN FITUR KHUSUS

### 7.1 Anti-Windup PID
- Integral term dibatasi dengan `DC_INTEGRAL_MAX/MIN` dan `AC_INTEGRAL_MAX/MIN`
- Mencegah integral overflow saat setpoint tidak tercapai
- Reset integral sum saat motor dimatikan

### 7.2 Anti-Stall Protection
- PWM minimum: DC = 800, AC = 800
- Jika output PID < minimum threshold → naikkan ke minimum
- Mencegah motor stall di output rendah

### 7.3 Kalman Filter (AC Motor)
- Smoothing pada RPM measurement
- Parameter: process_noise = 0.01, measure_noise = 3.0
- Mengurangi noise pada pembacaan encoder

### 7.4 Circular Buffer (Graph Display)
- Buffer size: 100 samples
- Oscilloscope-style incremental redraw
- Efficient memory usage

### 7.5 Persistent Storage
- PID parameters disimpan di EEPROM
- Auto-load saat restart
- Ensure parameter consistency

---

## 8. MESSAGE PROTOCOL

### 8.1 Message Format
```
┌──────────────────────┬──────────────────────┐
│  Type ID (4 bytes)   │  Value (4 bytes)     │
│  (int32_t)           │  (float)             │
└──────────────────────┴──────────────────────┘
Total: 8 bytes per message
```

### 8.2 Message Type IDs

#### DC Motor Messages (10-16):
- `10`: MSG_DC_SPEED (Plant → Interface)
- `11`: MSG_DC_KP (Interface → Plant)
- `12`: MSG_DC_KI (Interface → Plant)
- `13`: MSG_DC_KD (Interface → Plant)
- `14`: MSG_DC_Setpoint (Interface → Plant)
- `15`: MSG_DC_Control (Interface → Plant, ON/OFF)
- `16`: MSG_DC_Direction (Interface → Plant, 0/1)

#### AC Motor Messages (20-27):
- `20`: MSG_AC_SPEED (Plant → Interface)
- `21`: MSG_AC_KP (Interface → Plant)
- `22`: MSG_AC_KI (Interface → Plant)
- `23`: MSG_AC_KD (Interface → Plant)
- `24`: MSG_AC_Setpoint (Interface → Plant)
- `25`: MSG_AC_Control (Interface → Plant, ON/OFF)
- `26`: MSG_AC_Voltage (Interface → Plant)
- `27`: MSG_AC_Direction (Interface → Plant)

#### System Messages (30-100):
- `30`: MSG_PID_MODE (Interface → Plant, PID/Manual)
- `31`: MSG_SPD_REQUEST (Interface → Plant, 1=DC, 2=AC, 3=Both)
- `32`: MSG_TIMESTAMP (Plant → Interface)
- `100`: ESP_RESTART (Interface → Plant, Restart command)

---

## KESIMPULAN

Sistem ini mengimplementasikan **dual motor control** (DC dan AC) dengan:

1. **Real-time PID control** dengan anti-windup dan anti-stall
2. **Wireless communication** via ESP-NOW
3. **Interactive menu system** dengan TFT display
4. **Real-time graphing** dengan oscilloscope-style rendering
5. **Persistent parameter storage** di EEPROM
6. **Kalman filtering** untuk noise reduction (AC motor)
7. **Modular architecture** dengan separation of concerns

Kedua board bekerja secara **asynchronous** dengan komunikasi **bidirectional** untuk kontrol dan monitoring real-time.
