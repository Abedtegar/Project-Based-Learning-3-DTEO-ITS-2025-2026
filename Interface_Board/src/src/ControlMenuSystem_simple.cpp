#include "ControlMenuSystem.h"
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
bool g_isEditing = false;

// Escalator data
float g_escSpeed = 0;
float g_escTarget = 0;
int g_escDirection = 0;
int g_escMode = 0;
bool g_escRunning = false;
bool g_escConnected = false;
float g_escSpeedHistory[80] = {0};
int g_escHistoryIndex = 0;

// Motor data
float g_motorSpeed = 0;
float g_motorTarget = 0;
int g_motorMode = 0;
bool g_motorConnected = false;
float g_motorSpeedHistory[80] = {0};
int g_motorHistoryIndex = 0;

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

  drawMenuItem(30, "Escalator", g_selectedIndex == 0);
  drawMenuItem(60, "Motor AC", g_selectedIndex == 1);
  drawMenuItem(90, "WiFi Status", g_selectedIndex == 2);
}

void drawEscalatorMenu() {
  g_display->fillScreen(COLOR_BG);
  drawHeader("ESCALATOR");

  // Show current speed
  g_display->setTextSize(1);
  g_display->setTextColor(COLOR_TEXT);
  g_display->setCursor(10, 30);
  g_display->print("Speed: ");
  g_display->setTextColor(COLOR_ACCENT);
  g_display->print(g_escSpeed, 1);

  g_display->setTextColor(COLOR_TEXT);
  g_display->setCursor(10, 45);
  g_display->print("Target: ");
  g_display->setTextColor(COLOR_ACCENT);
  g_display->print(g_escTarget, 1);

  g_display->setTextColor(COLOR_TEXT);
  g_display->setCursor(10, 60);
  g_display->print("Dir: ");
  g_display->print(g_escDirection == 0 ? "FWD" : "REV");

  g_display->setCursor(10, 75);
  g_display->print("Mode: ");
  g_display->print(g_escMode == 0 ? "Manual" : "Auto");

  g_display->setCursor(10, 90);
  g_display->print("Run: ");
  g_display->print(g_escRunning ? "ON" : "OFF");

  // Simple menu
  drawMenuItem(110, "Toggle Run", g_selectedIndex == 0);
}

void drawMotorMenu() {
  g_display->fillScreen(COLOR_BG);
  drawHeader("MOTOR AC");

  g_display->setTextSize(1);
  g_display->setTextColor(COLOR_TEXT);
  g_display->setCursor(10, 30);
  g_display->print("Speed: ");
  g_display->setTextColor(COLOR_ACCENT);
  g_display->print(g_motorSpeed, 0);

  g_display->setTextColor(COLOR_TEXT);
  g_display->setCursor(10, 45);
  g_display->print("Target: ");
  g_display->setTextColor(COLOR_ACCENT);
  g_display->print(g_motorTarget, 0);

  g_display->setTextColor(COLOR_TEXT);
  g_display->setCursor(10, 60);
  g_display->print("Mode: ");
  g_display->print(g_motorMode == 0 ? "Manual" : "Auto");

  drawMenuItem(110, "Back", g_selectedIndex == 0);
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
  g_display->setTextColor(g_espnowInit ? ST77XX_GREEN : ST77XX_RED);
  g_display->print(g_espnowInit ? "OK" : "FAIL");

  drawMenuItem(110, "Back", g_selectedIndex == 0);
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
    // Edit mode - change values
    if (g_currentMenu == 1) {
      g_escTarget += direction * 5;
      if (g_escTarget < 0)
        g_escTarget = 0;
      if (g_escTarget > 100)
        g_escTarget = 100;
    } else if (g_currentMenu == 2) {
      g_motorTarget += direction * 50;
      if (g_motorTarget < 0)
        g_motorTarget = 0;
      if (g_motorTarget > 3000)
        g_motorTarget = 3000;
    }
    drawCurrentMenu();
  } else {
    // Navigation mode
    int maxItems = 3;
    if (g_currentMenu != 0)
      maxItems = 1;

    g_selectedIndex += direction;
    if (g_selectedIndex < 0)
      g_selectedIndex = maxItems - 1;
    if (g_selectedIndex >= maxItems)
      g_selectedIndex = 0;

    drawCurrentMenu();
  }
}

void menuSelect() {
  if (g_currentMenu == 0) {
    // Root menu
    g_currentMenu = g_selectedIndex + 1;
    g_selectedIndex = 0;
    drawCurrentMenu();
  } else if (g_currentMenu == 1) {
    // Escalator menu - toggle run
    g_escRunning = !g_escRunning;
    drawCurrentMenu();
  } else {
    // Other menus - back
    menuBack();
  }
}

void menuBack() {
  if (g_isEditing) {
    g_isEditing = false;
    drawCurrentMenu();
  } else {
    g_currentMenu = 0;
    g_selectedIndex = 0;
    drawCurrentMenu();
  }
}

// ========================================
// UPDATE DATA
// ========================================

void updateEscalatorSpeed(float speed) {
  g_escSpeed = speed;
  g_escConnected = true;
  g_escSpeedHistory[g_escHistoryIndex] = speed;
  g_escHistoryIndex = (g_escHistoryIndex + 1) % 80;
}

void updateMotorSpeed(float speed) {
  g_motorSpeed = speed;
  g_motorConnected = true;
  g_motorSpeedHistory[g_motorHistoryIndex] = speed;
  g_motorHistoryIndex = (g_motorHistoryIndex + 1) % 80;
}

void menuUpdate() {
  // Periodic update - redraw if needed
  if (millis() - g_lastUpdate > 1000) {
    if (g_currentMenu == 1 || g_currentMenu == 2) {
      drawCurrentMenu();
    }
    g_lastUpdate = millis();
  }
}
