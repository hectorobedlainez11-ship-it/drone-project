/*
   DRONE DESDE CERO - v2.0
   Hardware: Arduino UNO + MPU6500 + ESP32 (WiFi/Cam)
   https://arduproject.es/
*/

#define usCiclo 6000

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

// RC (from ESP32 via UART)
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
  Serial.begin(115200);

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

  MPU6500_iniciar();
  MPU6500_calibrar();

  while (!Serial.available()) {
    leer_comando_serial();
    delay(10);
  }

  while (RC_Throttle > 1050 || RC_Throttle < 950) {
    leer_comando_serial();
  }
  armado = true;
  loop_timer = micros();
}

void loop() {
  while (pwm_active) PWM_end();

  if (micros() - loop_timer > usCiclo + 50) digitalWrite(pin_LED_rojo2, HIGH);
  while (micros() - loop_timer < usCiclo);
  loop_timer = micros();

  PWM_begin();
  leer_comando_serial();
  Lectura_tension_bateria();
  if (micros() - loop_timer > 1200) digitalWrite(pin_LED_rojo2, HIGH);
  MPU6500_leer();
  MPU6500_procesar();
  if (armado) {
    PID_ang();
    PID_w();
    Modulador();
  } else {
    ESC1_us = 950; ESC2_us = 950; ESC3_us = 950; ESC4_us = 950;
  }
  PWM_end();

  angulo_pitch_ant = angulo_pitch;
  angulo_roll_ant  = angulo_roll;
  gyro_X_ant = gyro_X;
  gyro_Y_ant = gyro_Y;
  gyro_Z_ant = gyro_Z;
}

void leer_comando_serial() {
  static char buf[64];
  static byte i = 0;
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      buf[i] = '\0';
      i = 0;

      char *ck_start = strstr(buf, "CK:");
      if (ck_start) {
        uint8_t calc_ck = 0;
        for (char *p = buf; p < ck_start; p++) calc_ck ^= *p;
        uint8_t recv_ck = strtol(ck_start + 3, NULL, 16);
        if (calc_ck != recv_ck) return;
        *ck_start = '\0';
      }

      char *p = buf;
      while (*p) {
        if      (strncmp(p, "THR:", 4) == 0) RC_Throttle = atoi(p + 4);
        else if (strncmp(p, "PIT:", 4) == 0) RC_Pitch    = atoi(p + 4);
        else if (strncmp(p, "ROL:", 4) == 0) RC_Roll     = atoi(p + 4);
        else if (strncmp(p, "YAW:", 4) == 0) RC_Yaw      = atoi(p + 4);
        else if (strncmp(p, "SET:", 4) == 0) {
          char var[20]; int val;
          if (sscanf(p, "SET:%[^:]:%d", var, &val) == 2) {
            if      (strcmp(var, "PITCH_ANG_KP")==0) Pitch_ang_Kp = val / 10.0;
            else if (strcmp(var, "PITCH_ANG_KI")==0) Pitch_ang_Ki = val / 100.0;
            else if (strcmp(var, "PITCH_ANG_KD")==0) Pitch_ang_Kd = val / 10.0;
            else if (strcmp(var, "ROLL_ANG_KP")==0)  Roll_ang_Kp  = val / 10.0;
            else if (strcmp(var, "ROLL_ANG_KI")==0)  Roll_ang_Ki  = val / 100.0;
            else if (strcmp(var, "ROLL_ANG_KD")==0)  Roll_ang_Kd  = val / 10.0;
            else if (strcmp(var, "PITCH_W_KP")==0)   Pitch_W_Kp   = val / 10.0;
            else if (strcmp(var, "PITCH_W_KI")==0)   Pitch_W_Ki   = val / 100.0;
            else if (strcmp(var, "PITCH_W_KD")==0)   Pitch_W_Kd   = val / 10.0;
            else if (strcmp(var, "ROLL_W_KP")==0)    Roll_W_Kp    = val / 10.0;
            else if (strcmp(var, "ROLL_W_KI")==0)    Roll_W_Ki    = val / 100.0;
            else if (strcmp(var, "ROLL_W_KD")==0)    Roll_W_Kd    = val / 10.0;
            else if (strcmp(var, "YAW_W_KP")==0)     Yaw_W_Kp     = val / 10.0;
            else if (strcmp(var, "YAW_W_KI")==0)     Yaw_W_Ki     = val / 100.0;
            else if (strcmp(var, "YAW_W_KD")==0)     Yaw_W_Kd     = val / 10.0;
          }
        }
        while (*p && *p != ',') p++;
        if (*p == ',') p++;
      }
      return;
    } else if (c != '\r' && i < sizeof(buf) - 1) {
      buf[i++] = c;
    }
  }
}

void MPU6500_iniciar() {
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
    while (1) {
      digitalWrite(pin_LED_rojo1, LOW);
      delay(500);
      digitalWrite(pin_LED_rojo1, HIGH);
      delay(500);
    }
  }
}

void MPU6500_calibrar() {
  for (int cal_int = 0; cal_int < 3000; cal_int++) {
    MPU6500_leer();
    gyro_X_cal += gx;
    gyro_Y_cal += gy;
    gyro_Z_cal += gz;
    acc_X_cal  += ax;
    acc_Y_cal  += ay;
    acc_Z_cal  += az;
    delayMicroseconds(20);
  }
  gyro_X_cal /= 3000;
  gyro_Y_cal /= 3000;
  gyro_Z_cal /= 3000;
  acc_X_cal  /= 3000;
  acc_Y_cal  /= 3000;
  acc_Z_cal  /= 3000;
  accCalibOK = true;
}

void MPU6500_leer() {
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
    az += 4096;
  }
}

void MPU6500_procesar() {
  #define LPF_ALPHA 0.6
  static float gx_filt = 0, gy_filt = 0, gz_filt = 0;
  gx_filt = gx * LPF_ALPHA + gx_filt * (1 - LPF_ALPHA);
  gy_filt = gy * LPF_ALPHA + gy_filt * (1 - LPF_ALPHA);
  gz_filt = gz * LPF_ALPHA + gz_filt * (1 - LPF_ALPHA);
  gx = gx_filt; gy = gy_filt; gz = gz_filt;

  gyro_X = (gx - gyro_X_cal) / 65.5;
  gyro_Y = (gy - gyro_Y_cal) / 65.5;
  gyro_Z = (gz - gyro_Z_cal) / 65.5;

  tiempo_ejecucion_MPU = (micros() - tiempo_MPU_1) / 1000.0;

  angulo_pitch += gyro_X * tiempo_ejecucion_MPU / 1000.0;
  angulo_roll  += gyro_Y * tiempo_ejecucion_MPU / 1000.0;
  angulo_pitch += angulo_roll  * sin((gz - gyro_Z_cal) * tiempo_ejecucion_MPU * 0.000000266);
  angulo_roll  -= angulo_pitch * sin((gz - gyro_Z_cal) * tiempo_ejecucion_MPU * 0.000000266);
  tiempo_MPU_1 = micros();

  acc_total_vector = sqrt(pow(ay, 2) + pow(ax, 2) + pow(az, 2));
  angulo_pitch_acc = asin((float)ay / acc_total_vector) * 57.2958;
  angulo_roll_acc  = asin((float)ax / acc_total_vector) * -57.2958;

  if (set_gyro_angles) {
    float acc_mag = sqrt(ax * ax + ay * ay + az * az);
    float diff = acc_mag - 4096.0;
    float confidence = 1.0 - (diff > 0 ? diff : -diff) / 4096.0;
    if (confidence < 0.001) confidence = 0.001;
    if (confidence > 0.05)  confidence = 0.05;
    angulo_pitch = angulo_pitch * (1 - confidence) + angulo_pitch_acc * confidence;
    angulo_roll  = angulo_roll  * (1 - confidence) + angulo_roll_acc  * confidence;
  } else {
    angulo_pitch = angulo_pitch_acc;
    angulo_roll  = angulo_roll_acc;
    set_gyro_angles = true;
  }
}

void Modulador() {
  if (RC_Throttle <= 1300) {
    PID_W_Pitch_I = 0;
    PID_W_Roll_I = 0;
    PID_W_Yaw_I  = 0;
    PID_ang_Pitch_I = 0;
    PID_ang_Roll_I = 0;

    ESC1_us = RC_Throttle;
    ESC2_us = RC_Throttle;
    ESC3_us = RC_Throttle;
    ESC4_us = RC_Throttle;

    if (ESC1_us < 1000) ESC1_us = 950;
    if (ESC2_us < 1000) ESC2_us = 950;
    if (ESC3_us < 1000) ESC3_us = 950;
    if (ESC4_us < 1000) ESC4_us = 950;
  } else {
    if (RC_Throttle > 1800) RC_Throttle = 1800;

    ESC1_us = RC_Throttle + PID_W_Pitch_OUT - PID_W_Roll_OUT - PID_W_Yaw_OUT;
    ESC2_us = RC_Throttle + PID_W_Pitch_OUT + PID_W_Roll_OUT + PID_W_Yaw_OUT;
    ESC3_us = RC_Throttle - PID_W_Pitch_OUT + PID_W_Roll_OUT - PID_W_Yaw_OUT;
    ESC4_us = RC_Throttle - PID_W_Pitch_OUT - PID_W_Roll_OUT + PID_W_Yaw_OUT;

    if (ESC1_us < 1100) ESC1_us = 1100;
    if (ESC2_us < 1100) ESC2_us = 1100;
    if (ESC3_us < 1100) ESC3_us = 1100;
    if (ESC4_us < 1100) ESC4_us = 1100;
    if (ESC1_us > 2000) ESC1_us = 2000;
    if (ESC2_us > 2000) ESC2_us = 2000;
    if (ESC3_us > 2000) ESC3_us = 2000;
    if (ESC4_us > 2000) ESC4_us = 2000;
  }
}

void PWM_begin() {
  pwm_active = true;
  pwm_start_time = micros();
  digitalWrite(pin_motor1, HIGH);
  digitalWrite(pin_motor2, HIGH);
  digitalWrite(pin_motor3, HIGH);
  digitalWrite(pin_motor4, HIGH);
}

void PWM_end() {
  if (!pwm_active) return;
  unsigned long elapsed = micros() - pwm_start_time;

  if (elapsed >= ESC1_us) digitalWrite(pin_motor1, LOW);
  if (elapsed >= ESC2_us) digitalWrite(pin_motor2, LOW);
  if (elapsed >= ESC3_us) digitalWrite(pin_motor3, LOW);
  if (elapsed >= ESC4_us) digitalWrite(pin_motor4, LOW);

  if (digitalRead(pin_motor1) == LOW && digitalRead(pin_motor2) == LOW &&
      digitalRead(pin_motor3) == LOW && digitalRead(pin_motor4) == LOW) {
    pwm_active = false;
  }
}

void PID_ang() {
  PID_ang_Pitch_error = RC_Pitch - angulo_pitch;
  PID_ang_Pitch_P  = Pitch_ang_Kp * PID_ang_Pitch_error;
  PID_ang_Pitch_I += Pitch_ang_Ki * PID_ang_Pitch_error;
  PID_ang_Pitch_I  = constrain(PID_ang_Pitch_I, -PID_ang_sat1, PID_ang_sat1);
  PID_ang_Pitch_D  = Pitch_ang_Kd * (angulo_pitch - angulo_pitch_ant);
  PID_ang_Pitch_OUT = PID_ang_Pitch_P + PID_ang_Pitch_I + PID_ang_Pitch_D;
  PID_ang_Pitch_OUT = constrain(PID_ang_Pitch_OUT, -PID_ang_sat2, PID_ang_sat2);

  PID_ang_Roll_error = RC_Roll - angulo_roll;
  PID_ang_Roll_P  = Roll_ang_Kp * PID_ang_Roll_error;
  PID_ang_Roll_I += Roll_ang_Ki * PID_ang_Roll_error;
  PID_ang_Roll_I  = constrain(PID_ang_Roll_I, -PID_ang_sat1, PID_ang_sat1);
  PID_ang_Roll_D  = Roll_ang_Kd * (angulo_roll - angulo_roll_ant);
  PID_ang_Roll_OUT = PID_ang_Roll_P + PID_ang_Roll_I + PID_ang_Roll_D;
  PID_ang_Roll_OUT = constrain(PID_ang_Roll_OUT, -PID_ang_sat2, PID_ang_sat2);
}

void PID_w() {
  PID_W_Pitch_consigna = PID_ang_Pitch_OUT;
  PID_W_Roll_consigna  = PID_ang_Roll_OUT;

  PID_W_Pitch_error = PID_W_Pitch_consigna - gyro_X;
  PID_W_Pitch_P  = Pitch_W_Kp * PID_W_Pitch_error;
  PID_W_Pitch_I += Pitch_W_Ki * PID_W_Pitch_error;
  PID_W_Pitch_I  = constrain(PID_W_Pitch_I, -PID_W_sat1, PID_W_sat1);
  PID_W_Pitch_D  = Pitch_W_Kd * (gyro_X - gyro_X_ant);
  PID_W_Pitch_OUT = PID_W_Pitch_P + PID_W_Pitch_I + PID_W_Pitch_D;
  PID_W_Pitch_OUT = constrain(PID_W_Pitch_OUT, -PID_W_sat2, PID_W_sat2);

  PID_W_Roll_error = PID_W_Roll_consigna - gyro_Y;
  PID_W_Roll_P  = Roll_W_Kp * PID_W_Roll_error;
  PID_W_Roll_I += Roll_W_Ki * PID_W_Roll_error;
  PID_W_Roll_I  = constrain(PID_W_Roll_I, -PID_W_sat1, PID_W_sat1);
  PID_W_Roll_D  = Roll_W_Kd * (gyro_Y - gyro_Y_ant);
  PID_W_Roll_OUT = PID_W_Roll_P + PID_W_Roll_I + PID_W_Roll_D;
  PID_W_Roll_OUT = constrain(PID_W_Roll_OUT, -PID_W_sat2, PID_W_sat2);

  PID_W_Yaw_error = RC_Yaw - gyro_Z;
  PID_W_Yaw_P  = Yaw_W_Kp * PID_W_Yaw_error;
  PID_W_Yaw_I += Yaw_W_Ki * PID_W_Yaw_error;
  PID_W_Yaw_I  = constrain(PID_W_Yaw_I, -PID_W_sat1, PID_W_sat1);
  PID_W_Yaw_D  = Yaw_W_Kd * (gyro_Z - gyro_Z_ant);
  PID_W_Yaw_OUT = PID_W_Yaw_P + PID_W_Yaw_I + PID_W_Yaw_D;
  PID_W_Yaw_OUT = constrain(PID_W_Yaw_OUT, -PID_W_sat2, PID_W_sat2);
}

void Lectura_tension_bateria() {
  if (RC_Throttle < 1100) {
    lectura_ADC = analogRead(A0);
    tension_bateria = 2.95 * (lectura_ADC * 5.0 / 1023);
    if (tension_bateria < 10.5 && !LOW_BAT_WARNING) {
      LOW_BAT_WARNING_cont++;
      if (LOW_BAT_WARNING_cont > 30) LOW_BAT_WARNING = true;
    } else LOW_BAT_WARNING_cont = 0;
  }
}
