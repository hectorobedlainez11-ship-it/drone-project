/*
   PRUEBA FISICA - Solo Arduino UNO + LEDs
   Sin MPU6500, sin potenciometros.
   Control por Serial Monitor.

   CONEXIONES:
     D3 --> LED1 (M1) + 220ohm a GND
     D5 --> LED2 (M2) + 220ohm a GND
     D6 --> LED3 (M3) + 220ohm a GND
     D9 --> LED4 (M4) + 220ohm a GND
     D10 --> LED_STATUS1 (armado)
     D13 --> LED_STATUS2 (heartbeat)

   COMANDOS POR SERIAL:
     THR 1500     -> Todos los LEDs al mismo valor (1000-2000)
     MIX a b c d  -> Cada LED individual (ej: MIX 1000 1200 1400 1600)
     ARM          -> Activa modo armado
     DIS          -> Desactiva modo armado
*/

#define pin_motor1 3
#define pin_motor2 5
#define pin_motor3 6
#define pin_motor4 9
#define pin_status1 10
#define pin_status2 13

#define PWM_PERIOD 6000

volatile unsigned long pwm_start_time;
unsigned long ESC1_us = 1000, ESC2_us = 1000, ESC3_us = 1000, ESC4_us = 1000;
bool armado = false;
unsigned long loop_timer;

void setup() {
  Serial.begin(57600);
  Serial.println(F("=== PRUEBA FISICA - Solo Arduino UNO + LEDs ==="));

  pinMode(pin_motor1, OUTPUT);
  pinMode(pin_motor2, OUTPUT);
  pinMode(pin_motor3, OUTPUT);
  pinMode(pin_motor4, OUTPUT);
  pinMode(pin_status1, OUTPUT);
  pinMode(pin_status2, OUTPUT);

  digitalWrite(pin_motor1, LOW);
  digitalWrite(pin_motor2, LOW);
  digitalWrite(pin_motor3, LOW);
  digitalWrite(pin_motor4, LOW);
  digitalWrite(pin_status1, LOW);
  digitalWrite(pin_status2, LOW);

  Serial.println(F("Test de LEDs:"));
  for (int i = 0; i < 3; i++) {
    digitalWrite(pin_motor1, HIGH); delay(120); digitalWrite(pin_motor1, LOW);
    digitalWrite(pin_motor2, HIGH); delay(120); digitalWrite(pin_motor2, LOW);
    digitalWrite(pin_motor3, HIGH); delay(120); digitalWrite(pin_motor3, LOW);
    digitalWrite(pin_motor4, HIGH); delay(120); digitalWrite(pin_motor4, LOW);
  }
  Serial.println(F("Listo. Envia comandos: THR 1500 | MIX a b c d | ARM | DIS"));

  loop_timer = micros();
}

void PWM_begin() {
  pwm_start_time = micros();
  digitalWrite(pin_motor1, HIGH);
  digitalWrite(pin_motor2, HIGH);
  digitalWrite(pin_motor3, HIGH);
  digitalWrite(pin_motor4, HIGH);
}

void loop() {
  while (micros() - loop_timer < PWM_PERIOD);

  if (Serial.available() > 0) {
    char buf[32];
    int len = 0;
    while (Serial.available() > 0 && len < 31) {
      char c = Serial.read();
      if (c == '\n') break;
      buf[len++] = c;
    }
    buf[len] = '\0';

    if (strncmp(buf, "THR ", 4) == 0) {
      unsigned long v = constrain(atol(buf + 4), 1000, 2000);
      ESC1_us = v; ESC2_us = v; ESC3_us = v; ESC4_us = v;
      Serial.print(F("THR ")); Serial.println(v);
    }
    else if (strncmp(buf, "MIX ", 4) == 0) {
      int a, b, c, d;
      if (sscanf(buf + 4, "%d %d %d %d", &a, &b, &c, &d) == 4) {
        ESC1_us = constrain(a, 1000, 2000);
        ESC2_us = constrain(b, 1000, 2000);
        ESC3_us = constrain(c, 1000, 2000);
        ESC4_us = constrain(d, 1000, 2000);
        Serial.print(F("MIX "));
        Serial.print(ESC1_us); Serial.print(" ");
        Serial.print(ESC2_us); Serial.print(" ");
        Serial.print(ESC3_us); Serial.print(" ");
        Serial.println(ESC4_us);
      }
    }
    else if (strcmp(buf, "ARM") == 0) {
      armado = true;
      Serial.println(F("ARMADO"));
    }
    else if (strcmp(buf, "DIS") == 0) {
      armado = false;
      ESC1_us = 1000; ESC2_us = 1000; ESC3_us = 1000; ESC4_us = 1000;
      Serial.println(F("DESARMADO"));
    }
    else {
      Serial.println(F("Comandos: THR 1500 | MIX 1000 1200 1400 1600 | ARM | DIS"));
    }
  }

  if (!armado) {
    ESC1_us = 1000; ESC2_us = 1000; ESC3_us = 1000; ESC4_us = 1000;
  }

  loop_timer = micros();
  PWM_begin();
  while (micros() - pwm_start_time < 2100) {
    unsigned long dt = micros() - pwm_start_time;
    if (dt >= ESC1_us) digitalWrite(pin_motor1, LOW);
    if (dt >= ESC2_us) digitalWrite(pin_motor2, LOW);
    if (dt >= ESC3_us) digitalWrite(pin_motor3, LOW);
    if (dt >= ESC4_us) digitalWrite(pin_motor4, LOW);
  }
  while (micros() - pwm_start_time < PWM_PERIOD);

  digitalWrite(pin_status1, armado ? HIGH : LOW);
  digitalWrite(pin_status2, (millis() / 500) % 2);
}
