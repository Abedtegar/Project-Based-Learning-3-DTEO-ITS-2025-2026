#include "DataStorage.h"
#include "LedControl.h"
#include <PID.h>
#include <Plant.h>
#include <PlantConfig.h>
#include <Preferences.h>
#include <esp_err.h>
#include <nvs_flash.h>

// Preferences instance untuk NVS storage
static Preferences preferences;

void ResetNVSData() {
  Serial.println("Resetting aall NVS data...");
  preferences.clear();
  Serial.println("NVS data cleared. Restarting...");
  ESP.restart();
}

void DataStorage_INIT() {

  if (!preferences.begin("plant_config", false)) {
    Serial.println("[ERROR] Failed to initialize Preferences!");
    Serial.println("Using default PID values");
    return;
  } else {
    Serial.println("[OK] Preferences initialized successfully");
  }

  if (preferences.isKey("dc_kp")) {
    // Load DC PID values from NVS with safe defaults
    DCkP = preferences.getFloat("dc_kp", 1.5f);
    DCkI = preferences.getFloat("dc_ki", 0.1f);
    DCkD = preferences.getFloat("dc_kd", 0.05f);
    DCsetpoint = preferences.getFloat("dc_setpoint", 50.0f);
    DCDirection = preferences.getBool("dc_direction", false);

    // Load AC PID values from NVS with safe defaults
    ACkP = preferences.getFloat("ac_kp", 2.0f);
    ACkI = preferences.getFloat("ac_ki", 0.08f);
    ACkD = preferences.getFloat("ac_kd", 0.04f);
    ACsetpoint = preferences.getFloat("ac_setpoint", 0.0f);
    ACVoltage = preferences.getBool("ac_voltage", false);

    Serial.println("\n=== Loaded PID values from NVS ===");
    Serial.println("DC PID: kP=" + String(DCkP, 3) + " kI=" + String(DCkI, 3) +
                   " kD=" + String(DCkD, 3));
    Serial.println("DC Direction: " + String(DCDirection ? "CW" : "CCW"));
    Serial.println("AC PID: kP=" + String(ACkP, 3) + " kI=" + String(ACkI, 3) +
                   " kD=" + String(ACkD, 3));
    Serial.println("AC Voltage: " + String(ACVoltage ? "ON" : "OFF"));
    Serial.println("DataStorage initialized!");
    Serial.println();
  } else {
    Serial.println("nodata");
    DCkP = 1.5f;
    DCkI = 0.1f;
    DCkD = 0.05f;
    DCsetpoint = 50.0f;
    DCDirection = false;
    ACkP = 2.0f;
    ACkI = 0.08f;
    ACkD = 0.04f;
    ACsetpoint = 0.0f;
    ACVoltage = false;
  }
}
float ReadData(String name, String data, float &value) {
  // Read from NVS
  String key = name + "_" + data;

  if (name == "pid_dc") {
    if (data == "kP") {
      value = preferences.getFloat("dc_kp", DCkP);
    } else if (data == "kI") {
      value = preferences.getFloat("dc_ki", DCkI);
    } else if (data == "kD") {
      value = preferences.getFloat("dc_kd", DCkD);
    } else if (data == "setpoint") {
      value = preferences.getFloat("dc_setpoint", DCsetpoint);
    }
  } else if (name == "pid_ac") {
    if (data == "kP") {
      value = preferences.getFloat("ac_kp", ACkP);
    } else if (data == "kI") {
      value = preferences.getFloat("ac_ki", ACkI);
    } else if (data == "kD") {
      value = preferences.getFloat("ac_kd", ACkD);
    } else if (data == "setpoint") {
      value = preferences.getFloat("ac_setpoint", ACsetpoint);
    }
  }

  Serial.print("Read from NVS: ");
  Serial.print(name);
  Serial.print("/");
  Serial.print(data);
  Serial.print(" = ");
  Serial.println(value, 3);

  return value;
}

void StoreData(String name, String data, float value) {
  // Validasi Preferences sudah initialized
  // if (!preferences) {
  //   Serial.println("[WARNING] Preferences not initialized, skipping store");
  //   return;
  // }

  // Store to NVS
  Serial.print("[Store] ");
  Serial.print(name);
  Serial.print("/");
  Serial.print(data);
  Serial.print(" = ");
  Serial.println(value, 3);

  if (name == "pid_dc") {
    if (data == "kP") {
      preferences.putFloat("dc_kp", value);
      DCkP = value;
    } else if (data == "kI") {
      preferences.putFloat("dc_ki", value);
      DCkI = value;
    } else if (data == "kD") {
      preferences.putFloat("dc_kd", value);
      DCkD = value;
    } else if (data == "setpoint") {
      preferences.putFloat("dc_setpoint", value);
      DCsetpoint = value;
    } else if (data == "dcdirection") {
      preferences.putBool("dc_direction", value != 0);
      DCDirection = value != 0;
    }
  } else if (name == "pid_ac") {
    if (data == "kP") {
      preferences.putFloat("ac_kp", value);
      ACkP = value;
    } else if (data == "kI") {
      preferences.putFloat("ac_ki", value);
      ACkI = value;
    } else if (data == "kD") {
      preferences.putFloat("ac_kd", value);
      ACkD = value;
    } else if (data == "setpoint") {
      preferences.putFloat("ac_setpoint", value);
      ACsetpoint = value;
    } else if (data == "acvoltage") {
      preferences.putBool("ac_voltage", value != 0);
      ACVoltage = value != 0;
    }
  }
}

void PrintStoredData() {
  Serial.println();
  Serial.println("=== Current Configuration (NVS) ===");
  Serial.println("[ DC PID ]");
  Serial.print("  kP: ");
  Serial.println(DCkP);
  Serial.print("  kI: ");
  Serial.println(DCkI);
  Serial.print("  kD: ");
  Serial.println(DCkD);
  Serial.print("  Setpoint: ");
  Serial.println(DCsetpoint);

  Serial.println("[ AC PID ]");
  Serial.print("  kP: ");
  Serial.println(ACkP);
  Serial.print("  kI: ");
  Serial.println(ACkI);
  Serial.print("  kD: ");
  Serial.println(ACkD);
  Serial.print("  Setpoint: ");
  Serial.println(ACsetpoint);

  Serial.println("============================");
  Serial.println();
}
