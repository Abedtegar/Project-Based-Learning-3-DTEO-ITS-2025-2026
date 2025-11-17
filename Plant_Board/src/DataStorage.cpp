#include "DataStorage.h"
#include "LedControl.h"
#include <PID.h>
#include <Plant.h>
#include <PlantConfig.h>
Preferences preferences;

void DataStorage_INIT() {
  Serial.println(">>> Initializing DataStorage...");

  // === DC PID Configuration ===
  preferences.begin("pid_dc", false);

  if (!preferences.isKey("kP")) {
    Serial.println("First boot - Writing DC PID defaults");
    preferences.putFloat("kP", 1.5);
    preferences.putFloat("kI", 0.1);
    preferences.putFloat("kD", 0.05);
    preferences.putFloat("setpoint", 50.0);
  } else {
    Serial.println("Loading existing DC PID config");
  }

  DCkP = preferences.getFloat("kP", 1.5);
  DCkI = preferences.getFloat("kI", 0.1);
  DCkD = preferences.getFloat("kD", 0.05);
  DCsetpoint = preferences.getFloat("setpoint", 50.0);

  preferences.end();

  Serial.println("DC PID: kP=" + String(DCkP, 3) + " kI=" + String(DCkI, 3) +
                 " kD=" + String(DCkD, 3));

  // === AC PID Configuration ===
  preferences.begin("pid_ac", false);

  if (!preferences.isKey("kP")) {
    Serial.println("First boot - Writing AC PID defaults");
    preferences.putFloat("kP", 2.0);
    preferences.putFloat("kI", 0.08);
    preferences.putFloat("kD", 0.04);
    preferences.putFloat("setpoint", 0.0);
  } else {
    Serial.println("Loading existing AC PID config");
  }

  ACkP = preferences.getFloat("kP", 2.0);
  ACkI = preferences.getFloat("kI", 0.08);
  ACkD = preferences.getFloat("kD", 0.04);
  ACsetpoint = preferences.getFloat("setpoint", 0.0);

  preferences.end();

  Serial.println("AC PID: kP=" + String(ACkP, 3) + " kI=" + String(ACkI, 3) +
                 " kD=" + String(ACkD, 3));
  Serial.println("✓ DataStorage initialized!\n");
}

float ReadData(String name, String data, float &value) {
  preferences.begin(name.c_str(), true);
  value = preferences.getFloat(data.c_str(), 0.0);
  preferences.end();

  Serial.println("✓ Read: " + name + "/" + data + " = " + String(value, 3));
  return value;
}

void StoreData(String name, String data, float value) {
  preferences.begin(name.c_str(), false);
  preferences.putFloat(data.c_str(), value);
  preferences.end();

  Serial.println("✓ Stored: " + name + "/" + data + " = " + String(value, 3));

  if (name == "pid_dc") {
    if (data == "kP")
      DCkP = value;
    else if (data == "kI")
      DCkI = value;
    else if (data == "kD")
      DCkD = value;
    else if (data == "setpoint")
      DCsetpoint = value;
  } else if (name == "pid_ac") {
    if (data == "kP")
      ACkP = value;
    else if (data == "kI")
      ACkI = value;
    else if (data == "kD")
      ACkD = value;
    else if (data == "setpoint")
      ACsetpoint = value;
  }
}

void PrintStoredData() {
  Serial.println("\n=== Stored Configuration ===");

  // DC PID
  preferences.begin("pid_dc", true);
  Serial.println("[ DC PID ]");
  Serial.println("  kP: " + String(preferences.getFloat("kP", 0.0), 3));
  Serial.println("  kI: " + String(preferences.getFloat("kI", 0.0), 3));
  Serial.println("  kD: " + String(preferences.getFloat("kD", 0.0), 3));
  Serial.println("  Setpoint: " +
                 String(preferences.getFloat("setpoint", 0.0), 1));
  preferences.end();

  // AC PID
  preferences.begin("pid_ac", true);
  Serial.println("[ AC PID ]");
  Serial.println("  kP: " + String(preferences.getFloat("kP", 0.0), 3));
  Serial.println("  kI: " + String(preferences.getFloat("kI", 0.0), 3));
  Serial.println("  kD: " + String(preferences.getFloat("kD", 0.0), 3));
  Serial.println("  Setpoint: " +
                 String(preferences.getFloat("setpoint", 0.0), 1));
  preferences.end();

  Serial.println("============================\n");
}