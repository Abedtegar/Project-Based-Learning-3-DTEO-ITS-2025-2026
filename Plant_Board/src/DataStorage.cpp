#include "DataStorage.h"
#include "LedControl.h"
#include <PID.h>
#include <Plant.h>
#include <PlantConfig.h>
#include <Preferences.h>

// Preferences instance untuk NVS storage
static Preferences preferences;

void DataStorage_INIT() {
  Serial.println(">>> Initializing DataStorage with Preferences...");

  // Initialize Preferences
  if (!preferences.begin("plant_config", false)) {
    Serial.println("ERROR: Failed to initialize Preferences!");
    Serial.println("Using default PID values");
  } else {
    Serial.println("Preferences initialized successfully");
  }

  // Load DC PID values from NVS, use defaults if not found
  DCkP = preferences.getFloat("dc_kp", 1.5);
  DCkI = preferences.getFloat("dc_ki", 0.1);
  DCkD = preferences.getFloat("dc_kd", 0.05);
  DCsetpoint = preferences.getFloat("dc_setpoint", 50.0);
  DCDirection = preferences.getBool("dc_direction", false);

  // Load AC PID values from NVS, use defaults if not found
  ACkP = preferences.getFloat("ac_kp", 2.0);
  ACkI = preferences.getFloat("ac_ki", 0.08);
  ACkD = preferences.getFloat("ac_kd", 0.04);
  ACsetpoint = preferences.getFloat("ac_setpoint", 0.0);
  ACVoltage = preferences.getBool("ac_voltage", false);

  Serial.println("Loaded PID values from NVS:");
  Serial.println("DC PID: kP=" + String(DCkP, 3) + " kI=" + String(DCkI, 3) +
                 " kD=" + String(DCkD, 3));
  Serial.println("AC PID: kP=" + String(ACkP, 3) + " kI=" + String(ACkI, 3) +
                 " kD=" + String(ACkD, 3));
  Serial.println("DataStorage initialized!");
  Serial.println();
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
  // Store to NVS
  Serial.print("Stored to NVS: ");
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
      preferences.putFloat("ac_voltage", value);
      ACVoltage = value;
    }
  }
}

void PrintStoredData() {
  Serial.println();
  Serial.println("=== Current Configuration (NVS) ===");

  Serial.println("[ DC PID ]");
  Serial.print("  kP: ");
  Serial.println(DCkP, 3);
  Serial.print("  kI: ");
  Serial.println(DCkI, 3);
  Serial.print("  kD: ");
  Serial.println(DCkD, 3);
  Serial.print("  Setpoint: ");
  Serial.println(DCsetpoint, 1);

  Serial.println("[ AC PID ]");
  Serial.print("  kP: ");
  Serial.println(ACkP, 3);
  Serial.print("  kI: ");
  Serial.println(ACkI, 3);
  Serial.print("  kD: ");
  Serial.println(ACkD, 3);
  Serial.print("  Setpoint: ");
  Serial.println(ACsetpoint, 1);

  Serial.println("============================");
  Serial.println();
}
