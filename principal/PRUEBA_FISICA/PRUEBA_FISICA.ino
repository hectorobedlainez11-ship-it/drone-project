/*
   PRUEBA ESCs - Solo Arduino UNO + 4 ESCs
   Sin MPU6500, sin ESP32, sin potenciometros.
   Control por Serial Monitor.

   CONEXIONES:
     Señal (blanco/naranja):
       D3 --> ESC1 (señal)
       D5 --> ESC2 (señal)
       D6 --> ESC3 (señal)
       D9 --> ESC4 (señal)

     Alimentación ESCs:
       Rojo (+) -> Power HUB BAT+ (o directo a batería)
       Negro (-) -> Power HUB BAT- (GND)
       GND del Power HUB -> GND del Arduino (obligatorio)

   CALIBRACIÓN DE ESCs (hacer una vez):
     1. Desconectar batería
     2. Enviar THR 2000 por Serial
     3. Conectar batería (ESC emite tonos)
     4. Enviar THR 1000 (ESC emite tonos de confirmación)
     5. ESCs calibrados

   COMANDOS POR SERIAL (57600 baud):
     THR 1500     -> Todos los ESCs al mismo valor (1000-2000)
     MIX a b c d  -> Cada ESC individual (ej: MIX 1000 1200 1400 1600)
     ARM          -> Activa modo armado (envía señales a ESCs)
     DIS          -> Desactiva modo armado (señal 1000 en todos)
*/

#define pin_motor1 3
#define pin_motor2 5
#define pin_motor3 6
#define pin_motor4 9
#define PWM_PERIOD 20000
#define PULSE_MIN 1000
#define PULSE_MAX 2000
#define PULSE_SAFE 1000

volatile unsigned long pwm_start_time;
unsigned long ESC1_us = PULSE_SAFE, ESC2_us = PULSE_SAFE, ESC3_us = PULSE_SAFE, ESC4_us = PULSE_SAFE;
bool armado = false;
unsigned long loop_timer;

void setup() {
  Serial.begin(57600);
  Serial.println(F("=== PRUEBA ESCs - Arduino UNO + 4 ESCs ==="));

  pinMode(pin_motor1, OUTPUT);
  pinMode(pin_motor2, OUTPUT);
  pinMode(pin_motor3, OUTPUT);
  pinMode(pin_motor4, OUTPUT);

  digitalWrite(pin_motor1, LOW);
  digitalWrite(pin_motor2, LOW);
  digitalWrite(pin_motor3, LOW);
  digitalWrite(pin_motor4, LOW);

  Serial.println(F("Espera 3 segundos para que los ESCs inicialicen..."));
  for (int i = 3; i > 0; i--) {
    Serial.print(i);
    Serial.println(F("..."));
    delay(1000);
  }

  Serial.println(F("Listo. Envia comandos: THR 1500 | MIX a b c d | ARM | DIS"));
  Serial.println(F("PRECAUCION: Quita la helice antes de probar!"));

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
      if (c == '\r') continue;
      buf[len++] = c;
    }
    buf[len] = '\0';

    if (len == 0) { /* empty line, ignore */ }
    else if (strncmp(buf, "THR ", 4) == 0) {
      unsigned long v = constrain(atol(buf + 4), PULSE_MIN, PULSE_MAX);
      ESC1_us = v; ESC2_us = v; ESC3_us = v; ESC4_us = v;
      Serial.print(F("THR ")); Serial.println(v);
    }
    else if (strncmp(buf, "MIX ", 4) == 0) {
      int a, b, c, d;
      if (sscanf(buf + 4, "%d %d %d %d", &a, &b, &c, &d) == 4) {
        ESC1_us = constrain(a, PULSE_MIN, PULSE_MAX);
        ESC2_us = constrain(b, PULSE_MIN, PULSE_MAX);
        ESC3_us = constrain(c, PULSE_MIN, PULSE_MAX);
        ESC4_us = constrain(d, PULSE_MIN, PULSE_MAX);
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
      ESC1_us = PULSE_SAFE; ESC2_us = PULSE_SAFE; ESC3_us = PULSE_SAFE; ESC4_us = PULSE_SAFE;
      Serial.println(F("DESARMADO"));
    }
    else {
      Serial.println(F("Comandos: THR 1500 | MIX 1000 1200 1400 1600 | ARM | DIS"));
    }
  }

  if (!armado) {
    ESC1_us = PULSE_SAFE; ESC2_us = PULSE_SAFE; ESC3_us = PULSE_SAFE; ESC4_us = PULSE_SAFE;
  }

  loop_timer = micros();
  PWM_begin();
  while (micros() - pwm_start_time < PULSE_MAX + 100) {
    unsigned long dt = micros() - pwm_start_time;
    if (dt >= ESC1_us) digitalWrite(pin_motor1, LOW);
    if (dt >= ESC2_us) digitalWrite(pin_motor2, LOW);
    if (dt >= ESC3_us) digitalWrite(pin_motor3, LOW);
    if (dt >= ESC4_us) digitalWrite(pin_motor4, LOW);
  }
  while (micros() - pwm_start_time < PWM_PERIOD);
}
