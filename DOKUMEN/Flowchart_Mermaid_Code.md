# FLOWCHART SISTEM - MERMAID CODE

## Petunjuk Penggunaan:
1. Copy code mermaid di bawah
2. Paste ke editor yang support mermaid (GitHub, Notion, Obsidian, atau mermaid.live)
3. Atau gunakan plugin mermaid di VS Code/Word
4. Render otomatis menjadi diagram visual

---

## 1. PLANT BOARD - MAIN SYSTEM FLOW

```mermaid
flowchart TD
    Start([START Setup]) --> Brownout[Disable Brownout]
    Brownout --> LED[Init LED Control]
    LED --> ESPNOW[Init ESP-NOW]
    ESPNOW --> Storage[Init Data Storage<br/>Load PID Parameters]
    Storage --> DCInit[Init DC Motor<br/>- Encoder<br/>- Timer]
    DCInit --> ACInit[Init AC Motor<br/>- Encoder<br/>- Timer<br/>- Kalman Filter]
    ACInit --> UpdatePID[Update PID Parameters<br/>Send to Interface Board]
    UpdatePID --> MainLoop{MAIN LOOP}
    
    MainLoop --> DCCheck{dcspeedRequest?}
    DCCheck -->|YES| DCProcess[DC PID Process]
    DCCheck -->|NO| DCSkip[ ]
    
    MainLoop --> ACCheck{acspeedRequest?}
    ACCheck -->|YES| ACProcess[AC PID Process]
    ACCheck -->|NO| ACSkip[ ]
    
    DCProcess --> MainLoop
    ACProcess --> MainLoop
    DCSkip --> MainLoop
    ACSkip --> MainLoop
    
    style Start fill:#2e7d32,stroke:#1b5e20,color:#fff
    style MainLoop fill:#ff9800,stroke:#e65100,color:#fff
    style DCProcess fill:#1976d2,stroke:#0d47a1,color:#fff
    style ACProcess fill:#1976d2,stroke:#0d47a1,color:#fff
```

---

## 2. DC MOTOR CONTROL WITH PID (DETAILED)

```mermaid
flowchart TD
    Timer[Timer Interrupt<br/>50ms] --> SetFlag[Set DCnewDataReady = TRUE]
    SetFlag --> CheckRequest{dcspeedRequest?}
    
    CheckRequest -->|FALSE| MotorOff[Motor OFF<br/>PWM = 0]
    MotorOff --> EndLoop([Return to Loop])
    
    CheckRequest -->|TRUE| ReadEncoder[Read Encoder<br/>DCpulseCount = DCencoder - DClastEncoder]
    ReadEncoder --> CalcRPM[Calculate RPM<br/>DCrpm = pulseCount × 60000 / interval / PPR]
    CalcRPM --> GearRatio[Gear Ratio Conversion<br/>DCGearboxRPM = DCrpm / 200]
    
    GearRatio --> CheckMode{Mode?}
    
    CheckMode -->|PID Mode| CalcError[Calculate Error<br/>Error = Setpoint - Current RPM]
    CalcError --> Proportional[Proportional Term<br/>P = Kp × Error]
    Proportional --> Integral[Integral Term with Anti-Windup<br/>integralSum += Error × dt<br/>Clamp to MAX/MIN<br/>I = Ki × integralSum]
    Integral --> Derivative[Derivative Term<br/>D = Kd × Error - previousError / dt]
    Derivative --> SumPID[Total Output<br/>output = P + I + D × 10]
    SumPID --> Saturate[Output Saturation<br/>with Anti-Stall]
    
    Saturate --> CheckSat{Output Range?}
    CheckSat -->|> 4095| SetMax[output = 4095]
    CheckSat -->|< -4095| SetMin[output = -4095]
    CheckSat -->|0 to 800| SetAntiStall[output = 800<br/>Anti-Stall]
    CheckSat -->|-800 to 0| SetAntiStallNeg[output = -800]
    CheckSat -->|OK| KeepOutput[Keep output]
    
    SetMax --> SendPWM[Send PWM to Motor<br/>DCmotorControl]
    SetMin --> SendPWM
    SetAntiStall --> SendPWM
    SetAntiStallNeg --> SendPWM
    KeepOutput --> SendPWM
    
    CheckMode -->|Manual Mode| ManualPWM[PWM = map<br/>setpoint: 0-115 → 0-4095]
    ManualPWM --> SendPWM
    
    CheckMode -->|Motor OFF| MotorOff2[PWM = 0]
    MotorOff2 --> SendPWM
    
    SendPWM --> SendFeedback[Send Feedback via ESP-NOW<br/>MSG_DC_SPEED, DCGearboxRPM<br/>MSG_TIMESTAMP, millis]
    SendFeedback --> ClearFlag[Clear DCnewDataReady]
    ClearFlag --> EndLoop
    
    style Timer fill:#ff5722,stroke:#d84315,color:#fff
    style CalcError fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style Proportional fill:#3f51b5,stroke:#283593,color:#fff
    style Integral fill:#3f51b5,stroke:#283593,color:#fff
    style Derivative fill:#3f51b5,stroke:#283593,color:#fff
    style SendPWM fill:#4caf50,stroke:#2e7d32,color:#fff
```

---

## 3. AC MOTOR CONTROL WITH PID & KALMAN FILTER

```mermaid
flowchart TD
    Timer[Timer Interrupt<br/>Variable Interval] --> SetFlag[Set ACnewDataReady = TRUE]
    SetFlag --> CheckRequest{acspeedRequest?}
    
    CheckRequest -->|FALSE| MotorOff[Motor OFF<br/>PWM = 0<br/>Reset Integral]
    MotorOff --> EndLoop([Return to Loop])
    
    CheckRequest -->|TRUE| ReadRaw[Read Raw Encoder Pulses]
    ReadRaw --> Kalman[Apply Kalman Filter]
    
    Kalman --> Predict[Prediction Step<br/>estimate = prev_estimate<br/>error_est += process_noise]
    Predict --> Update[Update Step<br/>kalman_gain = error_est / error_est + measure_noise<br/>estimate += gain × measurement - estimate]
    Update --> FilterOut[Filtered RPM<br/>ACrpm = estimate]
    
    FilterOut --> CheckMode{Mode?}
    
    CheckMode -->|PID Mode| CalcError[Calculate Error<br/>Error = Setpoint - ACrpm]
    CalcError --> Proportional[Proportional Term<br/>P = Kp × Error]
    Proportional --> Integral[Integral Term<br/>with Anti-Windup<br/>I = Ki × integralSum]
    Integral --> Derivative[Derivative Term<br/>D = Kd × dError/dt]
    Derivative --> SumPID[Total Output<br/>output = P + I + D]
    SumPID --> Saturate{Saturation Check}
    
    Saturate -->|> 255| SetMax[output = 255]
    Saturate -->|< 0| SetMin[output = 0]
    Saturate -->|0 to 800| AntiStall[output = 800<br/>Anti-Stall]
    Saturate -->|OK| KeepOut[Keep output]
    
    SetMax --> SendPWM[Send PWM to Motor<br/>ACmotorControl]
    SetMin --> SendPWM
    AntiStall --> SendPWM
    KeepOut --> SendPWM
    
    CheckMode -->|Manual Mode| ManualPWM[PWM = map<br/>setpoint: 0-1500 → 0-255]
    ManualPWM --> SendPWM
    
    CheckMode -->|Motor OFF| MotorOff
    
    SendPWM --> SendFeedback[Send Feedback via ESP-NOW<br/>MSG_AC_SPEED, ACrpm<br/>MSG_TIMESTAMP, millis]
    SendFeedback --> ClearFlag[Clear ACnewDataReady]
    ClearFlag --> EndLoop
    
    style Timer fill:#ff5722,stroke:#d84315,color:#fff
    style Kalman fill:#00bcd4,stroke:#0097a7,color:#fff
    style Predict fill:#00acc1,stroke:#006064,color:#fff
    style Update fill:#00acc1,stroke:#006064,color:#fff
    style SendPWM fill:#4caf50,stroke:#2e7d32,color:#fff
```

---

## 4. INTERFACE BOARD - MAIN SYSTEM FLOW

```mermaid
flowchart TD
    Start([START Setup]) --> Serial[Init Serial 115200]
    Serial --> ESPNOW[Init ESP-NOW]
    ESPNOW --> Encoder[Init Rotary Encoder]
    Encoder --> TFT[Init TFT Display<br/>ST7735]
    TFT --> MenuInit[Menu System Init<br/>Show Splash Screen]
    MenuInit --> Restart[Send ESP_RESTART<br/>to Plant Board]
    Restart --> MainLoop{MAIN LOOP}
    
    MainLoop --> ReadEnc[Read Rotary Encoder]
    ReadEnc --> CheckPos{Position<br/>Changed?}
    CheckPos -->|YES| Navigate[menuNavigate<br/>Scroll Menu/Adjust Value]
    CheckPos -->|NO| CheckBtn[Check Button Click]
    Navigate --> CheckBtn
    
    CheckBtn --> ClickType{Click Type?}
    
    ClickType -->|Single Click| Select[menuSelect<br/>Enter Menu/Confirm]
    ClickType -->|Double Click| Toggle[menuDoubleClick<br/>Toggle Run/Stop or<br/>Encoder Mode]
    ClickType -->|Long Click| Back[menuBack<br/>Return to Previous Menu]
    ClickType -->|None| Update[menuUpdate<br/>Update Display]
    
    Select --> Update
    Toggle --> Update
    Back --> Update
    
    Update --> CheckFlags{Update Flags?}
    CheckFlags -->|escSpeedUpdated| UpdateGraphDC[Update DC Graph<br/>Shift Pixels<br/>Draw New Point]
    CheckFlags -->|motorSpeedUpdated| UpdateGraphAC[Update AC Graph<br/>Shift Pixels<br/>Draw New Point]
    CheckFlags -->|None| Delay[Delay 10ms]
    
    UpdateGraphDC --> Delay
    UpdateGraphAC --> Delay
    Delay --> MainLoop
    
    style Start fill:#2e7d32,stroke:#1b5e20,color:#fff
    style MainLoop fill:#ff9800,stroke:#e65100,color:#fff
    style Select fill:#1976d2,stroke:#0d47a1,color:#fff
    style Toggle fill:#1976d2,stroke:#0d47a1,color:#fff
    style Back fill:#1976d2,stroke:#0d47a1,color:#fff
    style UpdateGraphDC fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style UpdateGraphAC fill:#9c27b0,stroke:#6a1b9a,color:#fff
```

---

## 5. MENU NAVIGATION SYSTEM (STATE MACHINE)

```mermaid
stateDiagram-v2
    [*] --> InitSplash: Power On
    InitSplash --> RootMenu: Delay 2s
    
    RootMenu --> Escalator: Select [0]
    RootMenu --> MotorAC: Select [1]
    RootMenu --> WiFiStatus: Select [2]
    
    Escalator --> EscGraph: Enter (Graph Default)
    Escalator --> EscControl: Navigate & Select
    
    EscGraph --> EscControl: Single Click
    EscControl --> EscGraph: Long Click (Back)
    
    EscGraph --> RootMenu: Long Click (Back)
    EscControl --> RootMenu: Long Click (Back)
    
    MotorAC --> ACGraph: Enter (Graph Default)
    MotorAC --> ACControl: Navigate & Select
    
    ACGraph --> ACControl: Single Click
    ACControl --> ACGraph: Long Click (Back)
    
    ACGraph --> RootMenu: Long Click (Back)
    ACControl --> RootMenu: Long Click (Back)
    
    WiFiStatus --> RootMenu: Long Click (Back)
    
    state EscControl {
        [*] --> ViewParams
        ViewParams --> EditKp: Select Kp
        ViewParams --> EditKi: Select Ki
        ViewParams --> EditKd: Select Kd
        ViewParams --> EditSetpoint: Select Setpoint
        ViewParams --> EditRunStop: Select Run/Stop
        
        EditKp --> ViewParams: Confirm (Single Click)
        EditKi --> ViewParams: Confirm
        EditKd --> ViewParams: Confirm
        EditSetpoint --> ViewParams: Confirm
        EditRunStop --> ViewParams: Confirm
    }
    
    state ACControl {
        [*] --> ViewParams2
        ViewParams2 --> EditKp2: Select Kp
        ViewParams2 --> EditKi2: Select Ki
        ViewParams2 --> EditKd2: Select Kd
        ViewParams2 --> EditSetpoint2: Select Setpoint
        ViewParams2 --> EditRunStop2: Select Run/Stop
        
        EditKp2 --> ViewParams2: Confirm
        EditKi2 --> ViewParams2: Confirm
        EditKd2 --> ViewParams2: Confirm
        EditSetpoint2 --> ViewParams2: Confirm
        EditRunStop2 --> ViewParams2: Confirm
    }
```

---

## 6. ESP-NOW COMMUNICATION FLOW (SEQUENCE)

```mermaid
sequenceDiagram
    participant User
    participant Interface as Interface Board<br/>(ESP8266)
    participant Plant as Plant Board<br/>(ESP32)
    participant Motor as DC/AC Motor
    
    Note over User,Motor: Scenario: User Changes Setpoint
    
    User->>Interface: Rotate Encoder<br/>Select "Escalator"
    User->>Interface: Single Click<br/>Enter Menu
    Interface->>Interface: menuSelect()<br/>Enter Control View
    
    User->>Interface: Rotate Encoder<br/>Highlight "Setpoint"
    User->>Interface: Single Click<br/>Enter Edit Mode
    
    User->>Interface: Rotate Encoder<br/>Adjust Value (0-115)
    Interface->>Interface: Update Display<br/>Show Current Value
    
    User->>Interface: Single Click<br/>Confirm
    Interface->>Interface: g_dcSetpoint = new_value
    Interface->>Plant: ESP-NOW Send<br/>MSG_DC_Setpoint (14)<br/>Value: new_value
    
    Plant->>Plant: receiveData() Callback<br/>Parse typeId = 14
    Plant->>Plant: DCsetpoint = new_value
    
    Note over Plant,Motor: Next Timer Interrupt (50ms)
    
    Plant->>Plant: Calculate Error<br/>Error = new_setpoint - current_rpm
    Plant->>Plant: PID Calculation<br/>P + I + D
    Plant->>Motor: PWM Output<br/>Adjust Speed
    Motor->>Motor: Speed Changes
    
    Plant->>Interface: ESP-NOW Send<br/>MSG_DC_SPEED (10)<br/>Value: current_rpm
    
    Interface->>Interface: receiveData() Callback<br/>Update g_escSpeed
    Interface->>Interface: Add to History Buffer<br/>Set g_escSpeedUpdated = TRUE
    Interface->>Interface: menuUpdate()<br/>Redraw Graph
    
    Note over User,Interface: User Sees Graph Update
```

---

## 7. DATA FLOW - COMMAND (USER → MOTOR)

```mermaid
flowchart LR
    subgraph User Layer
        A1[User Input<br/>Rotary Encoder<br/>Button Press]
    end
    
    subgraph Interface Board
        B1[Detect Input<br/>Position/Click]
        B2[Menu Processing<br/>Navigate/Edit]
        B3[Value Validation<br/>Range Check]
        B4[Create ESP-NOW Packet<br/>TypeID 4B + Value 4B]
        B5[esp_now_send<br/>WiFi TX]
    end
    
    subgraph Wireless
        C1[ESP-NOW Protocol<br/>2.4GHz]
    end
    
    subgraph Plant Board
        D1[receiveData Callback<br/>Parse Packet]
        D2{Message Type?}
        D3[PID Parameter<br/>Store to EEPROM]
        D4[Setpoint<br/>Runtime Variable]
        D5[Control ON/OFF<br/>Motor State]
        D6[Timer Interrupt<br/>50ms]
        D7[PID Calculation<br/>P + I + D]
        D8[PWM Generation<br/>0-4095 DC<br/>0-255 AC]
    end
    
    subgraph Motor Layer
        E1[Motor Driver<br/>H-Bridge/TRIAC]
        E2[Motor Rotation<br/>Speed Change]
    end
    
    A1 --> B1
    B1 --> B2
    B2 --> B3
    B3 --> B4
    B4 --> B5
    B5 --> C1
    C1 --> D1
    D1 --> D2
    D2 -->|Kp/Ki/Kd| D3
    D2 -->|Setpoint| D4
    D2 -->|Control| D5
    D3 --> D6
    D4 --> D6
    D5 --> D6
    D6 --> D7
    D7 --> D8
    D8 --> E1
    E1 --> E2
    
    style A1 fill:#4caf50,stroke:#2e7d32,color:#fff
    style C1 fill:#ff9800,stroke:#e65100,color:#fff
    style D7 fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style E2 fill:#f44336,stroke:#c62828,color:#fff
```

---

## 8. DATA FLOW - FEEDBACK (MOTOR → DISPLAY)

```mermaid
flowchart LR
    subgraph Sensor Layer
        A1[Encoder Pulse<br/>Quadrature Signal]
        A2[Interrupt Handler<br/>DChandleEncoderA]
        A3[Pulse Counter<br/>DCencoder++]
    end
    
    subgraph Plant Board Processing
        B1[Timer Interrupt<br/>50ms]
        B2[Calculate Pulse Count<br/>DCpulseCount = current - last]
        B3[Calculate RPM<br/>pulseCount × 60000 / interval / PPR]
        B4[Kalman Filter<br/>AC Motor Only<br/>Noise Reduction]
        B5[Create Feedback Packet<br/>MSG_DC_SPEED TypeID 10<br/>Value: RPM]
        B6[esp_now_send<br/>WiFi TX]
    end
    
    subgraph Wireless
        C1[ESP-NOW Protocol<br/>Bidirectional]
    end
    
    subgraph Interface Board Reception
        D1[receiveData Callback<br/>Parse Packet]
        D2{TypeID?}
        D3[MSG_DC_SPEED 10<br/>Update g_escSpeed]
        D4[MSG_AC_SPEED 20<br/>Update g_motorSpeed]
        D5[Add to Circular Buffer<br/>history index++<br/>100 samples max]
        D6[Set Update Flag<br/>g_escSpeedUpdated = TRUE]
    end
    
    subgraph Display Update
        E1[menuUpdate Loop<br/>Check Flags]
        E2[Calculate Y Coordinate<br/>Scale: 0-115 → 0-70px DC<br/>0-1500 → 0-70px AC]
        E3[Shift Graph Left<br/>Move All Pixels -1]
        E4[Draw New Pixel<br/>Rightmost Column<br/>Yellow Color]
        E5[Clear Update Flag]
        E6[TFT Display Output<br/>User Sees Graph]
    end
    
    A1 --> A2
    A2 --> A3
    A3 --> B1
    B1 --> B2
    B2 --> B3
    B3 --> B4
    B4 --> B5
    B5 --> B6
    B6 --> C1
    C1 --> D1
    D1 --> D2
    D2 -->|DC| D3
    D2 -->|AC| D4
    D3 --> D5
    D4 --> D5
    D5 --> D6
    D6 --> E1
    E1 --> E2
    E2 --> E3
    E3 --> E4
    E4 --> E5
    E5 --> E6
    
    style A1 fill:#3f51b5,stroke:#283593,color:#fff
    style B4 fill:#00bcd4,stroke:#0097a7,color:#fff
    style C1 fill:#ff9800,stroke:#e65100,color:#fff
    style E4 fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style E6 fill:#4caf50,stroke:#2e7d32,color:#fff
```

---

## 9. ENCODER INTERRUPT HANDLER

```mermaid
flowchart TD
    Start([Encoder Pin Change<br/>CHANGE Interrupt]) --> ISR[ISR: DChandleEncoderA]
    ISR --> Enter[portENTER_CRITICAL_ISR]
    Enter --> ReadA[Read Encoder A State<br/>digitalRead ENCODER_A_PIN]
    ReadA --> ReadB[Read Encoder B State<br/>digitalRead ENCODER_B_PIN]
    
    ReadB --> CheckA{A State<br/>Changed?}
    
    CheckA -->|A != lastA| CheckARising{A Rising<br/>or Falling?}
    CheckA -->|A == lastA| Exit[portEXIT_CRITICAL_ISR]
    
    CheckARising -->|A Rising HIGH| CheckB1{B State?}
    CheckARising -->|A Falling LOW| CheckB2{B State?}
    
    CheckB1 -->|B = LOW| Inc1[DCencoder++<br/>Forward Direction]
    CheckB1 -->|B = HIGH| Dec1[DCencoder--<br/>Reverse Direction]
    
    CheckB2 -->|B = HIGH| Inc2[DCencoder++<br/>Forward Direction]
    CheckB2 -->|B = LOW| Dec2[DCencoder--<br/>Reverse Direction]
    
    Inc1 --> Update[Update lastStateA = A]
    Dec1 --> Update
    Inc2 --> Update
    Dec2 --> Update
    
    Update --> Exit
    Exit --> Return([Return from ISR])
    
    style Start fill:#ff5722,stroke:#d84315,color:#fff
    style ISR fill:#f44336,stroke:#c62828,color:#fff
    style Enter fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style Inc1 fill:#4caf50,stroke:#2e7d32,color:#fff
    style Inc2 fill:#4caf50,stroke:#2e7d32,color:#fff
    style Dec1 fill:#ff9800,stroke:#e65100,color:#fff
    style Dec2 fill:#ff9800,stroke:#e65100,color:#fff
```

---

## 10. EEPROM DATA STORAGE FLOW

```mermaid
flowchart TD
    Start([System Startup]) --> Init[EEPROM.begin<br/>Size: 512 bytes]
    Init --> CheckMagic[Read Magic Number<br/>Address 0-3]
    CheckMagic --> Valid{Magic<br/>Valid?}
    
    Valid -->|YES| LoadData[Load Stored Parameters]
    Valid -->|NO| DefaultData[Use Default Values<br/>DC: Kp=1.5 Ki=0.1 Kd=0.05<br/>AC: Kp=2.0 Ki=0.08 Kd=0.04]
    
    LoadData --> ReadDCKp[Read DC Kp<br/>Address 4-7]
    ReadDCKp --> ReadDCKi[Read DC Ki<br/>Address 8-11]
    ReadDCKi --> ReadDCKd[Read DC Kd<br/>Address 12-15]
    ReadDCKd --> ReadACKp[Read AC Kp<br/>Address 16-19]
    ReadACKp --> ReadACKi[Read AC Ki<br/>Address 20-23]
    ReadACKi --> ReadACKd[Read AC Kd<br/>Address 24-27]
    ReadACKd --> PrintData[Print Stored Data<br/>Serial Debug]
    
    DefaultData --> SaveDefaults[Save Default to EEPROM<br/>EEPROM.put + commit]
    SaveDefaults --> PrintData
    
    PrintData --> Ready([System Ready])
    
    Runtime[Runtime: Receive New PID Param] --> CheckType{Parameter<br/>Type?}
    CheckType -->|Kp/Ki/Kd| UpdateVar[Update Variable<br/>DCkP = new_value]
    CheckType -->|Setpoint/Control| SkipSave[Skip EEPROM Save<br/>Runtime Only]
    
    UpdateVar --> WriteEEPROM[EEPROM.put<br/>address, value]
    WriteEEPROM --> Commit[EEPROM.commit]
    Commit --> Confirm[Send Confirmation<br/>Serial Print]
    Confirm --> Continue([Continue Operation])
    SkipSave --> Continue
    
    style Start fill:#2e7d32,stroke:#1b5e20,color:#fff
    style Valid fill:#ff9800,stroke:#e65100,color:#000
    style LoadData fill:#1976d2,stroke:#0d47a1,color:#fff
    style DefaultData fill:#f44336,stroke:#c62828,color:#fff
    style WriteEEPROM fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style Commit fill:#9c27b0,stroke:#6a1b9a,color:#fff
```

---

## 11. GRAPH DISPLAY UPDATE (OSCILLOSCOPE STYLE)

```mermaid
flowchart TD
    Start([New Speed Data Arrived]) --> Callback[receiveData Callback<br/>MSG_DC_SPEED or MSG_AC_SPEED]
    Callback --> ParseValue[Extract RPM Value<br/>float rpm]
    ParseValue --> UpdateGlobal[Update Global Variable<br/>g_escSpeed = rpm]
    UpdateGlobal --> AddBuffer[Add to Circular Buffer<br/>history index++ % 100]
    AddBuffer --> SetFlag[Set Update Flag<br/>g_escSpeedUpdated = TRUE]
    SetFlag --> WaitMain([Wait for Main Loop])
    
    WaitMain --> MainLoop[menuUpdate Called<br/>in Main Loop]
    MainLoop --> CheckFlag{Update Flag<br/>TRUE?}
    
    CheckFlag -->|FALSE| SkipUpdate([Skip Update])
    CheckFlag -->|TRUE| CheckMode{Full Redraw<br/>or Incremental?}
    
    CheckMode -->|Full Redraw| ClearArea[Clear Graph Area<br/>118x70 pixels]
    ClearArea --> DrawBorder[Draw Border<br/>White Rectangle]
    DrawBorder --> DrawSetpoint[Draw Setpoint Line<br/>Red Dashed Horizontal]
    DrawSetpoint --> DrawGrid[Draw Time Grid<br/>Cyan Vertical Lines per 1s]
    DrawGrid --> PlotAll[Plot All Historical Data<br/>Loop through 100 samples]
    
    PlotAll --> LoopSamples[For col = 0 to 115]
    LoopSamples --> Interpolate[Interpolate Buffer Index<br/>idx = historyIndex - offset % 100]
    Interpolate --> GetValue[value = history idx]
    GetValue --> CalcY[Calculate Y Coordinate<br/>y = 94 - value × 69 / MAX]
    CalcY --> ClampY[Clamp Y to 25-94 range]
    ClampY --> DrawPixel[Draw Pixel x, y<br/>Yellow Color]
    DrawPixel --> SaveY[Save Y to escGraphY col]
    SaveY --> NextCol{More<br/>Columns?}
    NextCol -->|YES| LoopSamples
    NextCol -->|NO| SetInit[escGraphInit = TRUE]
    
    CheckMode -->|Incremental| ShiftLoop[For col = 0 to 114]
    ShiftLoop --> EraseOld[Erase Old Pixel<br/>Draw x, escGraphY col BLACK]
    EraseOld --> DrawNew[Draw New Pixel<br/>Draw x, escGraphY col+1 YELLOW]
    DrawNew --> UpdateBuffer[escGraphY col = escGraphY col+1]
    UpdateBuffer --> NextShift{More to Shift?}
    NextShift -->|YES| ShiftLoop
    NextShift -->|NO| DrawNewest[Draw Newest Data<br/>Rightmost Column]
    
    DrawNewest --> GetLatest[value = history lastIndex]
    GetLatest --> CalcYNew[Calculate Y<br/>Scale to Pixel]
    CalcYNew --> DrawRight[Draw Pixel<br/>rightX, y YELLOW]
    DrawRight --> SaveRight[escGraphY 115 = y]
    
    SetInit --> ClearFlag[Clear Update Flag<br/>g_escSpeedUpdated = FALSE]
    SaveRight --> ClearFlag
    ClearFlag --> Done([Graph Updated<br/>User Sees Result])
    
    style Callback fill:#ff5722,stroke:#d84315,color:#fff
    style AddBuffer fill:#1976d2,stroke:#0d47a1,color:#fff
    style SetFlag fill:#ff9800,stroke:#e65100,color:#fff
    style DrawPixel fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style DrawNew fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style Done fill:#4caf50,stroke:#2e7d32,color:#fff
```

---

## 12. COMPLETE SYSTEM INTERACTION

```mermaid
graph TB
    subgraph User["👤 USER"]
        U1[Rotary Encoder<br/>Rotate & Click]
    end
    
    subgraph Interface["📱 INTERFACE BOARD (ESP8266)"]
        I1[Input Handler<br/>Encoder + Button]
        I2[Menu System<br/>Navigation & Edit]
        I3[TFT Display<br/>ST7735 128x160]
        I4[Graph Engine<br/>Real-time Plot]
        I5[ESP-NOW TX/RX<br/>WiFi 2.4GHz]
    end
    
    subgraph Wireless["📡 WIRELESS COMMUNICATION"]
        W1[ESP-NOW Protocol<br/>Peer-to-Peer<br/>Low Latency]
    end
    
    subgraph Plant["🎛️ PLANT BOARD (ESP32)"]
        P1[ESP-NOW TX/RX<br/>WiFi 2.4GHz]
        P2[Data Storage<br/>EEPROM<br/>PID Parameters]
        P3[DC Motor Controller<br/>Timer + PID]
        P4[AC Motor Controller<br/>Timer + PID + Kalman]
        P5[LED Indicators<br/>Status Display]
    end
    
    subgraph Motors["⚙️ MOTORS & SENSORS"]
        M1[DC Motor<br/>Escalator<br/>+ Rotary Encoder]
        M2[AC Motor<br/>Conveyor<br/>+ Rotary Encoder]
    end
    
    U1 <-->|User Interaction| I1
    I1 --> I2
    I2 --> I3
    I2 --> I4
    I3 --> U1
    I4 --> I3
    I2 <--> I5
    
    I5 <-->|Commands<br/>MSG_DC_Setpoint<br/>MSG_AC_Setpoint<br/>MSG_PID_MODE<br/>etc.| W1
    
    W1 <-->|Feedback<br/>MSG_DC_SPEED<br/>MSG_AC_SPEED<br/>MSG_TIMESTAMP| P1
    
    P1 --> P2
    P1 --> P3
    P1 --> P4
    P2 -.->|Load/Save| P3
    P2 -.->|Load/Save| P4
    P3 --> P5
    P4 --> P5
    
    P3 <-->|PWM Control<br/>Encoder Feedback| M1
    P4 <-->|PWM Control<br/>Encoder Feedback| M2
    
    style U1 fill:#4caf50,stroke:#2e7d32,color:#fff
    style I3 fill:#2196f3,stroke:#0d47a1,color:#fff
    style I4 fill:#9c27b0,stroke:#6a1b9a,color:#fff
    style W1 fill:#ff9800,stroke:#e65100,color:#fff
    style P2 fill:#795548,stroke:#4e342e,color:#fff
    style P3 fill:#f44336,stroke:#c62828,color:#fff
    style P4 fill:#f44336,stroke:#c62828,color:#fff
    style M1 fill:#607d8b,stroke:#37474f,color:#fff
    style M2 fill:#607d8b,stroke:#37474f,color:#fff
```

---

## CARA MENGGUNAKAN MERMAID CODE:

### **Option 1: GitHub / GitLab**
1. Copy code mermaid (termasuk ``` mermaid dan ```)
2. Paste langsung ke file `.md` di GitHub
3. Push ke repository
4. GitHub auto-render jadi diagram

### **Option 2: Mermaid Live Editor**
1. Buka https://mermaid.live
2. Paste code mermaid (tanpa ``` mermaid, hanya isinya)
3. Preview real-time di kanan
4. Export sebagai PNG/SVG/PDF

### **Option 3: VS Code**
1. Install extension: **Markdown Preview Mermaid Support**
2. Buka file `.md` dengan mermaid code
3. Klik **Preview** (Ctrl+Shift+V)
4. Diagram muncul di preview

### **Option 4: Notion**
1. Type `/code`
2. Pilih **Mermaid**
3. Paste mermaid code
4. Auto-render jadi diagram

### **Option 5: Obsidian**
1. Obsidian sudah support mermaid by default
2. Paste code dengan ``` mermaid wrapper
3. Preview mode akan render diagram

### **Option 6: Word (dengan Plugin)**
1. Install plugin: **Mermaid for Word** atau **Markdown to Word**
2. Atau: Render di mermaid.live → export PNG → insert ke Word

---

## LEGEND WARNA:

- 🟢 **Hijau** = Start/End, Success, Ready
- 🔵 **Biru** = Process, Normal Operation
- 🟣 **Ungu** = PID Calculation, Critical Section
- 🟠 **Orange** = Loop, Main Flow, Warning
- 🔴 **Merah** = Interrupt, Timer, Motor Control
- 🔷 **Cyan** = Kalman Filter, Data Processing
- ⚫ **Abu-abu** = Hardware, Physical Component

---

## TIPS CUSTOMIZATION:

### **Ubah Warna Shape:**
```
style NodeName fill:#hexcolor,stroke:#hexcolor,color:#textcolor
```

### **Tambah Icon/Emoji:**
```
Node[🚀 Text dengan Icon]
```

### **Ubah Jenis Arrow:**
- `-->` = Solid arrow
- `-.->` = Dotted arrow
- `==>` = Thick arrow
- `<-->` = Bidirectional

### **Subgraph untuk Grouping:**
```mermaid
subgraph Title
    Node1 --> Node2
end
```

Selamat menggunakan Mermaid! 🎨📊
