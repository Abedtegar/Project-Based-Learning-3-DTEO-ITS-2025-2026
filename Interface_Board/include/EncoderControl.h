#ifndef ENCODER_CONTROL_H
#define ENCODER_CONTROL_H

#include <Arduino.h>

// Button click types
enum ButtonClick {
  CLICK_NONE = 0,
  CLICK_SINGLE = 1,
  CLICK_DOUBLE = 2,
  CLICK_LONG = 3
};

// Encoder mode
enum EncoderMode { ENCODER_UP = 0, ENCODER_DOWN = 1 };

extern volatile long DCencoder;
extern volatile long DClastEncoder;
extern volatile bool DCDirection;
extern volatile bool DCswitch;
extern bool scroll;

void DCinitEncoder();     // setup encoder DC
long DCgetEncoderCount(); // dapatkan jumlah encoder DC
bool DCgetSwitchState();  // dapatkan status switch encoder
bool DCgetDirection();    // dapatkan arah putaran encoder (deprecated, gunakan
                          // mode)
EncoderMode getEncoderMode(); // dapatkan mode encoder (UP/DOWN)
void toggleEncoderMode();     // toggle mode encoder

// Button detection functions
ButtonClick getButtonClick(); // deteksi jenis klik (single/double/long)
bool buttonPressed();         // cek apakah tombol encoder ditekan
bool buttonLongPress();       // deprecated - gunakan getButtonClick()
bool newscroll();
#endif
