#include "ControlMenuSystem.h"

// ========================================
// CONSTRUCTOR
// ========================================

ControlMenuSystem::ControlMenuSystem(Adafruit_ST7735 *tft) {
  display = tft;
  currentMenu = MENU_ROOT;
  selectedIndex = 0;
  isEditing = false;

  lastGraphUpdate = 0;
  graphUpdateInterval = 100; // Update grafik setiap 100ms

  // Initialize data structures
  escalatorData = {};
  escalatorData.currentSpeed = 0;
  escalatorData.targetSpeed = 0;
  escalatorData.direction = DIR_FORWARD;
  escalatorData.mode = MODE_MANUAL;
  escalatorData.pid = {1.0, 0.1, 0.05, 50.0, 0};
  escalatorData.isConnected = false;
  escalatorData.historyIndex = 0;
  escalatorData.isRunning = false;

  motorACData = {};
  motorACData.currentSpeed = 0;
  motorACData.targetSpeed = 0;
  motorACData.mode = MODE_MANUAL;
  motorACData.controlType = CTRL_DAC;
  motorACData.voltageType = VOLTAGE_10V;
  motorACData.pid = {2.0, 0.5, 0.1, 1000.0, 0};
  motorACData.isConnected = false;
  motorACData.historyIndex = 0;

  networkStatus = {};
  networkStatus.wifiConnected = false;
  networkStatus.espnowInitialized = false;

  networkManager = nullptr;

  // Initialize previous values for partial redraw
  prevEscalatorSpeed = 0;
  prevEscalatorTarget = 0;
  prevMotorSpeed = 0;
  needFullRedraw = true; // Force full redraw on first menu

  // Define colors
  COLOR_BG = display->color565(0, 0, 20);
  COLOR_HEADER = display->color565(0, 80, 160);
  COLOR_SELECTED = display->color565(50, 150, 255);
  COLOR_EDITING = display->color565(255, 100, 0);
  COLOR_TEXT = display->color565(255, 255, 255);
  COLOR_ACCENT = display->color565(255, 200, 0);
  COLOR_SUCCESS = display->color565(0, 255, 0);
  COLOR_ERROR = display->color565(255, 0, 0);
  COLOR_WARNING = display->color565(255, 150, 0);
  COLOR_GRAPH_GRID = display->color565(40, 40, 60);
  COLOR_GRAPH_LINE = display->color565(0, 255, 200);
}

// ========================================
// INITIALIZATION
// ========================================

void ControlMenuSystem::begin() {
  display->fillScreen(COLOR_BG);
  showSplashScreen();
  delay(2000);
  drawRootMenu();
}

void ControlMenuSystem::showSplashScreen() {
  display->fillScreen(ST77XX_BLACK);

  // Title
  display->setTextColor(COLOR_ACCENT);
  display->setTextSize(2);
  display->setCursor(getCenterX("Control", 2), 50);
  display->println("Control");
  display->setCursor(getCenterX("System", 2), 70);
  display->println("System");

  // Subtitle
  display->setTextColor(COLOR_TEXT);
  display->setTextSize(1);
  display->setCursor(getCenterX("Escalator"), 100);
  display->println("Escalator");
  display->setCursor(getCenterX("& Motor"), 112);
  display->println("& Motor");

  // Version
  display->setTextColor(COLOR_ACCENT);
  display->setCursor(getCenterX("v2.0"), 135);
  display->println("v2.0");

  // Loading bar (vertical layout)
  int barWidth = 90;
  int barX = (128 - barWidth) / 2;
  int barY = 150;
  display->drawRect(barX, barY, barWidth, 6, COLOR_HEADER);

  for (int i = 0; i < barWidth - 2; i += 4) {
    display->fillRect(barX + 1, barY + 1, i, 4, COLOR_SUCCESS);
    delay(35);
  }
}

// ========================================
// HELPER FUNCTIONS
// ========================================

int ControlMenuSystem::getCenterX(const char *text, int textSize) {
  if (!display || !text)
    return 0; // Safety check
  int16_t x1, y1;
  uint16_t w, h;
  display->setTextSize(textSize);
  display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return (128 - w) / 2;
}

void ControlMenuSystem::drawHeader(const char *title, bool showBackIcon) {
  if (!display || !title)
    return; // Safety check

  display->fillRect(0, 0, 128, 18, COLOR_HEADER);

  // Back icon
  if (showBackIcon && currentMenu != MENU_ROOT) {
    display->fillTriangle(5, 9, 10, 5, 10, 13, COLOR_TEXT);
  }

  // Title (centered)
  display->setTextColor(ST77XX_WHITE);
  display->setTextSize(1);
  display->setCursor(getCenterX(title), 5);
  display->println(title);
}

void ControlMenuSystem::drawFooter(const char *text) {
  if (!display || !text)
    return; // Safety check

  display->fillRect(0, 142, 128, 18, COLOR_HEADER);
  display->setTextColor(ST77XX_WHITE);
  display->setTextSize(1);
  // Wrap text untuk vertical display
  display->setCursor(2, 145);

  // Potong text jika terlalu panjang (max ~21 karakter untuk width 128)
  String str = String(text);
  if (str.length() > 21) {
    str = str.substring(0, 18) + "...";
  }
  display->println(str);
}

void ControlMenuSystem::drawMenuItem(int y, const char *text, bool selected,
                                     bool hasSubmenu) {
  if (!display || !text)
    return; // Safety check

  if (selected) {
    display->fillRoundRect(3, y, 122, 16, 3,
                           isEditing ? COLOR_EDITING : COLOR_SELECTED);
    display->setTextColor(ST77XX_BLACK);
  } else {
    display->drawRoundRect(3, y, 122, 16, 3, COLOR_TEXT);
    display->setTextColor(COLOR_TEXT);
  }

  display->setTextSize(1);
  display->setCursor(8, y + 4);
  display->println(text);

  // Arrow untuk submenu
  if (hasSubmenu) {
    display->fillTriangle(115, y + 8, 119, y + 5, 119, y + 11,
                          selected ? ST77XX_BLACK : COLOR_ACCENT);
  }
}

void ControlMenuSystem::drawValueBar(int x, int y, int w, int h, float value,
                                     float maxVal, uint16_t color) {
  if (!display)
    return; // Safety check

  display->drawRect(x, y, w, h, COLOR_TEXT);
  int fillWidth = (int)((value / maxVal) * (w - 2));
  fillWidth = constrain(fillWidth, 0, w - 2);
  display->fillRect(x + 1, y + 1, fillWidth, h - 2, color);
}

void ControlMenuSystem::drawGraph(int x, int y, int w, int h, float *data,
                                  int dataSize, float maxVal) {
  // Draw frame
  display->drawRect(x, y, w, h, COLOR_TEXT);

  // Draw grid lines (horizontal)
  for (int i = 1; i < 4; i++) {
    int gridY = y + (h * i) / 4;
    for (int gx = x + 2; gx < x + w - 2; gx += 3) {
      display->drawPixel(gx, gridY, COLOR_GRAPH_GRID);
    }
  }

  // Draw data points
  for (int i = 1; i < dataSize && i < w - 2; i++) {
    float val1 = data[i - 1];
    float val2 = data[i];

    int y1 = y + h - 2 - (int)((val1 / maxVal) * (h - 4));
    int y2 = y + h - 2 - (int)((val2 / maxVal) * (h - 4));

    y1 = constrain(y1, y + 1, y + h - 2);
    y2 = constrain(y2, y + 1, y + h - 2);

    display->drawLine(x + i, y1, x + i + 1, y2, COLOR_GRAPH_LINE);
  }
}

void ControlMenuSystem::drawStatusIndicator(int x, int y, bool connected) {
  if (connected) {
    display->fillCircle(x, y, 3, COLOR_SUCCESS);
  } else {
    display->fillCircle(x, y, 3, COLOR_ERROR);
  }
}

void ControlMenuSystem::drawPIDEditor(PIDParams &pid, const char *title) {
  display->fillScreen(COLOR_BG);
  drawHeader(title);

  const char *labels[] = {"Kp:", "Ki:", "Kd:", "SP:"};
  float *values[] = {&pid.kp, &pid.ki, &pid.kd, &pid.setPoint};

  for (int i = 0; i < 4; i++) {
    int y = 23 + (i * 27);
    bool selected = (i == pid.editingParam);

    // Label
    display->setTextColor(COLOR_TEXT);
    display->setTextSize(1);
    display->setCursor(5, y);
    display->println(labels[i]);

    // Value box (lebih lebar untuk vertical)
    if (selected) {
      display->fillRoundRect(30, y - 2, 92, 14, 3,
                             isEditing ? COLOR_EDITING : COLOR_SELECTED);
      display->setTextColor(ST77XX_BLACK);
    } else {
      display->drawRoundRect(30, y - 2, 92, 14, 3, COLOR_TEXT);
      display->setTextColor(COLOR_ACCENT);
    }

    display->setCursor(35, y);
    display->print(*values[i], 2);
  }

  drawFooter(isEditing ? "Edit|OK" : "Sel|Edit");
}

// ========================================
// MENU DRAWING FUNCTIONS
// ========================================

void ControlMenuSystem::drawRootMenu() {
  display->fillScreen(COLOR_BG);
  drawHeader("MAIN MENU", false);

  const char *items[] = {"Escalator Ctrl", "Motor AC Ctrl", "WiFi & Status"};

  for (int i = 0; i < 3; i++) {
    drawMenuItem(25 + (i * 30), items[i], i == selectedIndex, true);
  }

  drawFooter("Sel | Enter");
}

void ControlMenuSystem::drawEscalatorMenu() {
  if (!display)
    return; // Safety check

  display->fillScreen(COLOR_BG);
  drawHeader("ESCALATOR");

  const char *items[] = {"Speed Graph", "PID Settings", "Control Mode"};

  for (int i = 0; i < 3; i++) {
    drawMenuItem(25 + (i * 28), items[i], i == selectedIndex, true);
  }

  // Status info (lebih ke bawah)
  display->setTextSize(1);
  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 115);
  display->print("Speed:");
  display->setTextColor(COLOR_ACCENT);
  display->print(escalatorData.currentSpeed, 1);

  drawStatusIndicator(115, 117, escalatorData.isConnected);

  drawFooter("Sel | Enter");
}

void ControlMenuSystem::drawEscalatorGraph() {
  if (!display)
    return; // Safety check

  // Full redraw if needed (menu change)
  if (needFullRedraw) {
    display->fillScreen(COLOR_BG);
    drawHeader("ESC SPEED");
    drawFooter("Sel:Ctrl | Long: Back");
    needFullRedraw = false;
  }

  // Always redraw graph area (clear and redraw)
  display->fillRect(10, 22, 108, 75, COLOR_BG);
  drawGraph(10, 22, 108, 75, escalatorData.speedHistory,
            EscalatorData::GRAPH_BUFFER_SIZE, 100.0);

  // Only update text if value changed
  if (escalatorData.currentSpeed != prevEscalatorSpeed) {
    // Clear old text area
    display->fillRect(5, 102, 105, 10, COLOR_BG);

    // Current value
    display->setTextSize(1);
    display->setTextColor(COLOR_TEXT);
    display->setCursor(5, 102);
    display->print("Cur:");
    display->setTextColor(COLOR_ACCENT);
    display->print(escalatorData.currentSpeed, 1);

    prevEscalatorSpeed = escalatorData.currentSpeed;
  }

  // Only update target if changed
  if (escalatorData.targetSpeed != prevEscalatorTarget) {
    // Clear old text area
    display->fillRect(5, 115, 105, 10, COLOR_BG);

    // Target value
    display->setTextColor(COLOR_TEXT);
    display->setCursor(5, 115);
    display->print("Tgt:");
    display->setTextColor(COLOR_SUCCESS);
    display->print(escalatorData.targetSpeed, 1);

    prevEscalatorTarget = escalatorData.targetSpeed;
  }

  // Connection status
  drawStatusIndicator(115, 108, escalatorData.isConnected);
}

void ControlMenuSystem::drawEscalatorControl() {
  if (!display)
    return; // Safety check

  display->fillScreen(COLOR_BG);
  drawHeader("ESC CONTROL");

  if (escalatorData.mode == MODE_MANUAL) {
    // Manual: Target Speed, Direction, On/Off
    char buf[32];
    snprintf(buf, sizeof(buf), "Tgt Spd: %d%%", (int)escalatorData.targetSpeed);
    drawMenuItem(25, buf, selectedIndex == 0, false);

    const char *dirStr =
        (escalatorData.direction == DIR_FORWARD) ? "FWD" : "REV";
    char buf2[32];
    snprintf(buf2, sizeof(buf2), "Dir: %s", dirStr);
    drawMenuItem(55, buf2, selectedIndex == 1, false);

    const char *runStr = escalatorData.isRunning ? "ON" : "OFF";
    char buf3[32];
    snprintf(buf3, sizeof(buf3), "Run: %s", runStr);
    drawMenuItem(85, buf3, selectedIndex == 2, false);

    // If editing speed, show value bar
    if (isEditing && selectedIndex == 0) {
      drawValueBar(10, 115, 108, 10, escalatorData.targetSpeed, 100.0,
                   COLOR_ACCENT);
      drawFooter("Edit|OK");
    } else {
      drawFooter("Sel|Change");
    }
  } else {
    // PID mode -> only On/Off
    const char *runStr = escalatorData.isRunning ? "ON" : "OFF";
    char buf3[32];
    snprintf(buf3, sizeof(buf3), "Run: %s", runStr);
    drawMenuItem(35, buf3, selectedIndex == 0, false);
    drawFooter("Sel|Toggle");
  }
}

void ControlMenuSystem::drawEscalatorPID() {
  drawPIDEditor(escalatorData.pid, "ESC PID CONFIG");
}

void ControlMenuSystem::drawEscalatorMode() {
  display->fillScreen(COLOR_BG);
  drawHeader("ESC MODE");

  // Mode selection
  drawMenuItem(25, "Manual Mode", selectedIndex == 0, false);
  drawMenuItem(55, "Auto (PID)", selectedIndex == 1, false);

  // Current mode indicator
  display->setTextSize(1);
  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 90);
  display->print("Active:");
  display->setTextColor(COLOR_SUCCESS);
  display->print(escalatorData.mode == MODE_MANUAL ? "MANUAL" : "AUTO");

  if (escalatorData.mode == MODE_MANUAL) {
    // Manual controls
    display->setTextColor(COLOR_TEXT);
    display->setCursor(5, 105);
    display->print("Speed:");
    display->setTextColor(COLOR_ACCENT);
    display->print(escalatorData.targetSpeed, 0);
    display->print("%");

    display->setTextColor(COLOR_TEXT);
    display->setCursor(5, 120);
    display->print("Dir:");
    display->setTextColor(
        escalatorData.direction == DIR_FORWARD ? COLOR_SUCCESS : COLOR_WARNING);
    display->print(escalatorData.direction == DIR_FORWARD ? "FWD" : "REV");
  }

  drawFooter("Sel | Change");
}

void ControlMenuSystem::drawMotorACMenu() {
  display->fillScreen(COLOR_BG);
  drawHeader("MOTOR AC");

  const char *items[] = {"Speed Graph", "Control Type", "Voltage",
                         "PID Settings", "Control Mode"};

  int itemsToShow = min(4, 5 - selectedIndex); // Show max 4 items
  for (int i = 0; i < itemsToShow; i++) {
    drawMenuItem(25 + (i * 22), items[selectedIndex + i], i == 0, true);
  }

  // Status (lebih ke bawah)
  display->setTextSize(1);
  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 120);
  display->print("Spd:");
  display->setTextColor(COLOR_ACCENT);
  display->print(motorACData.currentSpeed, 0);

  drawStatusIndicator(115, 122, motorACData.isConnected);

  drawFooter("Sel | Enter");
}

void ControlMenuSystem::drawMotorGraph() {
  display->fillScreen(COLOR_BG);
  drawHeader("MOTOR SPD");

  drawGraph(10, 22, 108, 75, motorACData.speedHistory,
            MotorACData::GRAPH_BUFFER_SIZE, 3000.0);

  display->setTextSize(1);
  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 102);
  display->print("Cur:");
  display->setTextColor(COLOR_ACCENT);
  display->print(motorACData.currentSpeed, 0);

  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 115);
  display->print("Tgt:");
  display->setTextColor(COLOR_SUCCESS);
  display->print(motorACData.targetSpeed, 0);

  drawStatusIndicator(115, 108, motorACData.isConnected);

  drawFooter("Long: Back");
}

void ControlMenuSystem::drawMotorControl() {
  display->fillScreen(COLOR_BG);
  drawHeader("CTRL TYPE");

  drawMenuItem(25, "DAC Control", selectedIndex == 0, false);
  drawMenuItem(53, "PWM Control", selectedIndex == 1, false);
  drawMenuItem(81, "Modbus Ctrl", selectedIndex == 2, false);

  // Current selection indicator
  display->setTextSize(1);
  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 115);
  display->print("Active:");
  display->setTextColor(COLOR_SUCCESS);
  const char *ctrlTypes[] = {"DAC", "PWM", "MODBUS"};
  display->print(ctrlTypes[motorACData.controlType]);

  drawFooter("Sel | Apply");
}

void ControlMenuSystem::drawMotorVoltage() {
  display->fillScreen(COLOR_BG);
  drawHeader("VOLTAGE");

  drawMenuItem(35, "5V Output", selectedIndex == 0, false);
  drawMenuItem(70, "10V Output", selectedIndex == 1, false);

  display->setTextSize(1);
  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 110);
  display->print("Active:");
  display->setTextColor(COLOR_SUCCESS);
  display->print(motorACData.voltageType == VOLTAGE_5V ? "5V" : "10V");

  drawFooter("Sel | Apply");
}

void ControlMenuSystem::drawMotorPID() {
  drawPIDEditor(motorACData.pid, "MOTOR PID CONFIG");
}

void ControlMenuSystem::drawMotorMode() {
  display->fillScreen(COLOR_BG);
  drawHeader("MOTOR MODE");

  drawMenuItem(25, "Manual Mode", selectedIndex == 0, false);
  drawMenuItem(55, "Auto (PID)", selectedIndex == 1, false);

  display->setTextSize(1);
  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 90);
  display->print("Active:");
  display->setTextColor(COLOR_SUCCESS);
  display->print(motorACData.mode == MODE_MANUAL ? "MANUAL" : "AUTO");

  if (motorACData.mode == MODE_MANUAL) {
    display->setTextColor(COLOR_TEXT);
    display->setCursor(5, 108);
    display->print("Speed:");
    display->setTextColor(COLOR_ACCENT);
    display->print(motorACData.targetSpeed, 0);
  }

  drawFooter("Sel | Change");
}

void ControlMenuSystem::drawWiFiStatus() {
  display->fillScreen(COLOR_BG);
  drawHeader("NETWORK");

  // WiFi Status
  display->setTextSize(1);
  display->setTextColor(COLOR_ACCENT);
  display->setCursor(5, 23);
  display->println("WiFi:");

  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 35);
  display->print("SSID:");
  display->setTextColor(networkStatus.wifiConnected ? COLOR_SUCCESS
                                                    : COLOR_ERROR);
  display->setCursor(5, 46);
  String ssidStr = networkStatus.wifiConnected ? networkStatus.ssid : "Disc";
  if (ssidStr.length() > 20)
    ssidStr = ssidStr.substring(0, 17) + "...";
  display->println(ssidStr);

  if (networkStatus.wifiConnected) {
    display->setTextColor(COLOR_TEXT);
    display->setCursor(5, 58);
    display->print("IP:");
    display->setTextColor(COLOR_ACCENT);
    display->setCursor(5, 69);
    String ipStr = networkStatus.ipAddress;
    if (ipStr.length() > 20)
      ipStr = ipStr.substring(0, 17) + "...";
    display->println(ipStr);

    display->setTextColor(COLOR_TEXT);
    display->setCursor(5, 81);
    display->print("Clients: ");
    display->setTextColor(COLOR_SUCCESS);
    display->print(networkStatus.clientCount);
  }

  drawStatusIndicator(115, 35, networkStatus.wifiConnected);

  // ESP-NOW Status
  display->setTextColor(COLOR_ACCENT);
  display->setCursor(5, 98);
  display->println("ESP-NOW:");

  display->setTextColor(COLOR_TEXT);
  display->setCursor(5, 110);
  display->print("Stat: ");
  display->setTextColor(networkStatus.espnowInitialized ? COLOR_SUCCESS
                                                        : COLOR_ERROR);
  display->print(networkStatus.espnowInitialized ? "OK" : "FAIL");

  display->setTextColor(COLOR_TEXT);
  display->setCursor(68, 110);
  display->print("P:");
  display->setTextColor(COLOR_ACCENT);
  display->print(networkStatus.peerCount);

  drawStatusIndicator(115, 110, networkStatus.espnowInitialized);

  drawFooter("Long: Back");
}

// ========================================
// NAVIGATION FUNCTIONS
// ========================================

void ControlMenuSystem::navigate(int direction) {
  if (isEditing) {
    // Mode editing - ubah nilai
    switch (currentMenu) {
    case MENU_ESCALATOR_PID: {
      float *val = nullptr;
      float increment = 0.1;

      switch (escalatorData.pid.editingParam) {
      case 0:
        val = &escalatorData.pid.kp;
        break;
      case 1:
        val = &escalatorData.pid.ki;
        break;
      case 2:
        val = &escalatorData.pid.kd;
        break;
      case 3:
        val = &escalatorData.pid.setPoint;
        increment = 1.0;
        break;
      }

      if (val) {
        *val += direction * increment;
        *val = max(0.0f, *val);
        drawEscalatorPID();
      }
      break;
    }

    case MENU_MOTOR_PID: {
      float *val = nullptr;
      float increment = 0.1;

      switch (motorACData.pid.editingParam) {
      case 0:
        val = &motorACData.pid.kp;
        break;
      case 1:
        val = &motorACData.pid.ki;
        break;
      case 2:
        val = &motorACData.pid.kd;
        break;
      case 3:
        val = &motorACData.pid.setPoint;
        increment = 10.0;
        break;
      }

      if (val) {
        *val += direction * increment;
        *val = max(0.0f, *val);
        drawMotorPID();
      }
      break;
    }

    case MENU_ESCALATOR_MODE: {
      if (escalatorData.mode == MODE_MANUAL && selectedIndex == 0) {
        escalatorData.targetSpeed += direction * 5;
        escalatorData.targetSpeed =
            constrain(escalatorData.targetSpeed, 0, 100);
        drawEscalatorMode();
      }
      break;
    }

    case MENU_ESCALATOR_CONTROL: {
      // If editing target speed
      if (selectedIndex == 0 && escalatorData.mode == MODE_MANUAL) {
        // Invert direction and use smaller step (1% instead of 5%)
        escalatorData.targetSpeed += (-direction) * 1;
        escalatorData.targetSpeed =
            constrain(escalatorData.targetSpeed, 0, 100);
        drawEscalatorControl();
      }
      break;
    }

    case MENU_MOTOR_MODE: {
      if (motorACData.mode == MODE_MANUAL && selectedIndex == 0) {
        motorACData.targetSpeed += direction * 50;
        motorACData.targetSpeed = constrain(motorACData.targetSpeed, 0, 3000);
        drawMotorMode();
      }
      break;
    }

    default:
      break;
    }
  } else {
    // Mode navigasi - pindah menu
    int maxItems = 0;

    switch (currentMenu) {
    case MENU_ROOT:
      maxItems = 3;
      break;
    case MENU_ESCALATOR:
      maxItems = 3;
      break;
    case MENU_MOTOR_AC:
      maxItems = 5;
      break;
    case MENU_ESCALATOR_PID:
    case MENU_MOTOR_PID:
      maxItems = 4;
      break;
    case MENU_ESCALATOR_MODE:
    case MENU_MOTOR_MODE:
      maxItems = 2;
      break;
    case MENU_ESCALATOR_CONTROL:
      maxItems = (escalatorData.mode == MODE_MANUAL) ? 3 : 1;
      break;
    case MENU_MOTOR_CONTROL:
      maxItems = 3;
      break;
    case MENU_MOTOR_VOLTAGE:
      maxItems = 2;
      break;
    default:
      return;
    }

    selectedIndex += direction;

    // Wrap around
    if (selectedIndex < 0)
      selectedIndex = maxItems - 1;
    if (selectedIndex >= maxItems)
      selectedIndex = 0;

    // Redraw menu
    switch (currentMenu) {
    case MENU_ROOT:
      drawRootMenu();
      break;
    case MENU_ESCALATOR:
      drawEscalatorMenu();
      break;
    case MENU_ESCALATOR_PID:
      drawEscalatorPID();
      break;
    case MENU_ESCALATOR_MODE:
      drawEscalatorMode();
      break;
    case MENU_ESCALATOR_CONTROL:
      drawEscalatorControl();
      break;
    case MENU_MOTOR_AC:
      drawMotorACMenu();
      break;
    case MENU_MOTOR_CONTROL:
      drawMotorControl();
      break;
    case MENU_MOTOR_VOLTAGE:
      drawMotorVoltage();
      break;
    case MENU_MOTOR_PID:
      drawMotorPID();
      break;
    case MENU_MOTOR_MODE:
      drawMotorMode();
      break;
    default:
      break;
    }
  }
}

void ControlMenuSystem::select() {
  if (currentMenu == MENU_ESCALATOR_PID || currentMenu == MENU_MOTOR_PID) {
    if (isEditing) {
      isEditing = false; // Keluar dari mode edit
    } else {
      // Pilih parameter untuk diedit
      PIDParams &pid = (currentMenu == MENU_ESCALATOR_PID) ? escalatorData.pid
                                                           : motorACData.pid;
      pid.editingParam = selectedIndex;
      isEditing = true;
    }

    if (currentMenu == MENU_ESCALATOR_PID)
      drawEscalatorPID();
    else
      drawMotorPID();
    return;
  }

  switch (currentMenu) {
  case MENU_ROOT:
    // Gunakan selectedIndex yang sudah dipilih user, jangan di-reset!
    switch (selectedIndex) {
    case 0:
      currentMenu = MENU_ESCALATOR;
      selectedIndex = 0; // Reset untuk submenu
      drawEscalatorMenu();
      break;
    case 1:
      currentMenu = MENU_MOTOR_AC;
      selectedIndex = 0; // Reset untuk submenu
      drawMotorACMenu();
      break;
    case 2:
      currentMenu = MENU_WIFI_STATUS;
      selectedIndex = 0; // Reset untuk submenu
      drawWiFiStatus();
      break;
    }
    break;

  case MENU_ESCALATOR:
    switch (selectedIndex) {
    case 0:
      currentMenu = MENU_ESCALATOR_GRAPH;
      needFullRedraw = true; // Force full redraw on menu change
      // Enable streaming when entering graph view
      if (networkManager) {
        EscalatorCommandPacket cmd = {};
        cmd.commandType = 5; // ENABLE_STREAMING
        networkManager->sendEscalatorCommand(cmd);
      }
      drawEscalatorGraph();
      break;
    case 1:
      currentMenu = MENU_ESCALATOR_PID;
      selectedIndex = 0;
      needFullRedraw = true;
      drawEscalatorPID();
      break;
    case 2:
      currentMenu = MENU_ESCALATOR_MODE;
      selectedIndex = 0;
      needFullRedraw = true;
      drawEscalatorMode();
      break;
    }
    break;

  case MENU_ESCALATOR_MODE:
    escalatorData.mode = (selectedIndex == 0) ? MODE_MANUAL : MODE_AUTO;
    drawEscalatorMode();
    break;

  case MENU_ESCALATOR_GRAPH:
    // Open control submenu when selecting graph
    currentMenu = MENU_ESCALATOR_CONTROL;
    // Default selected index depending on mode
    selectedIndex = 0;
    isEditing = false;
    needFullRedraw = true; // Force full redraw
    drawEscalatorControl();
    break;

  case MENU_ESCALATOR_CONTROL: {
    // Handle actions in control menu
    if (escalatorData.mode == MODE_MANUAL) {
      switch (selectedIndex) {
      case 0: // Target Speed
        if (isEditing) {
          // Finish editing and send speed command
          isEditing = false;
          if (networkManager) {
            EscalatorCommandPacket cmd = {};
            cmd.commandType = 2; // SET_SPEED
            cmd.targetSpeed = escalatorData.targetSpeed;
            cmd.mode = escalatorData.mode == MODE_MANUAL ? 0 : 1;
            networkManager->sendEscalatorCommand(cmd);
          }
        } else {
          // Start editing
          isEditing = true;
        }
        drawEscalatorControl();
        break;

      case 1: // Direction
        // Toggle direction and send
        escalatorData.direction = (escalatorData.direction == DIR_FORWARD)
                                      ? DIR_REVERSE
                                      : DIR_FORWARD;
        if (networkManager) {
          EscalatorCommandPacket cmd = {};
          cmd.commandType = 3; // SET_DIRECTION
          cmd.direction = (escalatorData.direction == DIR_FORWARD)
                              ? 1
                              : 0; // follow Plant expectation
          networkManager->sendEscalatorCommand(cmd);
        }
        drawEscalatorControl();
        break;

      case 2: // Run On/Off
        escalatorData.isRunning = !escalatorData.isRunning;
        if (networkManager) {
          EscalatorCommandPacket cmd = {};
          cmd.commandType = escalatorData.isRunning ? 1 : 0; // START : STOP
          networkManager->sendEscalatorCommand(cmd);
        }
        drawEscalatorControl();
        break;
      }
    } else {
      // PID mode - only On/Off (selectedIndex 0)
      escalatorData.isRunning = !escalatorData.isRunning;
      if (networkManager) {
        EscalatorCommandPacket cmd = {};
        cmd.commandType = escalatorData.isRunning ? 1 : 0; // START : STOP
        networkManager->sendEscalatorCommand(cmd);
      }
      drawEscalatorControl();
    }

    break;
  }

  case MENU_MOTOR_AC:
    switch (selectedIndex) {
    case 0:
      currentMenu = MENU_MOTOR_GRAPH;
      drawMotorGraph();
      break;
    case 1:
      currentMenu = MENU_MOTOR_CONTROL;
      selectedIndex = motorACData.controlType;
      drawMotorControl();
      break;
    case 2:
      currentMenu = MENU_MOTOR_VOLTAGE;
      selectedIndex = motorACData.voltageType;
      drawMotorVoltage();
      break;
    case 3:
      currentMenu = MENU_MOTOR_PID;
      selectedIndex = 0;
      drawMotorPID();
      break;
    case 4:
      currentMenu = MENU_MOTOR_MODE;
      selectedIndex = 0;
      drawMotorMode();
      break;
    }
    break;

  case MENU_MOTOR_CONTROL:
    motorACData.controlType = (MotorControlType)selectedIndex;
    drawMotorControl();
    break;

  case MENU_MOTOR_VOLTAGE:
    motorACData.voltageType = (VoltageType)selectedIndex;
    drawMotorVoltage();
    break;

  case MENU_MOTOR_MODE:
    motorACData.mode = (selectedIndex == 0) ? MODE_MANUAL : MODE_AUTO;
    drawMotorMode();
    break;

  default:
    break;
  }
}

void ControlMenuSystem::back() {
  if (isEditing) {
    isEditing = false;
    if (currentMenu == MENU_ESCALATOR_PID)
      drawEscalatorPID();
    else if (currentMenu == MENU_MOTOR_PID)
      drawMotorPID();
    return;
  }

  selectedIndex = 0;

  switch (currentMenu) {
  case MENU_ROOT:
    // Already at root, do nothing
    break;

  case MENU_ESCALATOR:
  case MENU_MOTOR_AC:
  case MENU_WIFI_STATUS:
    currentMenu = MENU_ROOT;
    drawRootMenu();
    break;

  case MENU_ESCALATOR_GRAPH:
    // Disable streaming when leaving graph view
    if (networkManager) {
      EscalatorCommandPacket cmd = {};
      cmd.commandType = 6; // DISABLE_STREAMING
      networkManager->sendEscalatorCommand(cmd);
    }
    currentMenu = MENU_ESCALATOR;
    drawEscalatorMenu();
    break;

  case MENU_ESCALATOR_PID:
  case MENU_ESCALATOR_MODE:
    currentMenu = MENU_ESCALATOR;
    drawEscalatorMenu();
    break;

  case MENU_ESCALATOR_CONTROL:
    // Back from control goes to graph
    currentMenu = MENU_ESCALATOR_GRAPH;
    needFullRedraw = true; // Force full redraw
    drawEscalatorGraph();
    break;

  case MENU_MOTOR_GRAPH:
  case MENU_MOTOR_CONTROL:
  case MENU_MOTOR_VOLTAGE:
  case MENU_MOTOR_PID:
  case MENU_MOTOR_MODE:
    currentMenu = MENU_MOTOR_AC;
    drawMotorACMenu();
    break;

  default:
    break;
  }
}

// ========================================
// UPDATE FUNCTIONS
// ========================================

void ControlMenuSystem::updateEscalatorData(float speed, Direction dir) {
  escalatorData.currentSpeed = speed;
  escalatorData.direction = dir;
  escalatorData.isConnected = true;
  escalatorData.lastUpdate = millis();

  // Update history buffer
  escalatorData.speedHistory[escalatorData.historyIndex] = speed;
  escalatorData.historyIndex =
      (escalatorData.historyIndex + 1) % EscalatorData::GRAPH_BUFFER_SIZE;
}

void ControlMenuSystem::updateEscalatorFromPacket(EscalatorDataPacket pkt) {
  // Map packet fields to local state
  escalatorData.currentSpeed = pkt.currentSpeed;
  // Note: EscalatorDataPacket doesn't have targetSpeed, keep existing value
  escalatorData.direction = (pkt.direction == 1) ? DIR_FORWARD : DIR_REVERSE;
  escalatorData.mode = (pkt.mode == 1) ? MODE_AUTO : MODE_MANUAL;
  escalatorData.isRunning = pkt.isRunning;
  escalatorData.isConnected = true;
  escalatorData.lastUpdate = millis();

  // Update history
  escalatorData.speedHistory[escalatorData.historyIndex] = pkt.currentSpeed;
  escalatorData.historyIndex =
      (escalatorData.historyIndex + 1) % EscalatorData::GRAPH_BUFFER_SIZE;

  // DON'T auto-redraw here - let the periodic update() function handle it
  // This prevents crashes from too many redraws during ESP-NOW callbacks
}

void ControlMenuSystem::updateMotorACData(float speed) {
  motorACData.currentSpeed = speed;
  motorACData.isConnected = true;
  motorACData.lastUpdate = millis();

  motorACData.speedHistory[motorACData.historyIndex] = speed;
  motorACData.historyIndex =
      (motorACData.historyIndex + 1) % MotorACData::GRAPH_BUFFER_SIZE;
}

void ControlMenuSystem::updateNetworkStatus(bool wifiConn, String ssid,
                                            String ip, int clients) {
  networkStatus.wifiConnected = wifiConn;
  networkStatus.ssid = ssid;
  networkStatus.ipAddress = ip;
  networkStatus.clientCount = clients;
}

void ControlMenuSystem::updateESPNowStatus(bool initialized, int peers) {
  networkStatus.espnowInitialized = initialized;
  networkStatus.peerCount = peers;
}

void ControlMenuSystem::update() {
  // Check connection timeout
  if (millis() - escalatorData.lastUpdate > 3000) {
    escalatorData.isConnected = false;
  }

  if (millis() - motorACData.lastUpdate > 3000) {
    motorACData.isConnected = false;
  }

  // Update graph displays if currently viewing (periodic update)
  if (currentMenu == MENU_ESCALATOR_GRAPH &&
      millis() - lastGraphUpdate > graphUpdateInterval) {
    drawEscalatorGraph();
    lastGraphUpdate = millis();
  }

  // Control menu: only update when editing (not continuous)
  // When not editing, no need to refresh
  if (currentMenu == MENU_ESCALATOR_CONTROL && isEditing &&
      millis() - lastGraphUpdate > graphUpdateInterval) {
    drawEscalatorControl();
    lastGraphUpdate = millis();
  }

  if (currentMenu == MENU_MOTOR_GRAPH &&
      millis() - lastGraphUpdate > graphUpdateInterval) {
    drawMotorGraph();
    lastGraphUpdate = millis();
  }
}

void ControlMenuSystem::setNetworkManager(NetworkManager *nm) {
  networkManager = nm;
}
