/*
 * config.h
 * Configuracion general del brazo/robot (pines, limites, tiempos).
 * Todo lo que se tenga que ajustar sobre el armado fisico esta aqui.
 *
 *   Servo1 = base
 *   Servo2 = hombro
 *   Servo3 = codo
 */
#ifndef CONFIG_H
#define CONFIG_H

/* ---------------- GPIO / servos ---------------- */
#define GPIO_CHIP        "/dev/gpiochip0"   /* chip del header de 40 pines en RPi 4 */
#define NUM_SERVOS       3

/* Numeros BCM de los pines de senal de cada servo */
#define PIN_SERVO1       17   /* base   - pin fisico 11 */
#define PIN_SERVO2       27   /* hombro - pin fisico 13 */
#define PIN_SERVO3       22   /* codo   - pin fisico 15 */

/* PWM de servo: 50 Hz => periodo de 20 ms */
#define PWM_PERIODO_US   20000
#define PULSO_MIN_US     500    /* pulso para 0 grados   (SG90/MG90S) */
#define PULSO_MAX_US     2400   /* pulso para 180 grados (SG90/MG90S) */

/* Limites de cada servo en grados (ajustar a la mecanica del brazo) */
#define SERVO1_MIN   0
#define SERVO1_MAX   180
#define SERVO2_MIN   20
#define SERVO2_MAX   160
#define SERVO3_MIN   20
#define SERVO3_MAX   160

/* Posicion de reposo (home) */
#define SERVO1_HOME  90
#define SERVO2_HOME  90
#define SERVO3_HOME  90

/* Cada cuanto se revisa el teclado en modo manual */
#define MANUAL_PERIODO_MS  20

/* Velocidad del demo: milisegundos por grado */
#define DEMO_MS_POR_GRADO  12

#endif