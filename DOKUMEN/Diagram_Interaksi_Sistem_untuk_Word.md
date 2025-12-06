# DIAGRAM INTERAKSI SISTEM - FORMAT UNTUK MS WORD

## Petunjuk Pembuatan di MS Word:
1. Gunakan **SmartArt** → **Hierarchy** atau **Process**
2. Gunakan **Shapes** → **Rectangle** dan **Arrow**
3. Gunakan **Text Box** dengan bullets/numbering
4. Gunakan **Table** untuk mempermudah alignment

---

## 1. ALUR LENGKAP SISTEM (Top-Down)

### Level 1: USER INTERFACE
```
┌─────────────────────────────────────────────────────────┐
│               INTERFACE BOARD (ESP8266)                 │
│                     (User Side)                         │
└─────────────────────────────────────────────────────────┘
```

**Komponen:**
- Rotary Encoder + Button
- TFT Display (ST7735, 128x160)
- ESP-NOW TX/RX Module
- Control & Menu System

**Fungsi:**
- Input dari user (navigasi menu, edit parameter)
- Display real-time graph dan status
- Kirim perintah kontrol via ESP-NOW
- Terima data feedback dari Plant Board

---

### Level 2: WIRELESS COMMUNICATION
```
┌─────────────────────────────────────────────────────────┐
│                   ESP-NOW PROTOCOL                      │
│              (Wireless Communication)                   │
└─────────────────────────────────────────────────────────┘
```

**Data yang Dikirim Interface → Plant:**
- PID Parameters (Kp, Ki, Kd)
- Setpoint (target RPM)
- Control Commands (ON/OFF, Direction)
- Mode Selection (PID/Manual)
- Speed Request Flags

**Data yang Dikirim Plant → Interface:**
- Current Speed (DC RPM, AC RPM)
- Timestamp
- Connection Status

---

### Level 3: PLANT CONTROL
```
┌─────────────────────────────────────────────────────────┐
│                PLANT BOARD (ESP32)                      │
│                  (Control Side)                         │
└─────────────────────────────────────────────────────────┘
```

**Komponen:**
- ESP-NOW RX/TX Module
- DC Motor Controller + Encoder
- AC Motor Controller + Encoder
- Data Storage (EEPROM)
- LED Status Indicators

**Fungsi:**
- Terima perintah dari Interface Board
- Kontrol motor dengan PID
- Baca sensor encoder
- Simpan parameter ke EEPROM
- Kirim feedback real-time

---

## 2. ALUR DETAIL PER SUBSISTEM

### A. ALUR INPUT USER (Interface Board)

**Dropdown Level 1: User Input**
└── **Rotary Encoder**
    ├── Deteksi putaran (scroll menu)
    ├── Deteksi tombol (single/double/long click)
    └── Mode UP/DOWN (increment/decrement)

└── **Button Click Processing**
    ├── Single Click → SELECT (enter menu/confirm)
    ├── Double Click → Toggle (Run/Stop motor atau Encoder mode)
    └── Long Click → BACK (kembali ke menu sebelumnya)

**Dropdown Level 2: Menu Navigation**
└── **Root Menu** (Level 0)
    ├── [0] Escalator (DC Motor)
    ├── [1] Motor AC
    └── [2] WiFi Status

└── **Escalator Submenu** (Level 1)
    ├── [0] Graph View → Real-time monitoring
    └── [1] Control View → Edit parameters
        ├── Kp (Proportional gain)
        ├── Ki (Integral gain)
        ├── Kd (Derivative gain)
        ├── Setpoint (target RPM)
        ├── Run/Stop
        └── Direction (Forward/Reverse)

└── **Motor AC Submenu** (Level 1)
    ├── [0] Graph View → Real-time monitoring
    └── [1] Control View → Edit parameters
        ├── Kp (Proportional gain)
        ├── Ki (Integral gain)
        ├── Kd (Derivative gain)
        ├── Setpoint (target RPM)
        └── Run/Stop

**Dropdown Level 3: Parameter Editing**
└── **Edit Mode**
    ├── **Step 1:** Select parameter (single click)
    ├── **Step 2:** Enter edit mode (single click lagi)
    ├── **Step 3:** Adjust value (putar encoder)
    │   ├── UP Mode → Increment value
    │   └── DOWN Mode → Decrement value
    ├── **Step 4:** Confirm (single click)
    └── **Step 5:** Send to Plant Board via ESP-NOW

---

### B. ALUR DISPLAY OUTPUT (Interface Board)

**Dropdown Level 1: Display System**
└── **TFT Display (ST7735)**
    ├── **Header Section** (0-18px)
    │   └── Menu title
    ├── **Content Section** (18-150px)
    │   ├── Menu items (Root menu)
    │   ├── Real-time graph (Graph view)
    │   └── Parameter list (Control view)
    └── **Footer Section** (150-160px)
        └── Hints (1x:SEL 2x:DIR Lx:BACK)

**Dropdown Level 2: Graph Display**
└── **Real-time Graph (Oscilloscope Style)**
    ├── **Graph Area:** 118x70 pixels
    ├── **Buffer:** 100 data points (circular buffer)
    ├── **Update Method:** Incremental pixel redraw
    ├── **Elements:**
    │   ├── Border (white)
    │   ├── Setpoint line (red, dashed)
    │   ├── Time grid (cyan, per 1 second)
    │   └── Speed plot (yellow, real-time)
    └── **Scaling:**
        ├── DC Motor: 0-115 RPM
        └── AC Motor: 0-1500 RPM

**Dropdown Level 3: Graph Update Process**
└── **Update Loop**
    ├── **Step 1:** Terima data baru dari Plant Board
    ├── **Step 2:** Simpan ke circular buffer
    │   └── history[index] = new_value
    ├── **Step 3:** Set update flag
    │   └── g_escSpeedUpdated = TRUE
    ├── **Step 4:** menuUpdate() deteksi flag
    ├── **Step 5:** Shift graph 1 pixel ke kiri
    ├── **Step 6:** Hapus pixel lama (black)
    ├── **Step 7:** Gambar pixel baru (yellow)
    └── **Step 8:** Clear flag

---

### C. ALUR KOMUNIKASI ESP-NOW

**Dropdown Level 1: ESP-NOW Transmission**
└── **Interface Board → Plant Board**
    ├── **Initialization**
    │   ├── Set WiFi mode = WIFI_STA
    │   ├── Init ESP-NOW
    │   ├── Register callbacks
    │   └── Add peer (Plant MAC address)
    ├── **Send Command**
    │   ├── Create packet: [TypeID (4B) + Value (4B)]
    │   ├── Call: esp_now_send(mac, data, 8)
    │   └── Wait callback: OnDataSent()
    └── **Message Types (Interface→Plant)**
        ├── MSG_DC_KP (11) → DC motor Kp
        ├── MSG_DC_KI (12) → DC motor Ki
        ├── MSG_DC_KD (13) → DC motor Kd
        ├── MSG_DC_Setpoint (14) → DC target RPM
        ├── MSG_DC_Control (15) → DC ON/OFF
        ├── MSG_DC_Direction (16) → DC direction
        ├── MSG_AC_KP (21) → AC motor Kp
        ├── MSG_AC_KI (22) → AC motor Ki
        ├── MSG_AC_KD (23) → AC motor Kd
        ├── MSG_AC_Setpoint (24) → AC target RPM
        ├── MSG_AC_Control (25) → AC ON/OFF
        ├── MSG_PID_MODE (30) → PID/Manual mode
        ├── MSG_SPD_REQUEST (31) → Request speed data
        └── ESP_RESTART (100) → Restart Plant Board

**Dropdown Level 2: ESP-NOW Reception**
└── **Plant Board → Interface Board**
    ├── **Receive Callback**
    │   ├── OnDataReceived(mac, data, len)
    │   ├── Parse: typeId + value
    │   └── Execute action based on typeId
    └── **Message Types (Plant→Interface)**
        ├── MSG_DC_SPEED (10) → DC current RPM
        ├── MSG_AC_SPEED (20) → AC current RPM
        └── MSG_TIMESTAMP (32) → Current timestamp

**Dropdown Level 3: Data Processing**
└── **Plant Board Processing**
    ├── **Receive Command**
    │   ├── Callback: receiveData()
    │   ├── Parse message type
    │   └── Update internal variable
    ├── **Store to EEPROM (PID parameters only)**
    │   ├── Check if PID parameter (Kp/Ki/Kd)
    │   ├── Write to EEPROM address
    │   └── Commit changes
    └── **Send Feedback**
        ├── Timer interrupt trigger (50ms)
        ├── Calculate current RPM
        ├── Create feedback packet
        └── Send via ESP-NOW to Interface

---

### D. ALUR KONTROL MOTOR DC (Plant Board)

**Dropdown Level 1: DC Motor Control Loop**
└── **Main Control Flow**
    ├── **Step 1: Timer Interrupt** (50ms interval)
    │   └── Set flag: DCnewDataReady = TRUE
    ├── **Step 2: Check Speed Request**
    │   ├── IF dcspeedRequest = TRUE → Lanjut proses
    │   └── IF dcspeedRequest = FALSE → Motor OFF
    ├── **Step 3: Read Encoder**
    │   ├── DCpulseCount = DCencoder - DClastEncoder
    │   ├── DClastEncoder = DCencoder
    │   └── Calculate RPM = (pulseCount × 60000) / (interval × PPR)
    ├── **Step 4: Gear Ratio Conversion**
    │   └── DCGearboxRPM = DCrpm / 200
    ├── **Step 5: Mode Selection**
    │   ├── **IF PID Mode:**
    │   │   ├── Calculate PID (see dropdown below)
    │   │   └── Send PWM to motor
    │   ├── **IF Manual Mode:**
    │   │   ├── PWM = map(setpoint, 0, 115, 0, 4095)
    │   │   └── Send PWM to motor
    │   └── **IF Motor OFF:**
    │       └── PWM = 0
    └── **Step 6: Send Feedback**
        ├── sendTaggedFloat(MSG_DC_SPEED, DCGearboxRPM)
        └── sendTaggedFloat(MSG_TIMESTAMP, millis())

**Dropdown Level 2: DC PID Calculation**
└── **PID Controller Algorithm**
    ├── **1. Calculate Error**
    │   └── Error = Setpoint - Current_RPM
    ├── **2. Proportional Term**
    │   └── P = Kp × Error
    ├── **3. Integral Term (with Anti-Windup)**
    │   ├── integralSum += Error × dt
    │   ├── IF integralSum > MAX → integralSum = MAX
    │   ├── IF integralSum < MIN → integralSum = MIN
    │   └── I = Ki × integralSum
    ├── **4. Derivative Term**
    │   ├── D = Kd × (Error - previousError) / dt
    │   └── previousError = Error
    ├── **5. Total Output**
    │   └── output = (P + I + D) × 10
    ├── **6. Output Saturation (with Anti-Stall)**
    │   ├── IF output > 4095 → output = 4095
    │   ├── IF output < -4095 → output = -4095
    │   ├── IF 0 < output < 800 → output = 800 (Anti-stall)
    │   └── IF -800 < output < 0 → output = -800
    └── **7. Send to Motor**
        └── DCmotorControl(direction, output)

**Dropdown Level 3: Encoder Reading**
└── **Encoder Interrupt Handler**
    ├── **Interrupt Source:** CHANGE edge on Encoder A pin
    ├── **ISR Function:** DChandleEncoderA()
    ├── **Process:**
    │   ├── Read Encoder A state
    │   ├── Read Encoder B state
    │   ├── Determine rotation direction (quadrature decoding)
    │   │   ├── A rising + B low → Increment
    │   │   ├── A rising + B high → Decrement
    │   │   ├── A falling + B high → Increment
    │   │   └── A falling + B low → Decrement
    │   └── Update encoder count
    └── **Critical Section Protection**
        ├── portENTER_CRITICAL_ISR()
        ├── Update shared variable
        └── portEXIT_CRITICAL_ISR()

---

### E. ALUR KONTROL MOTOR AC (Plant Board)

**Dropdown Level 1: AC Motor Control Loop**
└── **Main Control Flow**
    ├── **Step 1: Timer Interrupt** (variable interval)
    │   └── Set flag: ACnewDataReady = TRUE
    ├── **Step 2: Check Speed Request**
    │   ├── IF acspeedRequest = TRUE → Lanjut proses
    │   └── IF acspeedRequest = FALSE → Motor OFF
    ├── **Step 3: Read Encoder with Kalman Filter**
    │   ├── Read raw encoder pulses
    │   ├── Apply Kalman Filter
    │   │   ├── Prediction step
    │   │   ├── Calculate Kalman gain
    │   │   └── Update estimate
    │   └── ACrpm = filtered_estimate
    ├── **Step 4: Mode Selection**
    │   ├── **IF PID Mode:**
    │   │   ├── Calculate PID (see dropdown below)
    │   │   └── Send PWM to motor
    │   ├── **IF Manual Mode:**
    │   │   ├── PWM = map(setpoint, 0, 1500, 0, 255)
    │   │   └── Send PWM to motor
    │   └── **IF Motor OFF:**
    │       ├── PWM = 0
    │       ├── Reset integralSum = 0
    │       └── Reset previousError = 0
    └── **Step 5: Send Feedback**
        ├── sendTaggedFloat(MSG_AC_SPEED, ACrpm)
        └── sendTaggedFloat(MSG_TIMESTAMP, millis())

**Dropdown Level 2: AC PID Calculation**
└── **PID Controller Algorithm**
    ├── **1. Calculate Error**
    │   └── Error = Setpoint - Current_RPM
    ├── **2. Proportional Term**
    │   └── P = Kp × Error
    ├── **3. Integral Term (with Anti-Windup)**
    │   ├── integralSum += Error × dt
    │   ├── IF integralSum > MAX → integralSum = MAX
    │   ├── IF integralSum < MIN → integralSum = MIN
    │   └── I = Ki × integralSum
    ├── **4. Derivative Term**
    │   ├── D = Kd × (Error - previousError) / dt
    │   └── previousError = Error
    ├── **5. Total Output**
    │   └── output = P + I + D
    ├── **6. Output Saturation (with Anti-Stall)**
    │   ├── IF output > 255 → output = 255
    │   ├── IF output < 0 → output = 0
    │   └── IF 0 < output < 800 → output = 800 (Anti-stall)
    └── **7. Send to Motor**
        └── ACmotorControl(true, output, true, 0)

**Dropdown Level 3: Kalman Filter**
└── **Kalman Filter Algorithm**
    ├── **Initialization**
    │   ├── estimate = 0.0
    │   ├── error_estimate = 1.0
    │   ├── process_noise = 0.01
    │   └── measure_noise = 3.0
    ├── **Prediction Step**
    │   ├── estimate = previous_estimate (no dynamic model)
    │   └── error_estimate = previous_error_estimate + process_noise
    ├── **Update Step**
    │   ├── kalman_gain = error_estimate / (error_estimate + measure_noise)
    │   ├── estimate = estimate + kalman_gain × (measurement - estimate)
    │   └── error_estimate = (1 - kalman_gain) × error_estimate
    └── **Output**
        └── filtered_RPM = estimate

---

### F. ALUR DATA STORAGE (Plant Board)

**Dropdown Level 1: EEPROM Management**
└── **Data Storage System**
    ├── **Initialization (Startup)**
    │   ├── Check EEPROM magic number (validation)
    │   ├── IF valid → Load stored parameters
    │   └── IF invalid → Use default values & save
    ├── **Read from EEPROM**
    │   ├── Address map:
    │   │   ├── 0-3: Magic number
    │   │   ├── 4-7: DC Kp
    │   │   ├── 8-11: DC Ki
    │   │   ├── 12-15: DC Kd
    │   │   ├── 16-19: AC Kp
    │   │   ├── 20-23: AC Ki
    │   │   └── 24-27: AC Kd
    │   └── EEPROM.get(address, variable)
    ├── **Write to EEPROM**
    │   ├── Receive new PID parameter from Interface
    │   ├── Update internal variable
    │   ├── EEPROM.put(address, value)
    │   └── EEPROM.commit()
    └── **Print Stored Data**
        └── Serial output untuk debugging

---

## 3. SEQUENCE DIAGRAM (Step-by-Step)

### Scenario 1: User Mengubah Setpoint DC Motor

```
Step 1: User Interface
└── User memutar encoder → pilih "Escalator"
    └── Single click → masuk menu Escalator

Step 2: Menu Navigation
└── Encoder turn → pilih "Control"
    └── Single click → masuk Control View

Step 3: Parameter Selection
└── Encoder turn → highlight "Setpoint"
    └── Single click → enter edit mode (g_isEditing = TRUE)

Step 4: Value Adjustment
└── Encoder turn (UP mode) → nilai naik
    ├── Tampilan update real-time
    └── Range: 0-115 RPM

Step 5: Confirm & Send
└── Single click → confirm
    ├── g_dcSetpoint = new_value
    ├── sendTaggedFloat(MSG_DC_Setpoint, new_value)
    └── g_isEditing = FALSE

Step 6: ESP-NOW Transmission
└── Interface Board → Plant Board
    ├── Packet: [14, new_value]
    └── Wait callback: OnDataSent()

Step 7: Plant Board Reception
└── receiveData() callback triggered
    ├── Parse typeId = 14 (MSG_DC_Setpoint)
    ├── DCsetpoint = new_value
    └── (Tidak disimpan ke EEPROM, hanya runtime)

Step 8: PID Controller Update
└── Next timer interrupt (50ms)
    ├── Calculate error with new setpoint
    ├── PID calculation
    ├── Adjust PWM output
    └── Motor speed changes

Step 9: Feedback to Interface
└── Plant Board → Interface Board
    ├── sendTaggedFloat(MSG_DC_SPEED, current_rpm)
    └── Real-time graph update

Step 10: Graph Display Update
└── Interface Board
    ├── receiveData() → update buffer
    ├── g_escSpeedUpdated = TRUE
    ├── menuUpdate() → redraw graph
    └── User sees speed approaching setpoint
```

---

### Scenario 2: User Mengubah Parameter PID (Kp)

```
Step 1-3: (Sama seperti Scenario 1, masuk ke Control View)

Step 4: Parameter Selection
└── Encoder turn → highlight "Kp"
    └── Single click → enter edit mode

Step 5: Value Adjustment
└── Encoder turn → adjust Kp value
    ├── Increment: ±0.1
    └── Display shows current value

Step 6: Confirm & Send
└── Single click → confirm
    ├── g_dcKp = new_value
    ├── sendTaggedFloat(MSG_DC_KP, new_value)
    └── g_isEditing = FALSE

Step 7: Plant Board Reception
└── receiveData() callback
    ├── Parse typeId = 11 (MSG_DC_KP)
    ├── DCkP = new_value
    └── **Store to EEPROM:**
        ├── EEPROM.put(ADDRESS_DC_KP, new_value)
        └── EEPROM.commit()

Step 8: PID Controller Update
└── Next PID calculation uses new Kp
    ├── Proportional term affected
    └── Motor response changes

Step 9: Persistent Storage
└── Parameter tersimpan di EEPROM
    └── Tetap ada setelah restart
```

---

### Scenario 3: User Start/Stop Motor

```
Step 1: User in Graph View
└── Double click → Toggle motor Run/Stop

Step 2: Interface Processing
└── menuDoubleClick() function
    ├── g_escRunning = !g_escRunning
    └── sendTaggedFloat(MSG_DC_Control, g_escRunning ? 1 : 0)

Step 3: Plant Board Reception
└── receiveData() callback
    ├── Parse typeId = 15 (MSG_DC_Control)
    ├── DCMode = value (0 atau 1)
    └── Update motor state

Step 4: Motor Control
└── IF DCMode = TRUE:
    ├── Timer continues
    ├── PID calculation active
    └── PWM output to motor
    
    IF DCMode = FALSE:
    ├── PWM = 0
    ├── Motor stops
    └── Speed feedback = 0

Step 5: Display Update
└── Interface Board
    ├── Status indicator updated
    ├── Graph continues (showing speed = 0)
    └── "RUN" atau "STOP" indicator
```

---

## 4. DATA FLOW DIAGRAM

### A. Command Flow (User → Motor)

```
Level 1: User Input
├── Rotary Encoder
│   └── Position change detected
├── Button Press
│   └── Click type detected
└── Menu System Processing
    └── Parameter value updated

    ↓ (Forward to Level 2)

Level 2: Interface Board Processing
├── Value validation
├── Display update
└── Prepare ESP-NOW packet
    ├── Type ID (4 bytes)
    └── Value (4 bytes float)

    ↓ (Transmit via ESP-NOW)

Level 3: Wireless Transmission
├── esp_now_send() called
├── WiFi radio transmission
└── Callback: OnDataSent()
    ├── Success → Log OK
    └── Fail → Retry or error

    ↓ (Received by Plant Board)

Level 4: Plant Board Reception
├── receiveData() callback
├── Parse packet
│   ├── Extract Type ID
│   └── Extract Value
└── Route to handler
    ├── PID parameter → Store EEPROM
    ├── Setpoint → Update runtime variable
    ├── Control → Update motor state
    └── Mode → Update control mode

    ↓ (Execute control)

Level 5: Motor Control Execution
├── Timer interrupt triggered
├── PID calculation (if enabled)
├── PWM signal generation
└── Motor driver output
    └── Motor speed changes
```

---

### B. Feedback Flow (Motor → Display)

```
Level 1: Sensor Reading
├── Encoder pulse interrupt
│   ├── Quadrature decoding
│   └── Pulse counter update
└── Timer interrupt (50ms)
    └── Calculate RPM from pulse count

    ↓ (Process data)

Level 2: Data Processing
├── RPM calculation
├── Kalman Filter (AC motor only)
├── Moving average (optional)
└── Prepare feedback packet

    ↓ (Transmit via ESP-NOW)

Level 3: Wireless Transmission
├── sendTaggedFloat(MSG_DC_SPEED, rpm)
├── WiFi radio transmission
└── Callback: OnDataSent()

    ↓ (Received by Interface Board)

Level 4: Interface Board Reception
├── receiveData() callback
├── Parse packet
│   ├── Type ID = 10 or 20
│   └── Value = current RPM
└── Update internal state
    ├── g_escSpeed = value
    ├── Update history buffer
    └── Set update flag = TRUE

    ↓ (Update display)

Level 5: Display Update
├── menuUpdate() checks flag
├── IF flag = TRUE:
│   ├── Calculate Y coordinate
│   ├── Shift graph pixels left
│   ├── Draw new pixel (right edge)
│   └── Clear update flag
└── User sees real-time graph
```

---

## 5. TIMING & SYNCHRONIZATION

### A. Interface Board Timing

```
Main Loop (~10ms cycle)
├── [0ms] Check encoder position
│   └── IF changed → menuNavigate()
├── [2ms] Check button click
│   ├── Single click → menuSelect()
│   ├── Double click → menuDoubleClick()
│   └── Long click → menuBack()
├── [5ms] menuUpdate()
│   ├── Check g_escSpeedUpdated flag
│   ├── Check g_motorSpeedUpdated flag
│   └── Redraw if needed
└── [10ms] Loop back

ESP-NOW Reception (Asynchronous)
└── receiveData() callback
    ├── Triggered by incoming packet
    ├── ~1ms processing time
    └── Set update flags
```

---

### B. Plant Board Timing

```
DC Motor Control Loop
├── Timer Interrupt: 50ms
│   └── DConTimer() ISR
│       └── Set DCnewDataReady flag
├── Main Loop: ~1ms
│   └── IF DCnewDataReady:
│       ├── Read encoder
│       ├── Calculate RPM
│       ├── PID calculation (~0.5ms)
│       ├── PWM output
│       └── Send feedback (~2ms)
└── Total cycle: 50ms

AC Motor Control Loop
├── Timer Interrupt: Variable (depends on motor)
│   └── AConTimer() ISR
│       └── Set ACnewDataReady flag
├── Main Loop: ~1ms
│   └── IF ACnewDataReady:
│       ├── Read encoder
│       ├── Kalman Filter (~0.3ms)
│       ├── Calculate RPM
│       ├── PID calculation (~0.5ms)
│       ├── PWM output
│       └── Send feedback (~2ms)
└── Total cycle: Variable

ESP-NOW Reception (Asynchronous)
└── receiveData() callback
    ├── Triggered by incoming packet
    ├── Parse and update variables (~0.5ms)
    └── EEPROM write (if needed, ~20ms)
```

---

## 6. ERROR HANDLING & EDGE CASES

### A. Communication Errors

**Dropdown: ESP-NOW Transmission Failure**
└── **Detection:**
    ├── OnDataSent() callback returns FAIL
    └── Timeout (no response dalam 100ms)
    
└── **Handling:**
    ├── **Interface Board:**
    │   ├── Update connection indicator (RED)
    │   ├── Display "Connection Lost"
    │   ├── Retry transmission (max 3x)
    │   └── IF still fail → Alert user
    └── **Plant Board:**
        ├── IF no command dalam 5 detik:
        │   ├── Continue last valid command
        │   └── Safety mode (optional: motor stop)
        └── LED indicator blink (disconnected)

---

### B. Motor Control Errors

**Dropdown: Motor Stall Detection**
└── **Detection:**
    ├── Setpoint > 0
    ├── PWM output > threshold
    └── Speed = 0 (atau sangat rendah)
    
└── **Handling:**
    ├── **Anti-Stall Protection:**
    │   ├── IF 0 < PWM < 800 → PWM = 800
    │   └── Ensure minimum torque
    ├── **Timeout Protection:**
    │   ├── IF stall > 5 seconds:
    │   │   ├── Stop motor
    │   │   ├── Reset integral sum
    │   │   └── Alert user
    └── **Recovery:**
        └── User restart motor manually

---

**Dropdown: PID Integral Windup**
└── **Detection:**
    ├── Large sustained error
    └── Integral term growing unbounded
    
└── **Handling:**
    ├── **Anti-Windup:**
    │   ├── Clamp integral sum
    │   │   ├── DC: ±1000
    │   │   └── AC: ±1000
    │   └── IF motor OFF:
    │       ├── Reset integral sum = 0
    │       └── Reset previous error = 0
    └── **Effect:**
        └── Prevent overshoot saat restart

---

### C. Input Validation

**Dropdown: Parameter Range Checking**
└── **Validation:**
    ├── **Kp, Ki, Kd:**
    │   ├── Min: 0.0
    │   └── Max: 100.0
    ├── **DC Setpoint:**
    │   ├── Min: 0.0 RPM
    │   └── Max: 115.0 RPM
    ├── **AC Setpoint:**
    │   ├── Min: 0.0 RPM
    │   └── Max: 1500.0 RPM
    └── **IF out of range:**
        ├── Clamp to min/max
        └── Display warning

---

## 7. TIPS UNTUK MEMBUAT DI MS WORD

### Metode 1: Gunakan SmartArt
1. **Insert → SmartArt → Hierarchy atau Process**
2. Pilih layout yang sesuai (misalnya: Vertical Bullet List)
3. Isi text di setiap box
4. Format warna dan style

### Metode 2: Gunakan Shapes Manual
1. **Insert → Shapes → Rectangle**
2. Buat box untuk setiap komponen
3. **Insert → Shapes → Arrow** untuk panah
4. Group shapes untuk mempermudah move

### Metode 3: Gunakan Table
1. **Insert → Table**
2. Merge cells untuk header
3. Buat nested bullets di dalam cell
4. Remove borders untuk aesthetic

### Metode 4: Gunakan Outline View
1. **View → Outline**
2. Ketik hierarchical list dengan Tab indentation
3. Auto-numbering untuk level
4. Convert to body text untuk formatting

### Format Teks yang Disarankan:
- **Level 1 (Sistem Utama):** Bold, 14pt, Background biru
- **Level 2 (Subsistem):** Bold, 12pt, Background abu-abu muda
- **Level 3 (Detail):** Regular, 11pt, Bullet list
- **Arrows:** Use Wingdings atau Insert→Shapes→Arrow

---

## KESIMPULAN

Struktur dropdown ini memudahkan Anda untuk:
1. ✅ Membuat diagram di MS Word dengan SmartArt
2. ✅ Menggunakan Outline View untuk hierarchical structure
3. ✅ Copy-paste text langsung ke Word
4. ✅ Customize format sesuai kebutuhan dokumen
5. ✅ Print-friendly layout

Setiap level dapat di-expand/collapse di Word menggu nakan Outline mode untuk presentasi yang lebih clean.
