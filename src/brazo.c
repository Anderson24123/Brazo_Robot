/*
 * brazo.c
 * Logica del brazo robot de 3 grados de libertad.
 *   Servo1 = base, Servo2 = hombro, Servo3 = codo
 */
#include <stdio.h>
#include <unistd.h>

#include "brazo.h"
#include "config.h"
#include "servo.h"
#include "terminal.h"

static const char *nombres[NUM_SERVOS] = { "Servo1 (base)", "Servo2 (hombro)", "Servo3 (codo)" };

static const unsigned int pines[NUM_SERVOS] = { PIN_SERVO1, PIN_SERVO2, PIN_SERVO3 };
static const int lim_min[NUM_SERVOS] = { SERVO1_MIN, SERVO2_MIN, SERVO3_MIN };
static const int lim_max[NUM_SERVOS] = { SERVO1_MAX, SERVO2_MAX, SERVO3_MAX };
static const int home[NUM_SERVOS]    = { SERVO1_HOME, SERVO2_HOME, SERVO3_HOME };

/* ------------------------------------------------------------------ */

static int limitar(int id, int angulo)
{
    if (angulo < lim_min[id]) return lim_min[id];
    if (angulo > lim_max[id]) return lim_max[id];
    return angulo;
}

static void mover(int id, int angulo)
{
    servo_escribir(id, limitar(id, angulo));
}

static void dormir_ms(int ms)
{
    usleep(ms * 1000);
}

/* Mueve todos los servos poco a poco (1 grado a la vez) hasta 'destino'.
 * Regresa -1 si el usuario presiono 'q' o Ctrl+C. */
static int mover_suave(const int destino[NUM_SERVOS], int ms_por_grado)
{
    int listo = 0;
    while (!listo) {
        if (g_salir) return -1;
        int c = terminal_tecla();
        if (c == 'q' || c == 'Q') return -1;

        listo = 1;
        for (int i = 0; i < NUM_SERVOS; i++) {
            int actual = servo_leer(i);
            int meta   = limitar(i, destino[i]);
            if (actual < meta)      { mover(i, actual + 1); listo = 0; }
            else if (actual > meta) { mover(i, actual - 1); listo = 0; }
        }
        dormir_ms(ms_por_grado);
    }
    return 0;
}

static void imprimir_angulos(void)
{
    printf("Servo1:%3d  Servo2:%3d  Servo3:%3d",
           servo_leer(SERVO1), servo_leer(SERVO2), servo_leer(SERVO3));
}

/* ------------------------------------------------------------------ */

int brazo_iniciar(void)
{
    if (servo_iniciar(GPIO_CHIP, pines, NUM_SERVOS) < 0) {
        fprintf(stderr, "Error: no se pudieron iniciar los servos\n");
        return -1;
    }
    for (int i = 0; i < NUM_SERVOS; i++)
        servo_escribir(i, home[i]);
    return 0;
}

void brazo_cerrar(void)
{
    servo_detener();
}

void brazo_home(void)
{
    terminal_modo_crudo();
    mover_suave(home, DEMO_MS_POR_GRADO);
    terminal_restaurar();
    printf("En home: ");
    imprimir_angulos();
    printf("\n");
}

/* ------------------------------------------------------------------ */
/*                              DEMO                                  */
/* ------------------------------------------------------------------ */

void brazo_demo(void)
{
    /* Cada fila = {Servo1 (base), Servo2 (hombro), Servo3 (codo)} */
    const int pasos[][NUM_SERVOS] = {
        {  90,  90,  90 },   /* reposo                    */
        {  30,  90,  90 },   /* girar base a la izquierda */
        {  30, 130,  60 },   /* bajar                     */
        {  30,  90,  90 },   /* subir                     */
        { 150,  90,  90 },   /* girar base a la derecha   */
        { 150, 130,  60 },   /* bajar                     */
        { 150,  90,  90 },   /* subir                     */
        {  90,  60, 120 },   /* brazo arriba              */
        {  90,  60,  60 },   /* saludo con el codo        */
        {  90,  60, 120 },
        {  90,  60,  60 },
        { SERVO1_HOME, SERVO2_HOME, SERVO3_HOME }   /* reposo */
    };
    const char *descripcion[] = {
        "Reposo", "Girar izquierda", "Bajar", "Subir",
        "Girar derecha", "Bajar", "Subir",
        "Brazo arriba", "Saludo", "Saludo", "Saludo", "Reposo"
    };
    int total = sizeof(pasos) / sizeof(pasos[0]);

    printf("\n--- MODO DEMO ---  (presiona 'q' para detener)\n");
    terminal_modo_crudo();

    for (int i = 0; i < total; i++) {
        printf("Paso %2d/%d: %-16s -> ", i + 1, total, descripcion[i]);
        fflush(stdout);
        if (mover_suave(pasos[i], DEMO_MS_POR_GRADO) < 0) {
            printf("\nDemo detenido.\n");
            break;
        }
        imprimir_angulos();
        printf("\n");
        dormir_ms(300);
    }

    terminal_restaurar();
    printf("Fin del demo.\n");
}

/* ------------------------------------------------------------------ */
/*                             MANUAL                                 */
/* ------------------------------------------------------------------ */

static void ayuda_manual(int paso)
{
    printf("\n--- MODO MANUAL ---  paso = %d grados\n", paso);
    printf("  Servo1 (base):   a = izquierda   d = derecha\n");
    printf("  Servo2 (hombro): w = subir       s = bajar\n");
    printf("  Servo3 (codo):   i = subir       k = bajar\n");
    printf("  p = cambiar paso   g = ir a un angulo   c = home   q = salir\n\n");
}

void brazo_manual(void)
{
    int paso = terminal_pedir_entero("Grados por paso (1-45): ", 1, 45);
    if (paso < 0) return;

    ayuda_manual(paso);
    imprimir_angulos();
    printf("  | paso:%2d   ", paso);
    fflush(stdout);

    terminal_modo_crudo();

    while (!g_salir) {
        int c = terminal_tecla();
        if (c < 0) {                 /* ninguna tecla: esperar un poco */
            dormir_ms(MANUAL_PERIODO_MS);
            continue;
        }
        if (c == 'q' || c == 'Q') break;

        int servo = -1, dir = 0;
        switch (c) {
        case 'a': servo = SERVO1; dir = -1; break;
        case 'd': servo = SERVO1; dir = +1; break;
        case 'w': servo = SERVO2; dir = +1; break;
        case 's': servo = SERVO2; dir = -1; break;
        case 'i': servo = SERVO3; dir = +1; break;
        case 'k': servo = SERVO3; dir = -1; break;
        case 'c':
            printf("\nRegresando a home...\n");
            mover_suave(home, DEMO_MS_POR_GRADO);
            break;
        case 'p':
            terminal_restaurar();
            printf("\n");
            paso = terminal_pedir_entero("Nuevo paso en grados (1-45): ", 1, 45);
            if (paso < 0) return;
            ayuda_manual(paso);
            terminal_modo_crudo();
            break;
        case 'g': {
            terminal_restaurar();
            printf("\n");
            int s = terminal_pedir_entero("Servo (1=base 2=hombro 3=codo): ", 1, 3);
            if (s < 0) return;
            s--;
            char msj[64];
            snprintf(msj, sizeof(msj), "Angulo para %s (%d-%d): ",
                     nombres[s], lim_min[s], lim_max[s]);
            int a = terminal_pedir_entero(msj, lim_min[s], lim_max[s]);
            if (a < 0) return;
            int destino[NUM_SERVOS];
            for (int i = 0; i < NUM_SERVOS; i++) destino[i] = servo_leer(i);
            destino[s] = a;
            terminal_modo_crudo();
            mover_suave(destino, DEMO_MS_POR_GRADO);
            ayuda_manual(paso);
            break;
        }
        default:
            continue;                /* tecla no usada */
        }

        /* Mover el servo elegido 'paso' grados (respetando sus limites) */
        if (servo >= 0)
            mover(servo, servo_leer(servo) + dir * paso);

        printf("\r");
        imprimir_angulos();
        printf("  | paso:%2d   ", paso);
        fflush(stdout);
    }

    terminal_restaurar();
    printf("\nSaliendo del modo manual.\n");
}