#include "ControlMenuSystem.h"
#include "NetworkManager.h"
#include <Arduino.h>

// Forward declarations

// ========================================
// DEFINISI VARIABEL GLOBAL
// ========================================

// Display
Adafruit_ST7735 *g_display = nullptr;

// Menu state
int g_currentMenu = 0;
int g_selectedIndex = 0;
int g_escSubMenu = 0;   // 0=graph, 1=control
int g_motorSubMenu = 0; // 0=graph, 1=control
bool g_isEditing = false;
bool espconnected = false;
// Escalator data
float g_escSpeed = 0;
float g_escTarget = 0;
int g_escDirection = 0;
int g_escMode = 0;
bool g_escRunning = true;
bool g_escConnected = false;
float g_escSpeedHistory[100] = {0};
int g_escHistoryIndex = 0;
bool g_pidMode = false;

// PID control values
float g_dcKp = 1.5, g_dcKi = 0.1, g_dcKd = 0.05, g_dcSetpoint = 50.0;

// Motor data
float g_motorSpeed = 0;
float g_motorTarget = 0;
int g_motorMode = 0;
bool g_motorRunning = false;
bool g_motorConnected = false;
float g_motorSpeedHistory[100] = {0};
int g_motorHistoryIndex = 0;
bool g_acPidMode = false;

// AC Motor PID control values
float g_acKp = 2.0, g_acKi = 0.08, g_acKd = 0.04, g_acSetpoint = 0.0;

// Flags for deferred graph updates (to avoid drawing from ISR/callback)
volatile bool g_escSpeedUpdated = false;
volatile bool g_motorSpeedUpdated = false;
volatile bool g_escStatusChanged = false;
volatile bool g_motorStatusChanged = false;

// Buffers untuk incremental pixel redraw (oscilloscope style)
static int escGraphY[116];
static bool escGraphInit = false;
static int motorGraphY[116];
static bool motorGraphInit = false;

// PID values
float g_escKp = 1.0, g_escKi = 0.1, g_escKd = 0.05, g_escSetpoint = 50.0;
float g_motorKp = 2.0, g_motorKi = 0.5, g_motorKd = 0.1,
      g_motorSetpoint = 1000.0;

// Network status
bool g_wifiConnected = false;
bool g_espnowInit = false;

// Timing
unsigned long g_lastUpdate = 0;

// Colors
uint16_t COLOR_BG, COLOR_HEADER, COLOR_SELECTED, COLOR_TEXT, COLOR_ACCENT;

// ========================================
// FUNGSI HELPER
// ========================================

int getCenterX(const char *text) {
  int16_t x1, y1;
  uint16_t w, h;
  g_display->setTextSize(1);
  g_display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  return (128 - w) / 2;
}

void drawHeader(const char *title) {
  g_display->fillRect(0, 0, 128, 18, COLOR_HEADER);
  g_display->setTextColor(ST77XX_WHITE);
  g_display->setTextSize(1);
  g_display->setCursor(getCenterX(title), 5);
  g_display->println(title);
}

void drawFooter(const char *hint) {
  g_display->fillRect(0, 150, 128, 10, COLOR_HEADER);
  g_display->setTextColor(ST77XX_WHITE);
  g_display->setTextSize(1);
  g_display->setCursor(2, 152);
  g_display->print(hint);
}

void drawMenuItem(int y, const char *text, bool selected) {
  if (selected) {
    g_display->fillRoundRect(3, y, 122, 16, 3, COLOR_SELECTED);
    g_display->setTextColor(ST77XX_BLACK);
  } else {
    g_display->drawRoundRect(3, y, 122, 16, 3, COLOR_TEXT);
    g_display->setTextColor(COLOR_TEXT);
  }
  g_display->setTextSize(1);
  g_display->setCursor(8, y + 4);
  g_display->println(text);
}

// ========================================
// INISIALISASI
// ========================================

void menuInit(Adafruit_ST7735 *tft) {
  g_display = tft;

  // Setup colors
  COLOR_BG = g_display->color565(0, 0, 20);
  COLOR_HEADER = g_display->color565(0, 80, 160);
  COLOR_SELECTED = g_display->color565(50, 150, 255);
  COLOR_TEXT = g_display->color565(255, 255, 255);
  COLOR_ACCENT = g_display->color565(255, 200, 0);

  g_display->fillScreen(COLOR_BG);
  showSplash();
  delay(2000);
  drawCurrentMenu();
}

void showSplash() {
  g_display->fillScreen(ST77XX_BLACK);
  g_display->setTextColor(COLOR_ACCENT);
  g_display->setTextSize(2);
  g_display->setCursor(20, 50);
  g_display->println("Control");
  g_display->setCursor(20, 70);
  g_display->println("System");

  g_display->setTextSize(1);
  g_display->setCursor(getCenterX("v1.0"), 120);
  g_display->println("v1.0");
}

// ========================================
// DRAWING MENUS
// ========================================

void drawRootMenu() {
  g_display->fillScreen(COLOR_BG);
  drawHeader("MAIN MENU");
  sendTaggedFloat(MSG_DC_Control, 0);
  sendTaggedFloat(MSG_AC_Control, 0);

  drawMenuItem(30, "Escalator", g_selectedIndex == 0);
  drawMenuItem(60, "Motor AC", g_selectedIndex == 1);
  drawMenuItem(90, "WiFi Status", g_selectedIndex == 2);

  drawFooter("1x:SEL 2x:DIR Lx:BACK");
}

void drawEscGraph(bool fullRedraw = false) {
  const int leftX = 6;
  const int innerWidth = 116; // x:6..121
  const int rightX = leftX + innerWidth - 1;
  const int maxSamples = 100;
  g_display->drawRect(5, 25, 118, 70, COLOR_TEXT);

  if (fullRedraw || !escGraphInit) {
    // Full redraw & buffer init
    g_display->fillRect(5, 25, 118, 70, ST77XX_BLACK);
    g_display->drawRect(5, 25, 118, 70, COLOR_TEXT);
    int setpointY = 94 - (int)(constrain(g_dcSetpoint, 0, ESC_GRAPH_MAX) * 69 /
                               ESC_GRAPH_MAX);
    for (int x = leftX; x <= rightX; x += 3) {
      g_display->drawPixel(x, setpointY, ST77XX_RED);
    }
    for (int col = 0; col < innerWidth; col++) {
      float pos = ((float)col * (maxSamples - 1)) / (innerWidth - 1);
      int off = (int)(pos + 0.5f);
      int idx = (g_escHistoryIndex - 1 - off + maxSamples) % maxSamples;
      float v = constrain(g_escSpeedHistory[idx], 0, ESC_GRAPH_MAX);
      int y = 94 - (int)(v * 69 / ESC_GRAPH_MAX);
      int x = rightX - col;
      escGraphY[col] = y; // simpan sesuai kolom kanan->kiri (col=0 newest)
      g_display->drawPixel(x, y, COLOR_ACCENT);
    }
    escGraphInit = true;
    // Label & info
    g_display->setTextSize(1);
    g_display->setTextColor(ST77XX_CYAN);
    g_display->setCursor(2, 26);
    g_display->print((int)ESC_GRAPH_MAX);
    g_display->setCursor(2, 88);
    g_display->print("0");
    g_display->fillRect(0, 100, 128, 28, COLOR_BG);
    g_display->setTextColor(COLOR_TEXT);
    g_display->setCursor(5, 105);
    g_display->print("Spd:");
    g_display->setTextColor(COLOR_ACCENT);
    g_display->print(g_escSpeed, 0);
    g_display->setTextColor(ST77XX_RED);
    g_display->setCursor(5, 115);
    g_display->print("SP:");
    g_display->print(g_dcSetpoint, 0);
    g_display->setTextColor(ST77XX_CYAN);
    g_display->setCursor(70, 105);
    g_display->print("Y:rpm");
    g_display->setCursor(70, 115);
    g_display->print("X:ms");
    g_display->setCursor(5, 125);
    g_display->setTextColor(COLOR_TEXT);
    g_display->print("M: ");
    g_display->setTextColor(g_escRunning ? ST77XX_GREEN : ST77XX_RED);
    g_display->print(g_escRunning ? "RUN" : "STOP");
    drawFooter("1x:CTL 2x:RUN Lx:BACK");
    return;
  }

  // Incremental scroll: shift buffer left, draw only changed pixels
  // escGraphY[0] newest at rightX, escGraphY[innerWidth-1] oldest at leftX
  // setelah konversi Kita simpan dalam urutan newest->oldest; saat shift kita
  // geser ke indeks lebih tinggi Erase dan redraw per kolom hanya bila y
  // berubah.
  int setpointY = 94 - (int)(constrain(g_dcSetpoint, 0, ESC_GRAPH_MAX) * 69 /
                             ESC_GRAPH_MAX);
  int newIdxVal = (g_escHistoryIndex - 1 + maxSamples) % maxSamples;
  float newV = constrain(g_escSpeedHistory[newIdxVal], 0, ESC_GRAPH_MAX);
  int newY = 94 - (int)(newV * 69 / ESC_GRAPH_MAX);

  // Shift: from oldest backward
  for (int i = innerWidth - 1; i > 0; --i) {
    int prevY = escGraphY[i];
    int srcY = escGraphY[i - 1];
    if (prevY != srcY) {
      int x = rightX - i; // posisi layar untuk kolom i
      // hapus pixel lama
      g_display->drawPixel(x, prevY, ST77XX_BLACK);
      // gambar pixel baru
      g_display->drawPixel(x, srcY, COLOR_ACCENT);
    }
    escGraphY[i] = srcY;
    // redraw setpoint dashed (jika kolom cocok pola)
    if (((rightX - i) - leftX) % 3 == 0) {
      g_display->drawPixel(rightX - i, setpointY, ST77XX_RED);
    }
  }
  // Update newest (index 0) pada paling kanan
  int prevNewestY = escGraphY[0];
  if (prevNewestY != newY) {
    g_display->drawPixel(rightX, prevNewestY, ST77XX_BLACK);
    g_display->drawPixel(rightX, newY, COLOR_ACCENT);
  }
  escGraphY[0] = newY;
  if ((rightX - leftX) % 3 == 0) {
    g_display->drawPixel(rightX, setpointY, ST77XX_RED);
  }

  // Update nilai speed & status motor jika berubah
  g_display->fillRect(24, 105, 35, 8, COLOR_BG);
  g_display->setTextColor(COLOR_ACCENT);
  g_display->setCursor(24, 105);
  g_display->print(g_escSpeed, 0);
  static bool lastRunEsc = false;
  if (lastRunEsc != g_escRunning) {
    g_display->fillRect(20, 125, 40, 8, COLOR_BG);
    g_display->setCursor(20, 125);
    g_display->setTextColor(g_escRunning ? ST77XX_GREEN : ST77XX_RED);
    g_display->print(g_escRunning ? "RUN" : "STOP");
    lastRunEsc = g_escRunning;
  }
}
void drawEscControl(int updateLine = -1) {
  if (updateLine == -1) {
    g_display->fillRect(0, 20, 128, 140, COLOR_BG);
    drawFooter("1x:EDT 2x:DIR Lx:BACK");
  }

  // Added "Dir" option at the end (index 6)
  const char *items[] = {"Run", "Kp", "Ki", "Kd", "Setpt", "PID", "Dir"};
  int start = (updateLine == -1) ? 0 : updateLine;
  int end = (updateLine == -1) ? 7 : updateLine + 1;

  for (int i = start; i < end; i++) {
    int y = 25 + i * 15;
    bool sel = (g_selectedIndex == i);

    if (updateLine != -1) {
      g_display->fillRect(0, y, 128, 10, COLOR_BG);
    }

    g_display->setTextColor(sel ? COLOR_SELECTED : COLOR_TEXT);
    g_display->setCursor(5, y);
    g_display->print(items[i]);
    g_display->print(":");

    g_display->setTextColor(COLOR_ACCENT);
    g_display->setCursor(60, y);
    if (i == 0)
      g_display->print(g_escRunning ? "ON " : "OFF");
    else if (i == 1)
      g_display->print(g_dcKp, 2);
    else if (i == 2)
      g_display->print(g_dcKi, 2);
    else if (i == 3)
      g_display->print(g_dcKd, 2);
    else if (i == 4)
      g_display->print(g_dcSetpoint, 1);
    else if (i == 5)
      g_display->print(g_pidMode ? "ON " : "OFF");
    else if (i == 6)
      g_display->print(g_escDirection ? "REV" : "FWD");
  }
}

void drawEscalatorMenu() {
  g_display->fillScreen(COLOR_BG);
  drawHeader(g_escSubMenu == 0 ? "ESC GRAPH" : "ESC CTRL");
  // sendTaggedFloat(MSG_DC_Control, 1);

  if (g_escSubMenu == 0) {
    drawEscGraph(true);
  } else {
    drawEscControl();
  }
}

void drawMotorGraph(bool fullRedraw = false) {
  const int leftX = 6;
  const int innerWidth = 116;
  const int rightX = leftX + innerWidth - 1;
  const int maxSamples = 100;
  g_display->drawRect(5, 25, 118, 70, COLOR_TEXT);

  if (fullRedraw || !motorGraphInit) {
    g_display->fillRect(5, 25, 118, 70, ST77XX_BLACK);
    g_display->drawRect(5, 25, 118, 70, COLOR_TEXT);
    int setpointY = 94 - (int)(constrain(g_acSetpoint, 0, AC_GRAPH_MAX) * 69 /
                               AC_GRAPH_MAX);
    for (int x = leftX; x <= rightX; x += 3)
      g_display->drawPixel(x, setpointY, ST77XX_RED);
    for (int col = 0; col < innerWidth; col++) {
      float pos = ((float)col * (maxSamples - 1)) / (innerWidth - 1);
      int off = (int)(pos + 0.5f);
      int idx = (g_motorHistoryIndex - 1 - off + maxSamples) % maxSamples;
      float v = constrain(g_motorSpeedHistory[idx], 0, AC_GRAPH_MAX);
      int y = 94 - (int)(v * 69 / AC_GRAPH_MAX);
      motorGraphY[col] = y;
      int x = rightX - col;
      g_display->drawPixel(x, y, COLOR_ACCENT);
    }
    motorGraphInit = true;
    g_display->setTextSize(1);
    g_display->setTextColor(ST77XX_CYAN);
    g_display->setCursor(2, 26);
    g_display->print((int)AC_GRAPH_MAX);
    g_display->setCursor(2, 88);
    g_display->print("0");
    g_display->fillRect(0, 100, 128, 28, COLOR_BG);
    g_display->setTextColor(COLOR_TEXT);
    g_display->setCursor(5, 105);
    g_display->print("Spd:");
    g_display->setTextColor(COLOR_ACCENT);
    g_display->print(g_motorSpeed, 0);
    g_display->setTextColor(ST77XX_RED);
    g_display->setCursor(5, 115);
    g_display->print("SP:");
    g_display->print(g_acSetpoint, 0);
    g_display->setTextColor(ST77XX_CYAN);
    g_display->setCursor(70, 105);
    g_display->print("Y:rpm");
    g_display->setCursor(70, 115);
    g_display->print("X:ms");
    g_display->setCursor(5, 125);
    g_display->setTextColor(COLOR_TEXT);
    g_display->print("M: ");
    g_display->setTextColor(g_motorRunning ? ST77XX_GREEN : ST77XX_RED);
    g_display->print(g_motorRunning ? "RUN" : "STOP");
    drawFooter("1x:CTL 2x:RUN Lx:BACK");
    return;
  }

  int setpointY =
      94 - (int)(constrain(g_acSetpoint, 0, AC_GRAPH_MAX) * 69 / AC_GRAPH_MAX);
  int newIdxVal = (g_motorHistoryIndex - 1 + maxSamples) % maxSamples;
  float newV = constrain(g_motorSpeedHistory[newIdxVal], 0, AC_GRAPH_MAX);
  int newY = 94 - (int)(newV * 69 / AC_GRAPH_MAX);

  for (int i = innerWidth - 1; i > 0; --i) {
    int prevY = motorGraphY[i];
    int srcY = motorGraphY[i - 1];
    if (prevY != srcY) {
      int x = rightX - i;
      g_display->drawPixel(x, prevY, ST77XX_BLACK);
      g_display->drawPixel(x, srcY, COLOR_ACCENT);
    }
    motorGraphY[i] = srcY;
    if (((rightX - i) - leftX) % 3 == 0) {
      g_display->drawPixel(rightX - i, setpointY, ST77XX_RED);
    }
  }
  int prevNewestY = motorGraphY[0];
  if (prevNewestY != newY) {
    g_display->drawPixel(rightX, prevNewestY, ST77XX_BLACK);
    g_display->drawPixel(rightX, newY, COLOR_ACCENT);
  }
  motorGraphY[0] = newY;
  if ((rightX - leftX) % 3 == 0)
    g_display->drawPixel(rightX, setpointY, ST77XX_RED);

  g_display->fillRect(24, 105, 35, 8, COLOR_BG);
  g_display->setTextColor(COLOR_ACCENT);
  g_display->setCursor(24, 105);
  g_display->print(g_motorSpeed, 0);
  static bool lastRunMotor = false;
  if (lastRunMotor != g_motorRunning) {
    g_display->fillRect(20, 125, 40, 8, COLOR_BG);
    g_display->setCursor(20, 125);
    g_display->setTextColor(g_motorRunning ? ST77XX_GREEN : ST77XX_RED);
    g_display->print(g_motorRunning ? "RUN" : "STOP");
    lastRunMotor = g_motorRunning;
  }
}

void drawMotorControl(int updateLine = -1) {
  if (updateLine == -1) {
    g_display->fillRect(0, 20, 128, 140, COLOR_BG);
    drawFooter("1x:EDT 2x:DIR Lx:BACK");
  }

  const char *items[] = {"Run", "Kp", "Ki", "Kd", "Setpt", "PID"};
  int start = (updateLine == -1) ? 0 : updateLine;
  int end = (updateLine == -1) ? 6 : updateLine + 1;

  for (int i = start; i < end; i++) {
    int y = 25 + i * 15;
    bool sel = (g_selectedIndex == i);

    if (updateLine != -1) {
      g_display->fillRect(0, y, 128, 10, COLOR_BG);
    }

    g_display->setTextColor(sel ? COLOR_SELECTED : COLOR_TEXT);
    g_display->setCursor(5, y);
    g_display->print(items[i]);
    g_display->print(":");

    g_display->setTextColor(COLOR_ACCENT);
    g_display->setCursor(60, y);
    if (i == 0)
      g_display->print(g_motorRunning ? "ON " : "OFF");
    else if (i == 1)
      g_display->print(g_acKp, 2);
    else if (i == 2)
      g_display->print(g_acKi, 2);
    else if (i == 3)
      g_display->print(g_acKd, 2);
    else if (i == 4)
      g_display->print(g_acSetpoint, 1);
    else if (i == 5)
      g_display->print(g_acPidMode ? "ON " : "OFF");
  }
}

void drawMotorMenu() {
  g_display->fillScreen(COLOR_BG);
  drawHeader(g_motorSubMenu == 0 ? "AC GRAPH" : "AC CTRL");
  // sendTaggedFloat(MSG_AC_Control, 1);

  if (g_motorSubMenu == 0) {
    drawMotorGraph(true);
  } else {
    drawMotorControl();
  }
}

void drawWiFiMenu() {
  g_display->fillScreen(COLOR_BG);
  drawHeader("NETWORK");

  g_display->setTextSize(1);
  g_display->setTextColor(COLOR_TEXT);
  g_display->setCursor(10, 30);
  g_display->print("WiFi: ");
  g_display->setTextColor(g_wifiConnected ? ST77XX_GREEN : ST77XX_RED);
  g_display->print(g_wifiConnected ? "OK" : "FAIL");

  g_display->setTextColor(COLOR_TEXT);
  g_display->setCursor(10, 50);
  g_display->print("ESP-NOW: ");
  g_display->setTextColor(espconnected ? ST77XX_GREEN : ST77XX_RED);
  g_display->print(espconnected ? "OK" : "FAIL");

  drawMenuItem(110, "Back", g_selectedIndex == 0);

  drawFooter("1x:SEL Lx:BACK");
}

void drawCurrentMenu() {
  if (g_currentMenu == 0) {
    drawRootMenu();
  } else if (g_currentMenu == 1) {
    drawEscalatorMenu();
  } else if (g_currentMenu == 2) {
    drawMotorMenu();
  } else if (g_currentMenu == 3) {
    drawWiFiMenu();
  }
}

// ========================================
// NAVIGASI
// ========================================

void menuNavigate(int direction) {
  if (g_isEditing) {
    // Edit mode
    if (g_currentMenu == 1 && g_escSubMenu == 1) {
      // Escalator control edit
      if (g_selectedIndex == 1) {
        g_dcKp += direction * 0.1;
        g_dcKp = constrain(g_dcKp, 0, 100);
      } else if (g_selectedIndex == 2) {
        g_dcKi += direction * 0.01;
        g_dcKi = constrain(g_dcKi, 0, 10);
      } else if (g_selectedIndex == 3) {
        g_dcKd += direction * 0.01;
        g_dcKd = constrain(g_dcKd, 0, 10);
      } else if (g_selectedIndex == 4) {
        g_dcSetpoint += direction * 5;
        g_dcSetpoint = constrain(g_dcSetpoint, 0, ESC_GRAPH_MAX);
      }
      // Update only current line
      drawEscControl(g_selectedIndex);
    } else if (g_currentMenu == 2 && g_motorSubMenu == 1) {
      // AC motor control edit
      if (g_selectedIndex == 1) {
        g_acKp += direction * 0.1;
        g_acKp = constrain(g_acKp, 0, 10);
      } else if (g_selectedIndex == 2) {
        g_acKi += direction * 0.01;
        g_acKi = constrain(g_acKi, 0, 5);
      } else if (g_selectedIndex == 3) {
        g_acKd += direction * 0.01;
        g_acKd = constrain(g_acKd, 0, 5);
      } else if (g_selectedIndex == 4) {
        g_acSetpoint += direction * 5;
        g_acSetpoint = constrain(g_acSetpoint, 0, AC_GRAPH_MAX);
      }
      // Update only current line
      drawMotorControl(g_selectedIndex);
    }
  } else {
    // Navigation mode
    int maxItems = 3;
    if (g_currentMenu == 1 && g_escSubMenu == 1)
      maxItems = 7; // include Direction option
    else if (g_currentMenu == 2 && g_motorSubMenu == 1)
      maxItems = 6;
    else if (g_currentMenu != 0)
      maxItems = 1;

    int oldIndex = g_selectedIndex;
    g_selectedIndex += direction;
    if (g_selectedIndex < 0)
      g_selectedIndex = maxItems - 1;
    if (g_selectedIndex >= maxItems)
      g_selectedIndex = 0;

    // Update only affected lines in control menu
    if (g_currentMenu == 1 && g_escSubMenu == 1) {
      drawEscControl(oldIndex);
      drawEscControl(g_selectedIndex);
    } else if (g_currentMenu == 2 && g_motorSubMenu == 1) {
      drawMotorControl(oldIndex);
      drawMotorControl(g_selectedIndex);
    } else {
      drawCurrentMenu();
    }
  }
}

void menuSelect() {
  if (g_currentMenu == 0) {
    g_currentMenu = g_selectedIndex + 1;
    g_selectedIndex = 0;
    g_escSubMenu = 0;
    g_motorSubMenu = 0;
    drawCurrentMenu();
  } else if (g_currentMenu == 1) {
    if (g_escSubMenu == 0) {
      // Switch to control submenu
      g_escSubMenu = 1;
      g_selectedIndex = 0;
      drawCurrentMenu();
    } else {
      // Control menu actions
      if (g_selectedIndex == 0) {
        g_escRunning = !g_escRunning;
        sendTaggedFloat(MSG_DC_Control, g_escRunning ? 1 : 0);
        Serial.print("Sent DC Control");
        Serial.println(g_escRunning ? 1 : 0);
      } else if (g_selectedIndex == 1) {
        g_isEditing = !g_isEditing;
        if (!g_isEditing)
          sendTaggedFloat(MSG_DC_KP, g_dcKp);
        Serial.print("Sent DC KP");
        Serial.println(g_dcKp);
      } else if (g_selectedIndex == 2) {
        g_isEditing = !g_isEditing;
        if (!g_isEditing)
          sendTaggedFloat(MSG_DC_KI, g_dcKi);
        Serial.print("Sent DC KI");
        Serial.println(g_dcKi);
      } else if (g_selectedIndex == 3) {
        g_isEditing = !g_isEditing;
        if (!g_isEditing)
          sendTaggedFloat(MSG_DC_KD, g_dcKd);
        Serial.print("Sent DC KD");
        Serial.println(g_dcKd);
      } else if (g_selectedIndex == 4) {
        g_isEditing = !g_isEditing;
        if (!g_isEditing)
          sendTaggedFloat(MSG_DC_Setpoint, g_dcSetpoint);
        Serial.print("Sent DC Setpoint");
        Serial.println(g_dcSetpoint);
      } else if (g_selectedIndex == 5) {
        g_pidMode = !g_pidMode;
        sendTaggedFloat(MSG_PID_MODE, g_pidMode ? 1 : 0);
        Serial.print("Sent PID Mode");
        Serial.println(g_pidMode ? 1 : 0);
      } else if (g_selectedIndex == 6) {
        // Toggle direction and send message
        g_escDirection = g_escDirection ? 0 : 1;
        sendTaggedFloat(MSG_DC_Direction, g_escDirection ? 1 : 0);
        Serial.print("Sent DC Direction: ");
        Serial.println(g_escDirection ? 1 : 0);
      }
      // Update only current line
      drawEscControl(g_selectedIndex);
    }
  } else if (g_currentMenu == 2) {
    if (g_motorSubMenu == 0) {
      // Switch to control submenu
      g_motorSubMenu = 1;
      g_selectedIndex = 0;
      drawCurrentMenu();
    } else {
      // Control menu actions
      if (g_selectedIndex == 0) {
        g_motorRunning = !g_motorRunning;
        sendTaggedFloat(MSG_AC_Control, g_motorRunning ? 1 : 0);
        Serial.print("Sent AC Control: ");
        Serial.println(g_motorRunning ? 1 : 0);
      } else if (g_selectedIndex == 1) {
        g_isEditing = !g_isEditing;
        if (!g_isEditing)
          sendTaggedFloat(MSG_AC_KP, g_acKp);
        Serial.print("Sent AC KP: ");
        Serial.println(g_acKp);
      } else if (g_selectedIndex == 2) {
        g_isEditing = !g_isEditing;
        if (!g_isEditing)
          sendTaggedFloat(MSG_AC_KI, g_acKi);
        Serial.print("Sent AC KI: ");
        Serial.println(g_acKi);
      } else if (g_selectedIndex == 3) {
        g_isEditing = !g_isEditing;
        if (!g_isEditing)
          sendTaggedFloat(MSG_AC_KD, g_acKd);
        Serial.print("Sent AC KD: ");
        Serial.println(g_acKd);
      } else if (g_selectedIndex == 4) {
        g_isEditing = !g_isEditing;
        if (!g_isEditing)
          sendTaggedFloat(MSG_AC_Setpoint, g_acSetpoint);
        Serial.print("Sent AC Setpoint: ");
        Serial.println(g_acSetpoint);
      } else if (g_selectedIndex == 5) {
        g_acPidMode = !g_acPidMode;
        sendTaggedFloat(MSG_PID_MODE, g_acPidMode ? 1 : 0);
        Serial.print("Sent AC PID Mode: ");
        Serial.println(g_acPidMode ? 1 : 0);
      }
      // Update only current line
      drawMotorControl(g_selectedIndex);
    }
  } else {
    menuBack();
  }
}

void menuBack() {
  if (g_isEditing) {
    g_isEditing = false;
    // Update only current line when exiting edit mode
    if (g_currentMenu == 1 && g_escSubMenu == 1) {
      drawEscControl(g_selectedIndex);
    } else if (g_currentMenu == 2 && g_motorSubMenu == 1) {
      drawMotorControl(g_selectedIndex);
    } else {
      drawCurrentMenu();
    }
  } else if (g_currentMenu == 1 && g_escSubMenu == 1) {
    // Back to graph from escalator control
    g_escSubMenu = 0;
    g_selectedIndex = 0;
    drawCurrentMenu();
  } else if (g_currentMenu == 2 && g_motorSubMenu == 1) {
    // Back to graph from AC motor control
    g_motorSubMenu = 0;
    g_selectedIndex = 0;
    drawCurrentMenu();
  } else {
    g_currentMenu = 0;
    g_selectedIndex = 0;
    drawCurrentMenu();
  }
}

void menuDoubleClick() {
  // Toggle motor run/stop when in graph mode
  if (g_currentMenu == 1 && g_escSubMenu == 0) {
    // Escalator graph mode - toggle DC motor
    g_escRunning = !g_escRunning;
    sendTaggedFloat(MSG_DC_Control, g_escRunning ? 1 : 0);
    Serial.print("DC Motor Toggle: ");
    Serial.println(g_escRunning ? "RUN" : "STOP");
    g_escStatusChanged = true;
  } else if (g_currentMenu == 2 && g_motorSubMenu == 0) {
    // AC motor graph mode - toggle AC motor
    g_motorRunning = !g_motorRunning;
    sendTaggedFloat(MSG_AC_Control, g_motorRunning ? 1 : 0);
    Serial.print("AC Motor Toggle: ");
    Serial.println(g_motorRunning ? "RUN" : "STOP");
    g_motorStatusChanged = true;
  }
}

// ========================================
// UPDATE DATA
// ========================================

void updateEscalatorSpeed(float speed) {
  g_escSpeed = speed;
  g_escConnected = true;
  g_escSpeedHistory[g_escHistoryIndex] = speed;
  g_escHistoryIndex = (g_escHistoryIndex + 1) % 100;

  // Set flag instead of drawing directly (avoid LCD ops in callback)
  g_escSpeedUpdated = true;
}
void updateMotorSpeed(float speed) {
  g_motorSpeed = speed;
  g_motorConnected = true;
  g_motorSpeedHistory[g_motorHistoryIndex] = speed;
  g_motorHistoryIndex = (g_motorHistoryIndex + 1) % 100;

  // Set flag instead of drawing directly (avoid LCD ops in callback)
  g_motorSpeedUpdated = true;
}

void menuUpdate() {
  // Check flags and update graphs if needed (safe in main loop)
  if ((g_escSpeedUpdated || g_escStatusChanged) && g_currentMenu == 1 &&
      g_escSubMenu == 0) {
    drawEscGraph(false);
    g_escSpeedUpdated = false;
    g_escStatusChanged = false;
  }

  if ((g_motorSpeedUpdated || g_motorStatusChanged) && g_currentMenu == 2 &&
      g_motorSubMenu == 0) {
    drawMotorGraph(false);
    g_motorSpeedUpdated = false;
    g_motorStatusChanged = false;
  }

  g_lastUpdate = millis();
}
