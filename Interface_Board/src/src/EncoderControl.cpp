#include "EncoderControl.h"

// Initialize static instance pointer
EncoderControl *EncoderControl::instance = nullptr;

// ========================================
// CONSTRUCTOR
// ========================================

EncoderControl::EncoderControl(int clkPin, int dtPin, int swPin) {
  pinCLK = clkPin;
  pinDT = dtPin;
  pinSW = swPin;

  encoderPos = 0;
  lastEncoded = 0;
  buttonPressed = false;
  lastInterruptTime = 0;

  lastButtonPress = 0;
  lastButtonRelease = 0;
  buttonPressTime = 0;
  debounceDelay = 50;       // 50ms debounce
  longPressThreshold = 800; // 800ms untuk long press
  lastButtonState = HIGH;
  buttonHandled = false;

  onRotateCallback = nullptr;
  onClickCallback = nullptr;
  onLongPressCallback = nullptr;

  // Set static instance untuk ISR
  instance = this;
}

// ========================================
// INITIALIZATION
// ========================================

void EncoderControl::begin() {
  // Setup pins
  pinMode(pinCLK, INPUT_PULLUP);
  pinMode(pinDT, INPUT_PULLUP);
  pinMode(pinSW, INPUT_PULLUP);

  // Small delay to stabilize
  delay(10);

  // Attach interrupts - only on CLK for simpler detection
  attachInterrupt(digitalPinToInterrupt(pinCLK), handleEncoderISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinSW), handleButtonISR, CHANGE);
}

// ========================================
// INTERRUPT SERVICE ROUTINES
// ========================================

void IRAM_ATTR EncoderControl::handleEncoderISR() {
  if (!instance)
    return;

  // Debounce - ignore interrupts too close together (increased to 2ms)
  unsigned long LEDCurrentTime = millis();
  if (LEDCurrentTime - instance->lastInterruptTime < 2) {
    return;
  }

  // Read current state
  int clkState = digitalRead(instance->pinCLK);
  int dtState = digitalRead(instance->pinDT);

  // Simple logic: only process on CLK falling edge
  // This reduces false triggers significantly
  static int lastCLK = HIGH;

  if (lastCLK == HIGH && clkState == LOW) {
    // CLK went from HIGH to LOW (falling edge)
    // Check DT to determine direction
    if (dtState == HIGH) {
      instance->encoderPos++; // Clockwise
    } else {
      instance->encoderPos--; // Counter-clockwise
    }
    instance->lastInterruptTime = LEDCurrentTime;
  }

  lastCLK = clkState;
}

void IRAM_ATTR EncoderControl::handleButtonISR() {
  if (!instance)
    return;

  // Simple flag, debouncing handled in update()
  instance->buttonPressed = !digitalRead(instance->pinSW);
}

// ========================================
// CALLBACK SETTERS
// ========================================

void EncoderControl::onRotate(void (*callback)(int direction)) {
  onRotateCallback = callback;
}

void EncoderControl::onClick(void (*callback)()) { onClickCallback = callback; }

void EncoderControl::onLongPress(void (*callback)()) {
  onLongPressCallback = callback;
}

// ========================================
// UPDATE FUNCTION (call in loop)
// ========================================

void EncoderControl::update() {
  // Check for rotation with threshold to reduce jitter
  static int lastReportedPos = 0;
  static unsigned long lastRotateReport = 0;

  int posDiff = encoderPos - lastReportedPos;

  // Only report if position changed AND enough time has passed (debounce in
  // software)
  if (posDiff != 0 && millis() - lastRotateReport > 10) {
    int direction = (posDiff > 0) ? 1 : -1;

    // Call callback
    if (onRotateCallback) {
      onRotateCallback(direction);
    }

    lastReportedPos = encoderPos;
    lastRotateReport = millis();
  }

  // Check button state dengan debouncing
  bool currentButtonState = !digitalRead(pinSW); // LOW when pressed (pullup)
  unsigned long LEDCurrentTime = millis();

  if (currentButtonState != lastButtonState) {
    if (currentButtonState) {
      // Button just pressed
      if (LEDCurrentTime - lastButtonRelease > debounceDelay) {
        buttonPressTime = LEDCurrentTime;
        lastButtonPress = LEDCurrentTime;
        buttonHandled = false;
      }
    } else {
      // Button just released
      if (LEDCurrentTime - lastButtonPress > debounceDelay) {
        lastButtonRelease = LEDCurrentTime;

        unsigned long pressDuration = LEDCurrentTime - buttonPressTime;

        if (!buttonHandled) {
          if (pressDuration >= longPressThreshold) {
            // Long press detected
            if (onLongPressCallback) {
              onLongPressCallback();
            }
          } else if (pressDuration > debounceDelay) {
            // Short click detected
            if (onClickCallback) {
              onClickCallback();
            }
          }
          buttonHandled = true;
        }
      }
    }

    lastButtonState = currentButtonState;
  } else if (currentButtonState && !buttonHandled) {
    // Button is being held
    unsigned long pressDuration = LEDCurrentTime - buttonPressTime;

    if (pressDuration >= longPressThreshold) {
      // Long press threshold reached while holding
      if (onLongPressCallback) {
        onLongPressCallback();
      }
      buttonHandled = true;
    }
  }
}

// ========================================
// MANUAL READ FUNCTIONS
// ========================================

int EncoderControl::readRotation() {
  static int lastPos = 0;
  int currentPos = encoderPos;

  if (currentPos != lastPos) {
    int direction = (currentPos > lastPos) ? 1 : -1;
    lastPos = currentPos;
    return direction;
  }

  return 0;
}

bool EncoderControl::readButton() { return !digitalRead(pinSW); }
