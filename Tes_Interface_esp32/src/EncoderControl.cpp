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

  // Read initial state
  int MSB = digitalRead(pinCLK);
  int LSB = digitalRead(pinDT);
  lastEncoded = (MSB << 1) | LSB;

  // Attach interrupts
  attachInterrupt(digitalPinToInterrupt(pinCLK), handleEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinDT), handleEncoderISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(pinSW), handleButtonISR, CHANGE);
}

// ========================================
// INTERRUPT SERVICE ROUTINES
// ========================================

void IRAM_ATTR EncoderControl::handleEncoderISR() {
  if (!instance)
    return;

  // Debounce - ignore interrupts too close together
  unsigned long currentTime = millis();
  if (currentTime - instance->lastInterruptTime < 5) {
    return;
  }
  instance->lastInterruptTime = currentTime;

  // Read current state
  int MSB = digitalRead(instance->pinCLK);
  int LSB = digitalRead(instance->pinDT);
  int encoded = (MSB << 1) | LSB;

  int sum = (instance->lastEncoded << 2) | encoded;

  // Determine direction
  if (sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011) {
    instance->encoderPos++;
  } else if (sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000) {
    instance->encoderPos--;
  }

  instance->lastEncoded = encoded;
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
  // Check for rotation
  static int lastReportedPos = 0;

  if (encoderPos != lastReportedPos) {
    int direction = (encoderPos > lastReportedPos) ? 1 : -1;

    // Call callback
    if (onRotateCallback) {
      onRotateCallback(direction);
    }

    lastReportedPos = encoderPos;
  }

  // Check button state dengan debouncing
  bool currentButtonState = !digitalRead(pinSW); // LOW when pressed (pullup)
  unsigned long currentTime = millis();

  if (currentButtonState != lastButtonState) {
    if (currentButtonState) {
      // Button just pressed
      if (currentTime - lastButtonRelease > debounceDelay) {
        buttonPressTime = currentTime;
        lastButtonPress = currentTime;
        buttonHandled = false;
      }
    } else {
      // Button just released
      if (currentTime - lastButtonPress > debounceDelay) {
        lastButtonRelease = currentTime;

        unsigned long pressDuration = currentTime - buttonPressTime;

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
    unsigned long pressDuration = currentTime - buttonPressTime;

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
