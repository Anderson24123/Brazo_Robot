/*
 * terminal.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "terminal.h"

static struct termios original;
static int en_modo_crudo = 0;

void terminal_modo_crudo(void)
{
    if (en_modo_crudo)
        return;
    struct termios t;
    tcgetattr(STDIN_FILENO, &original);
    t = original;
    t.c_lflag &= ~(ICANON | ECHO);   /* sin Enter y sin eco */
    t.c_cc[VMIN]  = 0;               /* read() no bloquea   */
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    en_modo_crudo = 1;
}

void terminal_restaurar(void)
{
    if (!en_modo_crudo)
        return;
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    en_modo_crudo = 0;
}

int terminal_tecla(void)
{
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) == 1)
        return c;
    return -1;
}

int terminal_pedir_entero(const char *mensaje, int min, int max)
{
    char linea[64];
    char *fin;
    long n;

    while (1) {
        printf("%s", mensaje);
        fflush(stdout);
        if (!fgets(linea, sizeof(linea), stdin))
            return -1;                       /* Ctrl+C o fin de entrada */
        n = strtol(linea, &fin, 10);
        if (fin != linea && n >= min && n <= max)
            return (int)n;
        printf("  Valor invalido, debe estar entre %d y %d\n", min, max);
    }
}
