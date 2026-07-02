# Simulacion Wokwi - Drone Desde Cero v2

## Como usar

1. Abre https://wokwi.com/projects/new/arduino-uno
2. Copia el contenido de `sketch.ino` en el editor
3. En el panel de la izquierda, haz click en "diagram.json" y pega el contenido del archivo `diagram.json`
4. En la esquina inferior, activa el Serial Monitor (icono de terminal) para ver la salida
5. Haz click en "Start Simulation"

## Controles

| Potenciometro | Pin | Rango | Funcion |
|---|---|---|---|
| POT1 | A0 | 950-2000 | Throttle (acelerador) |
| POT2 | A1 | -45 a 45 | Pitch (cabeceo) |
| POT3 | A2 | -45 a 45 | Roll (alabeo) |
| POT4 | A3 | -45 a 45 | Yaw (guinada) |

## Como armar/desarmar

- **ARMAR**: Throttle al minimo + todos los sticks hacia abajo (< -20°)
- **DESARMAR**: Throttle al minimo + todos los sticks hacia arriba (> 20°)

## Ajuste de PID en caliente

Envia por el Serial Monitor:
```
SET:PITCH_ANG_KP:0.8
SET:ROLL_ANG_KD:8
SET:YAW_W_KP:1.5
```

## Diferencias con el codigo real

- `leer_comandos_RC()` reemplaza la recepcion UART del ESP32 por lecturas analogicas de potenciometros
- La lectura de bateria en A6 se simula (sin bateria real conectada)
- El loop_PWM() tiene limitaciones: el periodo de loop (6ms) es mayor que los pulsos ESC (1-2ms)
- El MPU6050 de Wokwi es compatible a nivel I2C con el MPU6500 (WhoAmI=0x68 vs 0x70)
- Sin camara OV2640 ni WiFi
