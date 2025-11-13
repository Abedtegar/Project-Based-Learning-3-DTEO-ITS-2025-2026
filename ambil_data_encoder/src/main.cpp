#include <Arduino.h>
#include <PlantConfig.h>

volatile long encoder = 0;
volatile long lastEncoder = 0;
volatile float rpm = 0;
volatile long pulseCount = 0;
volatile bool dataready = false;
hw_timer_t *m_timer = NULL;

void IRAM_ATTR onTimer() {
  digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
  pulseCount = encoder - lastEncoder;
  lastEncoder = encoder;
  rpm =
      (pulseCount * (60000.0 / DCREAD_INTERVAL)) / PPR; // pembacaan tiap 10 ms
  dataready = true;
}

void IRAM_ATTR handleEncodera_rising() {
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA == HIGH && stateB == LOW) {
    encoder++;
  } else if (stateA == HIGH && stateB == HIGH) {
    encoder--;
  }
}
void IRAM_ATTR handleEncoderb_rising() {
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA == HIGH && stateB == HIGH) {
    encoder++;
  } else if (stateA == LOW && stateB == HIGH) {
    encoder--;
  }
}

void IRAM_ATTR handleEncodera_falling() {
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA == LOW && stateB == HIGH) {
    encoder++;
  } else if (stateA == LOW && stateB == LOW) {
    encoder--;
  }
}
void IRAM_ATTR handleEncoderb_falling() {
  int stateA = digitalRead(ENCODER_A_PIN);
  int stateB = digitalRead(ENCODER_B_PIN);

  if (stateA == LOW && stateB == LOW) {
    encoder++;
  } else if (stateA == HIGH && stateB == LOW) {
    encoder--;
  }
}

void setup() {
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_PID, OUTPUT);
  pinMode(LED_DC_CONTROL, OUTPUT);
  pinMode(LED_AC_CONTROL, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_ESPNOW, OUTPUT);
  pinMode(ENCODER_A_PIN, INPUT_PULLUP);
  pinMode(ENCODER_B_PIN, INPUT_PULLUP);
  pinMode(MOTOR_DIR_PIN, OUTPUT);
  digitalWrite(MOTOR_DIR_PIN, HIGH);
  m_timer = timerBegin(0, 80, true); // Timer 0, prescaler 80, count up
  timerAlarmWrite(m_timer, DCREAD_INTERVAL * 1000,
                  true); // 10 ms alarm, autoreload true
  timerAttachInterrupt(m_timer, &onTimer, true);
  timerAlarmEnable(m_timer); // Enable the alarm

  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), handleEncodera_rising,
                  RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), handleEncoderb_rising,
                  RISING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_A_PIN), handleEncodera_falling,
                  FALLING);
  attachInterrupt(digitalPinToInterrupt(ENCODER_B_PIN), handleEncoderb_falling,
                  FALLING);
  ledcSetup(PWM_CHANNEL, PWM_FREQ, PWM_RESOLUTION);
  ledcAttachPin(MOTOR_PWM_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 255); // speed 0 - 255

  Serial.begin(115200);
}

void loop() {
  if (dataready) {
    Serial.print(encoder);
    Serial.print(" , ");
    Serial.print(pulseCount);
    Serial.print(" , ");
    Serial.println(rpm);
    dataready = false;
  }
}
