// ========================================
// ENCODER DEBUG & LOGGING TEST PROGRAM
// Upload this to test encoder reading
// ========================================

#include "PinConfig.h"
#include <Arduino.h>


// Encoder pins
const int CLK_PIN = ENCODER_CLK;
const int DT_PIN = ENCODER_DT;
const int SW_PIN = ENCODER_SW;

// Encoder state
volatile int encoderPos = 0;
volatile int triggerCount = 0;
volatile unsigned long lastTriggerTime = 0;

// Logging variables
int lastPrintedPos = 0;
unsigned long lastPrintTime = 0;
const unsigned long PRINT_INTERVAL = 100; // Print every 100ms

// ========================================
// SIMPLE ENCODER ISR
// ========================================

void IRAM_ATTR encoderISR() {
  unsigned long now = millis();

  // Read pins
  int clkState = digitalRead(CLK_PIN);
  int dtState = digitalRead(DT_PIN);

  // Count triggers for debugging
  triggerCount++;

  // Debounce: minimum 2ms between valid reads
  if (now - lastTriggerTime < 2) {
    return;
  }

  // Simple edge detection
  static int lastCLK = HIGH;

  if (lastCLK == HIGH && clkState == LOW) {
    // Falling edge on CLK
    if (dtState == HIGH) {
      encoderPos++;
      Serial.print("+"); // Visual feedback
    } else {
      encoderPos--;
      Serial.print("-"); // Visual feedback
    }
    lastTriggerTime = now;
  }

  lastCLK = clkState;
}

// ========================================
// BUTTON ISR
// ========================================

void IRAM_ATTR buttonISR() {
  static unsigned long lastButtonTime = 0;
  unsigned long now = millis();

  if (now - lastButtonTime > 200) {
    bool pressed = !digitalRead(SW_PIN);
    if (pressed) {
      Serial.println("\n[BUTTON PRESSED]");
      encoderPos = 0; // Reset position on button press
      Serial.println("Position RESET to 0");
    }
    lastButtonTime = now;
  }
}

// ========================================
// SETUP
// ========================================

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n========================================");
  Serial.println("ENCODER DEBUG & LOGGING TEST");
  Serial.println("========================================");
  Serial.printf("CLK Pin: %d\n", CLK_PIN);
  Serial.printf("DT Pin: %d\n", DT_PIN);
  Serial.printf("SW Pin: %d\n", SW_PIN);
  Serial.println("========================================");
  Serial.println("Rotate encoder to test...");
  Serial.println("Press button to reset position");
  Serial.println("+ = Clockwise, - = Counter-clockwise");
  Serial.println("========================================\n");

  // Setup pins
  pinMode(CLK_PIN, INPUT_PULLUP);
  pinMode(DT_PIN, INPUT_PULLUP);
  pinMode(SW_PIN, INPUT_PULLUP);

  delay(50);

  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(CLK_PIN), encoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SW_PIN), buttonISR, CHANGE);

  Serial.println("Ready! Start rotating encoder...\n");
}

// ========================================
// LOOP
// ========================================

void loop() {
  unsigned long now = millis();

  // Periodic detailed logging
  if (now - lastPrintTime >= PRINT_INTERVAL) {
    // Only print if position changed
    if (encoderPos != lastPrintedPos) {
      int diff = encoderPos - lastPrintedPos;

      Serial.println(); // New line after +/- symbols
      Serial.println("----------------------------------------");
      Serial.printf("Time: %lu ms\n", now);
      Serial.printf("Position: %d (Change: %+d)\n", encoderPos, diff);
      Serial.printf("Total Triggers: %d\n", triggerCount);
      Serial.printf("Trigger Rate: %.1f Hz\n", triggerCount / (now / 1000.0));

      // Read raw pin states
      int clkRaw = digitalRead(CLK_PIN);
      int dtRaw = digitalRead(DT_PIN);
      int swRaw = digitalRead(SW_PIN);

      Serial.printf("Pin States: CLK=%d, DT=%d, SW=%d\n", clkRaw, dtRaw, swRaw);
      Serial.println("----------------------------------------\n");

      lastPrintedPos = encoderPos;
    }

    lastPrintTime = now;
  }

  // Continuous raw pin monitoring (optional, for very detailed debug)
  // Uncomment below to see raw pin changes in real-time
  /*
  static int lastCLK = -1, lastDT = -1, lastSW = -1;
  int clk = digitalRead(CLK_PIN);
  int dt = digitalRead(DT_PIN);
  int sw = digitalRead(SW_PIN);

  if (clk != lastCLK || dt != lastDT || sw != lastSW) {
    Serial.printf("RAW: CLK=%d DT=%d SW=%d Time=%lu\n", clk, dt, sw, millis());
    lastCLK = clk;
    lastDT = dt;
    lastSW = sw;
  }
  */

  delay(10);
}
