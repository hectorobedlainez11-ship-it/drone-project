/*
   DRONE DESDE CERO - v2.0
   Wokwi Simulation Version
   CHANGES vs original SOFTWARE_PRINCIPAL.ino:
   - RC inputs from potentiometers A0-A3 (instead of UART/ESP32)
   - Serial output only (no UART parsing)
   - All flight controller logic unchanged
*/

#define usCiclo 20000

#define pin_motor1 3
#define pin_motor2 5
#define pin_motor3 6
#define pin_motor4 9
#define pin_LED_rojo1 10
#define pin_LED_rojo2 13

#define MPU6500_adress 0x68

#include <Wire.h>

// PID gains
float Roll_ang_Kp  = 0.5, Roll_ang_Ki  = 0.05, Roll_ang_Kd  = 10;
float Pitch_ang_Kp = 0.5, Pitch_ang_Ki = 0.05, Pitch_ang_Kd = 10;
float Pitch_W_Kp   = 2,   Pitch_W_Ki   = 0.02, Pitch_W_Kd   = 0;
float Roll_W_Kp    = 2,   Roll_W_Ki    = 0.02, Roll_W_Kd    = 0;
float Yaw_W_Kp     = 1,   Yaw_W_Ki     = 0.05, Yaw_W_Kd     = 0;

int PID_W_sat1   = 380;
int PID_W_sat2   = 380;
int PID_ang_sat1 = 130;
int PID_ang_sat2 = 130;

float PID_ang_Pitch_error, PID_ang_Pitch_P, PID_ang_Pitch_I, PID_ang_Pitch_D, PID_ang_Pitch_OUT;
float PID_ang_Roll_error, PID_ang_Roll_P, PID_ang_Roll_I, PID_ang_Roll_D, PID_ang_Roll_OUT;
float PID_W_Pitch_error, PID_W_Pitch_P, PID_W_Pitch_I, PID_W_Pitch_D, PID_W_Pitch_OUT;
float PID_W_Roll_error, PID_W_Roll_P, PID_W_Roll_I, PID_W_Roll_D, PID_W_Roll_OUT;
float PID_W_Yaw_error, PID_W_Yaw_P, PID_W_Yaw_I, PID_W_Yaw_D, PID_W_Yaw_OUT;
float PID_W_Pitch_consigna, PID_W_Roll_consigna;

// MPU6500
float angulo_pitch, angulo_roll, angulo_pitch_acc, angulo_roll_acc;
float angulo_pitch_ant, angulo_roll_ant;
int gx, gy, gz, gyro_Z, gyro_X, gyro_Y, gyro_X_ant, gyro_Y_ant, gyro_Z_ant;
float gyro_X_cal, gyro_Y_cal, gyro_Z_cal;
float ax, ay, az, acc_X_cal, acc_Y_cal, acc_Z_cal, acc_total_vector;
bool set_gyro_angles, accCalibOK = false;
float tiempo_ejecucion_MPU, tiempo_MPU_1;

// Timing
long loop_timer;

// Battery
float tension_bateria, lectura_ADC;
bool LOW_BAT_WARNING = false;
int LOW_BAT_WARNING_cont;

// RC (from potentiometers in simulation)
float RC_Throttle = 1000, RC_Pitch = 0, RC_Roll = 0, RC_Yaw = 0;
float throttle_min = 950;
bool armado = false;

// ESC signals
float ESC1_us, ESC2_us, ESC3_us, ESC4_us;

// Non-blocking PWM state
volatile unsigned long pwm_start_time;
volatile bool pwm_active = false;

void setup() {
  Wire.begin();
  Serial.begin(57600);

  pinMode(pin_LED_rojo1, OUTPUT);
  pinMode(pin_LED_rojo2, OUTPUT);

  pinMode(pin_motor1, OUTPUT);
  pinMode(pin_motor2, OUTPUT);
  pinMode(pin_motor3, OUTPUT);
  pinMode(pin_motor4, OUTPUT);
  digitalWrite(pin_motor1, LOW);
  digitalWrite(pin_motor2, LOW);
  digitalWrite(pin_motor3, LOW);
  digitalWrite(pin_motor4, LOW);

  delay(1000);

  Serial.println(F("Iniciando MPU6500..."));

  Wire.beginTransmission(MPU6500_adress);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  Wire.beginTransmission(MPU6500_adress);
  Wire.write(0x1B);
  Wire.write(0x08);
  Wire.endTransmission();

  Wire.beginTransmission(MPU6500_adress);
  Wire.write(0x1C);
  Wire.write(0x10);
  Wire.endTransmission();

  Wire.beginTransmission(MPU6500_adress);
  Wire.write(0x1A);
  Wire.write(0x04);
  Wire.endTransmission();

  Wire.beginTransmission(MPU6500_adress);
  Wire.write(0x75);
  Wire.endTransmission();
  Wire.requestFrom(MPU6500_adress, 1);
  while (Wire.available() < 1);
  byte whoami = Wire.read();

  if (whoami != 0x70 && whoami != 0x68) {
    Serial.print(F("MPU6500 no detectado. WHOAMI=0x"));
    Serial.println(whoami, HEX);
    while (1) { digitalWrite(pin_LED_rojo1, !digitalRead(pin_LED_rojo1)); delay(200); }
  }
  Serial.println(F("MPU6500 OK"));

  Serial.println(F("Calibrando MPU6500. NO MOVER EL DRON..."));
  for (int i = 0; i < 2000; i++) {
    leer_MPU6500();
    gyro_X_cal += gyro_X;
    gyro_Y_cal += gyro_Y;
    gyro_Z_cal += gyro_Z;
    acc_X_cal += ax;
    acc_Y_cal += ay;
    acc_Z_cal += az;
    delay(1);
  }
  gyro_X_cal /= 2000;
  gyro_Y_cal /= 2000;
  gyro_Z_cal /= 2000;
  acc_X_cal /= 2000;
  acc_Y_cal /= 2000;
  acc_Z_cal /= 2000;
  accCalibOK = true;
  Serial.println(F("Calibracion lista"));

  loop_timer = micros();
}

void loop() {
  loop_PWM();

  // Read RC from potentiometers (SIMULATION MODE)
  leer_comandos_RC();

  // Check for serial commands (SET:PID:value)
  if (Serial.available() > 0) {
    procesar_comando_serial();
  }

  leer_MPU6500();

  // Calcular angulos con filtro complementario adaptativo
  angulo_pitch_acc = atan2(ay, sqrt(ax * ax + az * az)) * 180 / PI;
  angulo_roll_acc  = atan2(-ax, sqrt(ay * ay + az * az)) * 180 / PI;

  float acc_mag = sqrt(ax * ax + ay * ay + az * az);
  float alpha = constrain(0.01 / (acc_mag * 0.01 + 0.001), 0.001, 0.05);

  angulo_pitch = (1 - alpha) * (angulo_pitch + gyro_X * 0.0000611) + alpha * angulo_pitch_acc;
  angulo_roll  = (1 - alpha) * (angulo_roll + gyro_Y * 0.0000611)  + alpha * angulo_roll_acc;

  // Calcular con filtro EMA simplificado
  gyro_X = 0.6 * gyro_X + 0.4 * gyro_X_ant;
  gyro_Y = 0.6 * gyro_Y + 0.4 * gyro_Y_ant;
  gyro_Z = 0.6 * gyro_Z + 0.4 * gyro_Z_ant;
  gyro_X_ant = gyro_X;
  gyro_Y_ant = gyro_Y;
  gyro_Z_ant = gyro_Z;

  // PID angulo
  PID_ang_Pitch_error = RC_Pitch - angulo_pitch;
  PID_ang_Pitch_P = PID_ang_Pitch_error * Pitch_ang_Kp;
  PID_ang_Pitch_I += PID_ang_Pitch_error * Pitch_ang_Ki;
  PID_ang_Pitch_I = constrain(PID_ang_Pitch_I, -PID_ang_sat1, PID_ang_sat1);
  PID_ang_Pitch_D = (angulo_pitch - angulo_pitch_ant) * Pitch_ang_Kd;
  PID_ang_Pitch_OUT = PID_ang_Pitch_P + PID_ang_Pitch_I - PID_ang_Pitch_D;
  angulo_pitch_ant = angulo_pitch;

  PID_ang_Roll_error = RC_Roll - angulo_roll;
  PID_ang_Roll_P = PID_ang_Roll_error * Roll_ang_Kp;
  PID_ang_Roll_I += PID_ang_Roll_error * Roll_ang_Ki;
  PID_ang_Roll_I = constrain(PID_ang_Roll_I, -PID_ang_sat2, PID_ang_sat2);
  PID_ang_Roll_D = (angulo_roll - angulo_roll_ant) * Roll_ang_Kd;
  PID_ang_Roll_OUT = PID_ang_Roll_P + PID_ang_Roll_I - PID_ang_Roll_D;
  angulo_roll_ant = angulo_roll;

  // PID velocidad angular
  PID_W_Pitch_error = constrain(PID_ang_Pitch_OUT, -PID_ang_sat1, PID_ang_sat1) - gyro_X;
  PID_W_Pitch_P = PID_W_Pitch_error * Pitch_W_Kp;
  PID_W_Pitch_I += PID_W_Pitch_error * Pitch_W_Ki;
  PID_W_Pitch_I = constrain(PID_W_Pitch_I, -PID_W_sat1, PID_W_sat1);
  PID_W_Pitch_D = (gyro_X - gyro_X_ant) * Pitch_W_Kd;
  PID_W_Pitch_OUT = PID_W_Pitch_P + PID_W_Pitch_I - PID_W_Pitch_D;

  PID_W_Roll_error = constrain(PID_ang_Roll_OUT, -PID_ang_sat2, PID_ang_sat2) - gyro_Y;
  PID_W_Roll_P = PID_W_Roll_error * Roll_W_Kp;
  PID_W_Roll_I += PID_W_Roll_error * Roll_W_Ki;
  PID_W_Roll_I = constrain(PID_W_Roll_I, -PID_W_sat2, PID_W_sat2);
  PID_W_Roll_D = (gyro_Y - gyro_Y_ant) * Roll_W_Kd;
  PID_W_Roll_OUT = PID_W_Roll_P + PID_W_Roll_I - PID_W_Roll_D;

  PID_W_Yaw_error = RC_Yaw - gyro_Z;
  PID_W_Yaw_P = PID_W_Yaw_error * Yaw_W_Kp;
  PID_W_Yaw_I += PID_W_Yaw_error * Yaw_W_Ki;
  PID_W_Yaw_I = constrain(PID_W_Yaw_I, -PID_W_sat2, PID_W_sat2);
  PID_W_Yaw_D = (gyro_Z - gyro_Z_ant) * Yaw_W_Kd;
  PID_W_Yaw_OUT = PID_W_Yaw_P + PID_W_Yaw_I - PID_W_Yaw_D;

  // Mixer X
  float m1 = RC_Throttle - PID_W_Pitch_OUT - PID_W_Roll_OUT - PID_W_Yaw_OUT;
  float m2 = RC_Throttle + PID_W_Pitch_OUT - PID_W_Roll_OUT + PID_W_Yaw_OUT;
  float m3 = RC_Throttle - PID_W_Pitch_OUT + PID_W_Roll_OUT + PID_W_Yaw_OUT;
  float m4 = RC_Throttle + PID_W_Pitch_OUT + PID_W_Roll_OUT - PID_W_Yaw_OUT;

  // Arm check
  if (RC_Throttle < throttle_min + 30 && RC_Pitch < -20 && RC_Roll < -20 && RC_Yaw < -20) {
    armado = true;
    Serial.println(F("ARMADO!"));
    delay(500);
  } else if (RC_Throttle < throttle_min + 30 && RC_Pitch > 20 && RC_Roll > 20 && RC_Yaw > 20) {
    armado = false;
    Serial.println(F("DESARMADO"));
    delay(500);
  }

  if (!armado || RC_Throttle < throttle_min) {
    ESC1_us = 1000; ESC2_us = 1000; ESC3_us = 1000; ESC4_us = 1000;
    PID_ang_Pitch_I = 0; PID_ang_Roll_I = 0;
    PID_W_Pitch_I = 0; PID_W_Roll_I = 0; PID_W_Yaw_I = 0;
  } else {
    ESC1_us = constrain(m1, 1000, 2000);
    ESC2_us = constrain(m2, 1000, 2000);
    ESC3_us = constrain(m3, 1000, 2000);
    ESC4_us = constrain(m4, 1000, 2000);
  }

  // Non-blocking PWM output
  PWM_begin();

  // Debug every ~100ms
  static int cnt = 0;
  if (++cnt >= 16) {
    cnt = 0;
    Serial.print("T:");
    Serial.print(RC_Throttle);
    Serial.print(" P:");
    Serial.print(angulo_pitch, 1);
    Serial.print(" R:");
    Serial.print(angulo_roll, 1);
    Serial.print(" E1:");
    Serial.print(ESC1_us);
    Serial.print(" E2:");
    Serial.print(ESC2_us);
    Serial.print(" E3:");
    Serial.print(ESC3_us);
    Serial.print(" E4:");
    Serial.print(ESC4_us);
    Serial.print(" ");
    Serial.println(armado ? "ARM" : "DIS");
  }

  // LED status
  digitalWrite(pin_LED_rojo1, armado ? HIGH : LOW);
  digitalWrite(pin_LED_rojo2, (millis() / 500) % 2);

  // Battery check
  lectura_ADC = analogRead(A6);
  tension_bateria = lectura_ADC * (5.0 / 1023.0) * 2.95;
  if (tension_bateria < 10.5 && armado) {
    if (LOW_BAT_WARNING_cont++ > 25) {
      LOW_BAT_WARNING = true;
      armado = false;
    }
  } else {
    LOW_BAT_WARNING_cont = 0;
    LOW_BAT_WARNING = false;
  }

  // Maintain loop rate (20000 us = 50 Hz)
  while (micros() - loop_timer < usCiclo);
  loop_timer = micros();
}

// ==================== SUBROUTINES ====================

void leer_MPU6500() {
  Wire.beginTransmission(MPU6500_adress);
  Wire.write(0x3B);
  Wire.endTransmission();
  Wire.requestFrom(MPU6500_adress, 14);
  while (Wire.available() < 14);

  ax = Wire.read() << 8 | Wire.read();
  ay = Wire.read() << 8 | Wire.read();
  az = Wire.read() << 8 | Wire.read();
  Wire.read() << 8 | Wire.read();
  gx = Wire.read() << 8 | Wire.read();
  gy = Wire.read() << 8 | Wire.read();
  gz = Wire.read() << 8 | Wire.read();

  if (accCalibOK) {
    ax -= acc_X_cal;
    ay -= acc_Y_cal;
    az -= acc_Z_cal;
  }
  gyro_X = gx - gyro_X_cal;
  gyro_Y = gy - gyro_Y_cal;
  gyro_Z = gz - gyro_Z_cal;
}

void leer_comandos_RC() {
  // SIMULATION: Read RC from potentiometers on A0-A3
  // A0: Throttle (950-2000)
  // A1: Pitch (-45 to +45)
  // A2: Roll (-45 to +45)
  // A3: Yaw (-45 to +45)
  int raw_thr = analogRead(A0);
  int raw_pit = analogRead(A1);
  int raw_rol = analogRead(A2);
  int raw_yaw = analogRead(A3);

  RC_Throttle = map(raw_thr, 0, 1023, 950, 2000);
  RC_Pitch = map(raw_pit, 0, 1023, -45, 45);
  RC_Roll = map(raw_rol, 0, 1023, -45, 45);
  RC_Yaw = map(raw_yaw, 0, 1023, -45, 45);
}

void procesar_comando_serial() {
  char buf[32];
  int len = 0;
  while (Serial.available() > 0 && len < 31) {
    char c = Serial.read();
    if (c == '\n') break;
    buf[len++] = c;
  }
  buf[len] = '\0';

  if (strncmp(buf, "SET:", 4) == 0) {
    char *rest = buf + 4;
    char *param = strtok(rest, ":");
    char *valstr = strtok(NULL, ":");
    if (param && valstr) {
      float val = atof(valstr);
      if (strcmp(param, "PITCH_ANG_KP") == 0) Pitch_ang_Kp = val;
      else if (strcmp(param, "PITCH_ANG_KI") == 0) Pitch_ang_Ki = val;
      else if (strcmp(param, "PITCH_ANG_KD") == 0) Pitch_ang_Kd = val;
      else if (strcmp(param, "ROLL_ANG_KP") == 0) Roll_ang_Kp = val;
      else if (strcmp(param, "ROLL_ANG_KI") == 0) Roll_ang_Ki = val;
      else if (strcmp(param, "ROLL_ANG_KD") == 0) Roll_ang_Kd = val;
      else if (strcmp(param, "PITCH_W_KP") == 0)  Pitch_W_Kp = val;
      else if (strcmp(param, "PITCH_W_KI") == 0)  Pitch_W_Ki = val;
      else if (strcmp(param, "PITCH_W_KD") == 0)  Pitch_W_Kd = val;
      else if (strcmp(param, "ROLL_W_KP") == 0)   Roll_W_Kp = val;
      else if (strcmp(param, "ROLL_W_KI") == 0)   Roll_W_Ki = val;
      else if (strcmp(param, "ROLL_W_KD") == 0)   Roll_W_Kd = val;
      else if (strcmp(param, "YAW_W_KP") == 0)    Yaw_W_Kp = val;
      else if (strcmp(param, "YAW_W_KI") == 0)    Yaw_W_Ki = val;
      else if (strcmp(param, "YAW_W_KD") == 0)    Yaw_W_Kd = val;
      Serial.print(F("OK: "));
      Serial.print(param);
      Serial.print(" = ");
      Serial.println(val);
    }
  }
}

void PWM_begin() {
  pwm_start_time = micros();
  pwm_active = true;
  digitalWrite(pin_motor1, HIGH);
  digitalWrite(pin_motor2, HIGH);
  digitalWrite(pin_motor3, HIGH);
  digitalWrite(pin_motor4, HIGH);
}

void PWM_end() {
  digitalWrite(pin_motor1, LOW);
  digitalWrite(pin_motor2, LOW);
  digitalWrite(pin_motor3, LOW);
  digitalWrite(pin_motor4, LOW);
  pwm_active = false;
}

void loop_PWM() {
  if (!pwm_active) return;
  unsigned long now = micros();
  unsigned long dt = now - pwm_start_time;
  if (dt >= ESC1_us) digitalWrite(pin_motor1, LOW);
  if (dt >= ESC2_us) digitalWrite(pin_motor2, LOW);
  if (dt >= ESC3_us) digitalWrite(pin_motor3, LOW);
  if (dt >= ESC4_us) digitalWrite(pin_motor4, LOW);
  if (dt >= 2100) PWM_end();
}
