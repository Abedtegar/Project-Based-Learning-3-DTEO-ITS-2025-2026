#ifndef CONTROL_MENU_SYSTEM_H
#define CONTROL_MENU_SYSTEM_H

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Arduino.h>

// ========================================
// ENUM DEFINITIONS
// ========================================

// Menu utama dan submenu
enum MenuLevel {
  MENU_ROOT,            // Menu utama
  MENU_ESCALATOR,       // Submenu Ekskalator
  MENU_ESCALATOR_GRAPH, // Grafik kecepatan ekskalator
  MENU_ESCALATOR_PID,   // Setting PID ekskalator
  MENU_ESCALATOR_MODE,  // Mode manual/auto ekskalator
  MENU_MOTOR_AC,        // Submenu Motor AC
  MENU_MOTOR_GRAPH,     // Grafik kecepatan motor AC
  MENU_MOTOR_CONTROL,   // Pilih metode kontrol (DAC/PWM/Modbus)
  MENU_MOTOR_VOLTAGE,   // Pilih tegangan (5V/10V)
  MENU_MOTOR_PID,       // Setting PID motor AC
  MENU_MOTOR_MODE,      // Mode manual/auto motor AC
  MENU_WIFI_STATUS      // Status WiFi & ESP-NOW
};

enum ControlMode { MODE_MANUAL, MODE_AUTO };

enum MotorControlType { CTRL_DAC, CTRL_PWM, CTRL_MODBUS };

enum VoltageType { VOLTAGE_5V, VOLTAGE_10V };

enum Direction { DIR_FORWARD, DIR_REVERSE };

// ========================================
// DATA STRUCTURES
// ========================================

// Struktur PID parameters
struct PIDParams {
  float kp;
  float ki;
  float kd;
  float setPoint;
  int editingParam; // 0=kp, 1=ki, 2=kd, 3=setpoint
};

// Data ekskalator dari ESP-NOW
struct EscalatorData {
  float currentSpeed; // RPM atau m/s
  float targetSpeed;
  Direction direction;
  ControlMode mode;
  PIDParams pid;
  bool isConnected;
  unsigned long lastUpdate;

  // Data untuk grafik (buffer ring)
  static const int GRAPH_BUFFER_SIZE = 80; // Lebar grafik
  float speedHistory[GRAPH_BUFFER_SIZE];
  int historyIndex;
};

// Data Motor AC dari ESP-NOW
struct MotorACData {
  float currentSpeed;
  float targetSpeed;
  ControlMode mode;
  MotorControlType controlType;
  VoltageType voltageType;
  PIDParams pid;
  bool isConnected;
  unsigned long lastUpdate;

  // Data untuk grafik
  static const int GRAPH_BUFFER_SIZE = 80;
  float speedHistory[GRAPH_BUFFER_SIZE];
  int historyIndex;
};

// Status WiFi & ESP-NOW
struct NetworkStatus {
  bool wifiConnected;
  String ssid;
  String ipAddress;
  int clientCount;
  bool espnowInitialized;
  int peerCount;
};

// ========================================
// MAIN CLASS
// ========================================

class ControlMenuSystem {
private:
  Adafruit_ST7735 *display;

  // Menu state
  MenuLevel currentMenu;
  int selectedIndex;
  bool isEditing; // Flag untuk mode editing (PID, setpoint, dll)

  // Data structures
  EscalatorData escalatorData;
  MotorACData motorACData;
  NetworkStatus networkStatus;

  // UI Colors
  uint16_t COLOR_BG;
  uint16_t COLOR_HEADER;
  uint16_t COLOR_SELECTED;
  uint16_t COLOR_EDITING;
  uint16_t COLOR_TEXT;
  uint16_t COLOR_ACCENT;
  uint16_t COLOR_SUCCESS;
  uint16_t COLOR_ERROR;
  uint16_t COLOR_WARNING;
  uint16_t COLOR_GRAPH_GRID;
  uint16_t COLOR_GRAPH_LINE;

  // Timing untuk update otomatis
  unsigned long lastGraphUpdate;
  unsigned long graphUpdateInterval;

  // Helper functions untuk drawing
  void drawHeader(const char *title, bool showBackIcon = true);
  void drawFooter(const char *text);
  void drawMenuItem(int y, const char *text, bool selected,
                    bool hasSubmenu = false);
  void drawValueBar(int x, int y, int w, int h, float value, float maxVal,
                    uint16_t color);
  void drawGraph(int x, int y, int w, int h, float *data, int dataSize,
                 float maxVal);
  void drawPIDEditor(PIDParams &pid, const char *title);
  void drawStatusIndicator(int x, int y, bool connected);

  // Menu drawing functions
  void drawRootMenu();
  void drawEscalatorMenu();
  void drawEscalatorGraph();
  void drawEscalatorPID();
  void drawEscalatorMode();
  void drawMotorACMenu();
  void drawMotorGraph();
  void drawMotorControl();
  void drawMotorVoltage();
  void drawMotorPID();
  void drawMotorMode();
  void drawWiFiStatus();

  // Helper untuk text centering
  int getCenterX(const char *text, int textSize = 1);

public:
  ControlMenuSystem(Adafruit_ST7735 *tft);

  // Initialization
  void begin();
  void showSplashScreen();

  // Navigation (dipanggil dari encoder)
  void navigate(int direction); // direction: +1 atau -1
  void select();                // Tombol encoder ditekan
  void back();                  // Long press atau tombol back

  // Update data dari ESP-NOW
  void updateEscalatorData(float speed, Direction dir);
  void updateMotorACData(float speed);
  void updateNetworkStatus(bool wifiConn, String ssid, String ip, int clients);
  void updateESPNowStatus(bool initialized, int peers);

  // Getter untuk kontrol eksternal
  MenuLevel getCurrentMenu() const { return currentMenu; }
  bool isInEditMode() const { return isEditing; }

  // Untuk pengiriman data ke device via ESP-NOW
  EscalatorData getEscalatorSettings() const { return escalatorData; }
  MotorACData getMotorACSettings() const { return motorACData; }

  // Update periodic (panggil di loop untuk update grafik)
  void update();
};

#endif // CONTROL_MENU_SYSTEM_H
