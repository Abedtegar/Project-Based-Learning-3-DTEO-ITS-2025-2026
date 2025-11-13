#ifndef ENCODER_CONTROL_H
#define ENCODER_CONTROL_H

#include <Arduino.h>

// ========================================
// ROTARY ENCODER CONTROL CLASS
// Untuk encoder 2 channel (CLK, DT) + Switch
// ========================================

class EncoderControl {
private:
  // Pin definitions
  int pinCLK;
  int pinDT;
  int pinSW;

  // Encoder state
  volatile int encoderPos;
  volatile int lastEncoded;
  volatile bool buttonPressed;
  volatile unsigned long lastInterruptTime;

  // Button debouncing
  unsigned long lastButtonPress;
  unsigned long lastButtonRelease;
  unsigned long buttonPressTime;
  unsigned long debounceDelay;
  unsigned long longPressThreshold;
  bool lastButtonState;
  bool buttonHandled;

  // Callbacks
  void (*onRotateCallback)(int direction);
  void (*onClickCallback)();
  void (*onLongPressCallback)();

  // Static instance untuk ISR
  static EncoderControl *instance;

  // ISR handlers
  static void IRAM_ATTR handleEncoderISR();
  static void IRAM_ATTR handleButtonISR();

public:
  EncoderControl(int clkPin, int dtPin, int swPin);

  // Initialization
  void begin();

  // Set callbacks
  void onRotate(void (*callback)(int direction));
  void onClick(void (*callback)());
  void onLongPress(void (*callback)());

  // Update function (call in loop)
  void update();

  // Getters
  int getPosition() const { return encoderPos; }
  void resetPosition() { encoderPos = 0; }
  bool isButtonPressed() const { return buttonPressed; }

  // Manual read (non-interrupt mode)
  int readRotation();
  bool readButton();
};

#endif // ENCODER_CONTROL_H
