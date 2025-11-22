#include "EncoderControl.h"
#include "PinConfig.h"
volatile bool DCswitch;
volatile long DCencoder = 0;
volatile long DClastEncoder = 0;
volatile bool DCDirection;
static volatile unsigned long DCswitchLastISR = 0;

// Encoder mode (UP atau DOWN)
static EncoderMode encoderMode = ENCODER_UP;

void IRAM_ATTR DChandleEncoder() {


  if (encoderMode == ENCODER_UP) {
    DCencoder++;
    DCDirection = true;
  } else {
    DCencoder--;
    DCDirection = false;
  }

}

void IRAM_ATTR DChandleEncoderSW() {

}

void DCinitEncoder() {
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);
  pinMode(ENCODER_SW, INPUT_PULLUP);

  DCencoder = 0;
  DClastEncoder = 0;
  DCswitchLastISR = 0;
  DCDirection = false;

  // HANYA attach interrupt ke DT dengan FALLING edge
  // Jangan attach ke CLK - akan menyebabkan double trigger!
  attachInterrupt(digitalPinToInterrupt(ENCODER_DT), DChandleEncoder, FALLING);
  // TIDAK attach interrupt ke SW - gunakan polling untuk lebih reliable
  // attachInterrupt(digitalPinToInterrupt(ENCODER_SW), DChandleEncoderSW,
  // CHANGE);

  Serial.println("Encoder initialized");
  Serial.print("CLK Pin: ");
  Serial.println(ENCODER_CLK);
  Serial.print("DT Pin: ");
  Serial.println(ENCODER_DT);
  Serial.print("SW Pin: ");
  Serial.println(ENCODER_SW);
}

long DCgetEncoderCount() { return DCencoder; }

bool DCgetSwitchState() { return digitalRead(ENCODER_SW); }

bool DCgetDirection() { return DCDirection; }

EncoderMode getEncoderMode() { return encoderMode; }

void toggleEncoderMode() {
  if (encoderMode == ENCODER_UP) {
    encoderMode = ENCODER_DOWN;
    Serial.println("[Encoder Mode] DOWN");
  } else {
    encoderMode = ENCODER_UP;
    Serial.println("[Encoder Mode] UP");
  }
}

bool buttonPressed() { return (digitalRead(ENCODER_SW) == LOW); }

ButtonClick getButtonClick() {
  static unsigned long pressStartTime = 0;
  static unsigned long releaseTime = 0;
  static bool wasPressed = false;
  static bool longPressTriggered = false;
  static int clickCount = 0;

  const unsigned long LONG_PRESS_TIME = 1000; // 1 detik untuk long press
  const unsigned long DOUBLE_CLICK_TIME =
      400;                                // 400ms window untuk double click
  const unsigned long DEBOUNCE_TIME = 50; // 50ms debounce

  bool isPressed = buttonPressed();
  unsigned long now = millis();

  // Deteksi button press (transisi LOW)
  if (isPressed && !wasPressed) {
    if (now - releaseTime < DEBOUNCE_TIME) {
      // Bounce, abaikan
      wasPressed = isPressed;
      return CLICK_NONE;
    }

    pressStartTime = now;
    longPressTriggered = false;
    clickCount++;

    wasPressed = true;
  }

  // Deteksi long press saat masih ditekan
  if (isPressed && !longPressTriggered) {
    if (now - pressStartTime >= LONG_PRESS_TIME) {
      longPressTriggered = true;
      clickCount = 0;
      wasPressed = isPressed;
      Serial.println("[Button] LONG PRESS");
      return CLICK_LONG;
    }
  }

  // Deteksi button release (transisi HIGH)
  if (!isPressed && wasPressed) {
    releaseTime = now;
    wasPressed = false;

    // Jika long press sudah trigger, reset
    if (longPressTriggered) {
      clickCount = 0;
      return CLICK_NONE;
    }
  }

  // Evaluasi click setelah timeout double click
  if (clickCount > 0 && !isPressed &&
      (now - releaseTime >= DOUBLE_CLICK_TIME)) {
    ButtonClick result = CLICK_NONE;

    if (clickCount == 1) {
      result = CLICK_SINGLE;
      Serial.println("[Button] SINGLE CLICK");
    } else if (clickCount >= 2) {
      result = CLICK_DOUBLE;
      Serial.println("[Button] DOUBLE CLICK");
    }

    clickCount = 0;
    return result;
  }

  return CLICK_NONE;
}

// Backward compatibility
bool buttonLongPress() { return (getButtonClick() == CLICK_LONG); }

bool newscroll() {
  if (DClastEncoder != DCencoder) {
    DClastEncoder = DCencoder;
    return true;
  }
  return false;
}