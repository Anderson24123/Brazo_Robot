/*
 * servo.h
 * Libreria de PWM por software para servomotores usando libgpiod v2.
 * Un hilo genera la senal de 50 Hz para todos los servos a la vez.
 */
#ifndef SERVO_H
#define SERVO_H

/* Inicia los pines y el hilo de PWM. Regresa 0 si todo bien, -1 si falla. */
int  servo_iniciar(const char *chip, const unsigned int *pines, int cantidad);

/* Fija el angulo (0-180) de un servo. Los limites se aplican en brazo.c */
void servo_escribir(int id, int angulo);

/* Ultimo angulo escrito en ese servo */
int  servo_leer(int id);

/* Detiene el hilo, deja los pines en bajo y libera libgpiod */
void servo_detener(void);

#endif
