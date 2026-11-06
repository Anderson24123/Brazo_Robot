/*
 * main.c
 * Examen segundo parcial - Sistemas Embebidos con Linux
 * Control de brazo robot de 4 servos con Raspberry Pi 4,
 * modo demo y modo manual con teclado.
 */
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "brazo.h"
#include "terminal.h"

volatile sig_atomic_t g_salir = 0;

static void manejador_senal(int sig)
{
    (void)sig;
    g_salir = 1;
}

static void mostrar_menu(void)
{
    printf("\n===================================\n");
    printf("       BRAZO ROBOT - RPi 4\n");
    printf("===================================\n");
    printf(" 1) Modo demo\n");
    printf(" 2) Modo manual (teclado)\n");
    printf(" 3) Regresar a home\n");
    printf(" 0) Salir\n");
    printf("-----------------------------------\n");
}

int main(void)
{
    /* Ctrl+C: salir limpio (sin SA_RESTART para que fgets se interrumpa) */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = manejador_senal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (brazo_iniciar() < 0)
        return 1;

    while (!g_salir) {
        mostrar_menu();
        int opcion = terminal_pedir_entero("Opcion: ", 0, 3);
        if (opcion < 0 || opcion == 0)
            break;

        switch (opcion) {
        case 1: brazo_demo();   break;
        case 2: brazo_manual(); break;
        case 3: brazo_home();   break;
        }
    }

    printf("\nApagando servos...\n");
    terminal_restaurar();
    brazo_cerrar();
    return 0;
}
