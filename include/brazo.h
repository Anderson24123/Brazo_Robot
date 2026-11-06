/*
 * brazo.h
 * Funciones de alto nivel del brazo: demo y modo manual
 */
#ifndef BRAZO_H
#define BRAZO_H

#include <signal.h>

enum { SERVO1 = 0, SERVO2 = 1, SERVO3 = 2 };   /* base, hombro, codo */

/* Se pone en 1 con Ctrl+C (lo maneja main.c) */
extern volatile sig_atomic_t g_salir;

int  brazo_iniciar(void);      /* inicia los servos */
void brazo_cerrar(void);

void brazo_home(void);         /* mueve suave a la posicion de reposo */
void brazo_demo(void);         /* rutina automatica */
void brazo_manual(void);       /* control con teclado, paso en grados */

#endif