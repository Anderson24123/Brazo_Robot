/*
 * servo.c
 * PWM por software con libgpiod v2.
 *
 * Funcionamiento (cada 20 ms):
 *   1. Se ponen en ALTO todos los pines de los servos.
 *   2. Se ordenan los pulsos de menor a mayor y, conforme se cumple
 *      el ancho de cada uno (500..2400 us), se pone su pin en BAJO.
 *   3. Se espera hasta completar los 20 ms y se repite.
 * Se usa clock_nanosleep con tiempo ABSOLUTO para que no se acumule error.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE   /* para CPU_SET y pthread_setaffinity_np */
#endif
#include <gpiod.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>

#include "config.h"
#include "servo.h"

#define MAX_SERVOS 8

static struct gpiod_chip         *chip = NULL;
static struct gpiod_line_request *peticion = NULL;

static unsigned int pines_servo[MAX_SERVOS];
static int  num_servos = 0;
static int  pulso_us[MAX_SERVOS];      /* ancho de pulso actual de cada servo */
static int  angulo_actual[MAX_SERVOS];

static pthread_t       hilo;
static pthread_mutex_t candado = PTHREAD_MUTEX_INITIALIZER;
static volatile int    corriendo = 0;

/* Convierte grados (0-180) a microsegundos de pulso */
static int angulo_a_pulso(int angulo)
{
    if (angulo < 0)   angulo = 0;
    if (angulo > 180) angulo = 180;
    return PULSO_MIN_US + (angulo * (PULSO_MAX_US - PULSO_MIN_US)) / 180;
}

/* Suma microsegundos a un timespec */
static struct timespec sumar_us(struct timespec t, long us)
{
    t.tv_nsec += us * 1000L;
    while (t.tv_nsec >= 1000000000L) {
        t.tv_nsec -= 1000000000L;
        t.tv_sec++;
    }
    return t;
}

/* 1 si el tiempo a ya llego (o paso) al tiempo b */
static int ya_llego(struct timespec a, struct timespec b)
{
    if (a.tv_sec != b.tv_sec)
        return a.tv_sec > b.tv_sec;
    return a.tv_nsec >= b.tv_nsec;
}

/* Espera precisa hasta 't':
 * duerme hasta ESPERA_ACTIVA_US antes y el resto lo espera revisando
 * el reloj (espera activa). Asi no depende de que Linux despierte
 * al hilo justo a tiempo, que es lo que causa los "espasmos". */
#define ESPERA_ACTIVA_US 200
static void esperar_hasta(struct timespec t)
{
    struct timespec antes = t, ahora;
    antes.tv_nsec -= ESPERA_ACTIVA_US * 1000L;
    if (antes.tv_nsec < 0) {
        antes.tv_nsec += 1000000000L;
        antes.tv_sec--;
    }
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &antes, NULL);
    do {
        clock_gettime(CLOCK_MONOTONIC, &ahora);
    } while (!ya_llego(ahora, t));
}

static void *hilo_pwm(void *arg)
{
    (void)arg;
    enum gpiod_line_value altos[MAX_SERVOS];
    enum gpiod_line_value bajos[MAX_SERVOS];
    int pulsos[MAX_SERVOS];
    int orden[MAX_SERVOS];
    struct timespec inicio, subida, fin;

    for (int i = 0; i < num_servos; i++) {
        altos[i] = GPIOD_LINE_VALUE_ACTIVE;
        bajos[i] = GPIOD_LINE_VALUE_INACTIVE;
    }

    clock_gettime(CLOCK_MONOTONIC, &inicio);

    while (corriendo) {
        /* Copia de los pulsos (protegida, otro hilo los puede cambiar) */
        pthread_mutex_lock(&candado);
        memcpy(pulsos, pulso_us, sizeof(int) * num_servos);
        pthread_mutex_unlock(&candado);

        /* Ordenar indices por ancho de pulso (insercion, son pocos) */
        for (int i = 0; i < num_servos; i++) orden[i] = i;
        for (int i = 1; i < num_servos; i++) {
            int k = orden[i], j = i - 1;
            while (j >= 0 && pulsos[orden[j]] > pulsos[k]) {
                orden[j + 1] = orden[j];
                j--;
            }
            orden[j + 1] = k;
        }

        /* 1) Todos en alto. Se toma la hora REAL de la subida para medir
         *    el pulso desde ahi (aunque el hilo haya despertado tarde,
         *    el ancho del pulso sale exacto). */
        gpiod_line_request_set_values(peticion, altos);
        clock_gettime(CLOCK_MONOTONIC, &subida);

        /* 2) Bajar cada pin cuando se cumple su pulso */
        for (int i = 0; i < num_servos; i++) {
            int s = orden[i];
            fin = sumar_us(subida, pulsos[s]);
            esperar_hasta(fin);
            gpiod_line_request_set_value(peticion, pines_servo[s],
                                         GPIOD_LINE_VALUE_INACTIVE);
        }

        /* 3) Esperar al siguiente periodo de 20 ms */
        inicio = sumar_us(inicio, PWM_PERIODO_US);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &inicio, NULL);
    }

    gpiod_line_request_set_values(peticion, bajos);
    return NULL;
}

int servo_iniciar(const char *nombre_chip, const unsigned int *pines, int cantidad)
{
    if (cantidad < 1 || cantidad > MAX_SERVOS)
        return -1;

    num_servos = cantidad;
    for (int i = 0; i < cantidad; i++) {
        pines_servo[i]   = pines[i];
        angulo_actual[i] = 90;
        pulso_us[i]      = angulo_a_pulso(90);
    }

    chip = gpiod_chip_open(nombre_chip);
    if (!chip) {
        perror("gpiod_chip_open");
        return -1;
    }

    struct gpiod_line_settings *ajustes = gpiod_line_settings_new();
    gpiod_line_settings_set_direction(ajustes, GPIOD_LINE_DIRECTION_OUTPUT);
    gpiod_line_settings_set_output_value(ajustes, GPIOD_LINE_VALUE_INACTIVE);

    struct gpiod_line_config *config_lineas = gpiod_line_config_new();
    gpiod_line_config_add_line_settings(config_lineas, pines_servo, cantidad, ajustes);

    struct gpiod_request_config *config_peticion = gpiod_request_config_new();
    gpiod_request_config_set_consumer(config_peticion, "brazo_servos");

    peticion = gpiod_chip_request_lines(chip, config_peticion, config_lineas);

    gpiod_request_config_free(config_peticion);
    gpiod_line_config_free(config_lineas);
    gpiod_line_settings_free(ajustes);

    if (!peticion) {
        perror("gpiod_chip_request_lines");
        gpiod_chip_close(chip);
        chip = NULL;
        return -1;
    }

    corriendo = 1;
    if (pthread_create(&hilo, NULL, hilo_pwm, NULL) != 0) {
        perror("pthread_create");
        corriendo = 0;
        return -1;
    }

    /* Prioridad de tiempo real para menos jitter (solo funciona con sudo;
     * si falla, el PWM sigue funcionando con prioridad normal) */
    struct sched_param prioridad = { .sched_priority = 80 };
    if (pthread_setschedparam(hilo, SCHED_FIFO, &prioridad) != 0)
        fprintf(stderr, "Aviso: sin prioridad tiempo real (ejecuta con sudo para menos vibracion)\n");

    /* El hilo del PWM corre siempre en el nucleo 3 (la RPi 4 tiene 0-3),
     * asi no compite con el menu ni con otros programas */
    cpu_set_t nucleos;
    CPU_ZERO(&nucleos);
    CPU_SET(3, &nucleos);
    pthread_setaffinity_np(hilo, sizeof(nucleos), &nucleos);

    /* Bloquear la memoria en RAM para evitar pausas (requiere sudo) */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    return 0;
}

void servo_escribir(int id, int angulo)
{
    if (id < 0 || id >= num_servos)
        return;
    if (angulo < 0)   angulo = 0;
    if (angulo > 180) angulo = 180;

    pthread_mutex_lock(&candado);
    angulo_actual[id] = angulo;
    pulso_us[id]      = angulo_a_pulso(angulo);
    pthread_mutex_unlock(&candado);
}

int servo_leer(int id)
{
    if (id < 0 || id >= num_servos)
        return -1;
    pthread_mutex_lock(&candado);
    int a = angulo_actual[id];
    pthread_mutex_unlock(&candado);
    return a;
}

void servo_detener(void)
{
    if (corriendo) {
        corriendo = 0;
        pthread_join(hilo, NULL);
    }
    if (peticion) {
        gpiod_line_request_release(peticion);
        peticion = NULL;
    }
    if (chip) {
        gpiod_chip_close(chip);
        chip = NULL;
    }
}