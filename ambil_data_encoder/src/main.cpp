#include <Arduino.h>
#include <PlantConfig.h>

volatile long encoder = 0;
volatile long lastEncoder = 0;
volatile float rpm = 0;
volatile long pulseCount = 0;
volatile bool dataready = false;
hw_timer_t *m_timer = NULL;
bool flip = false;
// Variables for speed and direction control
int motorSpeed = 125;       // Default speed (0-255)
bool motorDirection = HIGH; // Default direction

void IRAM_ATTR onTimer() {
  digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
  pulseCount = encoder - lastEncoder;
  lastEncoder = encoder;
  rpm =
      (pulseCount * (60000.0 / DCREAD_INTERVAL)) / PPR; // pembacaan tiap 10 ms
  dataready = true;
}

void IRAM_ATTR handleEncodera_rising() {
  // Serial.print("ISR A Rising called\n");
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA == HIGH && stateB == LOW) {
    encoder++;
  } else if (stateA == HIGH && stateB == HIGH) {
    encoder--;
  }
}
void IRAM_ATTR handleEncoderb_rising() {
  // Serial.print("ISR B Rising called\n");
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA == HIGH && stateB == HIGH) {
    encoder++;
  } else if (stateA == LOW && stateB == HIGH) {
    encoder--;
  }
}

void IRAM_ATTR handleEncodera_falling() {
  // Serial.print("ISR A Falling called\n");
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA == LOW && stateB == HIGH) {
    encoder++;
  } else if (stateA == LOW && stateB == LOW) {
    encoder--;
  }
}
void IRAM_ATTR handleEncoderb_falling() {
  // Serial.print("ISR B Falling called\n");
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA == LOW && stateB == LOW) {
    encoder++;
  } else if (stateA == HIGH && stateB == LOW) {
    encoder--;
  }
}

void acsweep() {
  for (int x = 0; x <= 255; x++) {
    Serial.print("DAC Value: ");
    Serial.println(x);

    if (x < 1) {
      dacDisable(AC_DAC1_PIN);
      pinMode(AC_DAC1_PIN, OUTPUT);

      digitalWrite(AC_DAC1_PIN, LOW);
      delay(5000);
    } else if (x > 254) {
      dacDisable(AC_DAC1_PIN);
      pinMode(AC_DAC1_PIN, OUTPUT);

      digitalWrite(AC_DAC1_PIN, HIGH);
      delay(5000);
    } else {
      dacWrite(AC_DAC1_PIN, x);
      delay(10);
    }
  }
  Serial.println("AC sweep complete");
}

void setup() {
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_PID, OUTPUT);
  pinMode(LED_DC_CONTROL, OUTPUT);
  pinMode(LED_AC_CONTROL, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_ESPNOW, OUTPUT);
  pinMode(ENCODER_A_PIN, INPUT);
  pinMode(ENCODER_B_PIN, INPUT);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  digitalWrite(MOTOR_DIR_PIN, motorDirection);
  m_timer = timerBegin(0, 80, true); // Timer 0, prescaler 80, count up
  timerAlarmWrite(m_timer, DCREAD_INTERVAL * 1000,
                  true); // 10 ms alarm, autoreload true
  timerAttachInterrupt(m_timer, &onTimer, true);
  timerAlarmEnable(m_timer); // Enable the alarm

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN),handleEncodera_rising,RISING);
  // attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN),handleEncoderb_rising,RISING);
  // attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN),handleEncodera_falling,FALLING);
  // attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), handleEncoderb_falling,FALLING);

  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, motorSpeed); // speed 0 - 255

  Serial.begin(115200);
  Serial.println("Motor Control Ready!");
  Serial.println("Commands:");
  Serial.println("  s<value> - Set speed (0-255), e.g., s150");
  Serial.println("  d<value> - Set direction (0 or 1), e.g., d1");
  pinMode(AC_DAC1_PIN, OUTPUT);
  pinMode(AC_DAC_SOURCE_PIN, OUTPUT);
  pinMode(AC_DAC_VOLTAGE_SELECT_PIN, OUTPUT);
  digitalWrite(AC_DAC_SOURCE_PIN, LOW);
  digitalWrite(AC_DAC_VOLTAGE_SELECT_PIN, HIGH);
}

void loop() {
  // // acsweep();
  // // delay(1000);
  // // float analogValue = analogRead(AC_ENCODER_PIN);
  // // Serial.println(analogValue);
  // if (Serial.available() > 0) {
  //   String input = Serial.readStringUntil('\n');
  //   input.trim();

  //   if (input.length() > 0) {
  //     char command = input.charAt(0);
  //     int value = input.substring(1).toInt();

  //     if (command == 's' || command == 'S') {
  //       // Set speed
  //       if (value >= 0 && value <= 255) {
  //         motorSpeed = value;
  //         ledcWrite(PWM_CHANNEL, motorSpeed);
  //         Serial.print("Speed set to: ");
  //         Serial.println(motorSpeed);
  //       } else {
  //         Serial.println("Error: Speed must be 0-255");
  //       }
  //     } else if (command == 'd' || command == 'D') {
  //       // Set direction
  //       if (value == 0 || value == 1) {
  //         motorDirection = (value == 1) ? HIGH : LOW;
  //         digitalWrite(MOTOR_DIR_PIN, motorDirection);
  //         Serial.print("Direction set to: ");
  //         Serial.println(motorDirection == HIGH ? "HIGH (1)" : "LOW (0)");
  //       } else {
  //         Serial.println("Error: Direction must be 0 or 1");
  //       }
  //     } else {
  //       Serial.println("Unknown command. Use s<value> for speed or d<value> "
  //                      "for direction");
  //     }
  //   }
  // }

  if (dataready) {
    if (digitalRead(MOTOR_DIR_PIN) == HIGH) {
      Serial.print("Direction: HIGH , ");
    } else {
      Serial.print("Direction: LOW , ");
    }
    Serial.print(encoder);
    Serial.print(" , ");
    Serial.print(pulseCount);
    Serial.print(" , ");
    Serial.println(rpm);
    dataready = false;
  }

  static unsigned long lastSend = 0;
  if (millis() - lastSend > 10000) { // Send every 10 seconds
    lastSend = millis();
    flip = !flip;
    if (!flip) {
      digitalWrite(MOTOR_DIR_PIN, !digitalRead(MOTOR_DIR_PIN));
      delay(100);
    }
  }
  if (flip) {
    ledcWrite(PWM_CHANNEL, 0);
  } else {
    if (!flip) {
      ledcWrite(PWM_CHANNEL, 250);
    }
  }
  
}
