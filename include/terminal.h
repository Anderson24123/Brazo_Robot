/*
 * terminal.h
 * Lectura del teclado sin tener que presionar Enter (termios).
 */
#ifndef TERMINAL_H
#define TERMINAL_H

/* Quita el modo canonico y el eco: las teclas llegan al instante */
void terminal_modo_crudo(void);

/* Regresa la terminal a como estaba */
void terminal_restaurar(void);

/* Regresa la tecla presionada o -1 si no hay ninguna (no bloquea) */
int  terminal_tecla(void);

/* Pide un numero entero por teclado dentro de [min, max] */
int  terminal_pedir_entero(const char *mensaje, int min, int max);

#endif
