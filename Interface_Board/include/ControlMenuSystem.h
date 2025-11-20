#ifndef CONTROL_MENU_SYSTEM_H
#define CONTROL_MENU_SYSTEM_H

#include <Adafruit_ST7735.h>

// Variabel global
extern Adafruit_ST7735 *g_display;
extern int g_currentMenu;
extern int g_selectedIndex;
extern bool g_isEditing;
extern float g_escSpeed;
extern float g_escTarget;
extern int g_escDirection;
extern int g_escMode;
extern bool g_escRunning;
extern bool g_escConnected;
extern float g_escSpeedHistory[80];
extern int g_escHistoryIndex;
extern float g_motorSpeed;
extern float g_motorTarget;
extern int g_motorMode;
extern bool g_motorConnected;
extern float g_motorSpeedHistory[80];
extern int g_motorHistoryIndex;
extern bool g_wifiConnected;
extern bool g_espnowInit;
extern unsigned long g_lastUpdate;

// Fungsi
void menuInit(Adafruit_ST7735 *tft);
void menuNavigate(int direction);
void menuSelect();
void menuBack();
void updateEscalatorSpeed(float speed);
void updateMotorSpeed(float speed);
void menuUpdate();
void showSplash();
void drawCurrentMenu();
void drawRootMenu();
void drawEscalatorMenu();
void drawMotorMenu();
void drawWiFiMenu();

#endif
