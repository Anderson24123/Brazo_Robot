# Brazo robot 4 GDL – Raspberry Pi 4

Examen segundo parcial – Sistemas Embebidos con Linux.
Control de un brazo de 4 servomotores en C con PWM por software (libgpiod v2).
Modo demo y modo manual con teclado.

## Estructura

```
brazo_robot/
├── CMakeLists.txt
├── .gitignore
├── README.md
├── include/
│   ├── config.h      # pines, límites, tiempos (lo único que se ajusta)
│   ├── servo.h       # librería PWM por software
│   ├── terminal.h    # teclado sin Enter
│   └── brazo.h       # demo / manual
├── src/
│   ├── main.c        # menú
│   ├── servo.c
│   ├── terminal.c
│   └── brazo.c
└── scripts/
    ├── compilar.sh
    ├── ejecutar.sh
    └── revisar_hw.sh
```

Librerías usadas: solo **libc**, **pthread** y **libgpiod**.

## Conexiones

| Servo   | Pin RPi (BCM) | Pin físico |
|---------|---------------|-----------|
| Base    | GPIO17        | 11 |
| Hombro  | GPIO27        | 13 |
| Codo    | GPIO22        | 15 |
| Pinza   | GPIO23        | 16 |

> ⚠️ **Los servos NO se alimentan de la Raspberry.** Usar fuente externa de 5 V
> (≥2 A para 4 servos) y **unir la tierra de la fuente con GND de la Raspberry**
> (por ejemplo pin 6). Sin tierra común los servos tiemblan o no se mueven.

## Preparar la Raspberry

```bash
sudo apt install cmake libgpiod-dev gpiod
./scripts/revisar_hw.sh
```

## Compilar y ejecutar

```bash
./scripts/compilar.sh
./scripts/ejecutar.sh        # corre con sudo
```

O a mano:

```bash
mkdir -p build && cd build
cmake ..
make
sudo ./brazo_robot
```

## Uso

Menú:
1. **Modo demo** – rutina automática: toma un objeto a la izquierda, lo deja a la derecha y saluda. `q` la detiene.
2. **Modo manual** – primero pide **los grados por paso** (1–45). Cada tecla mueve el servo ese número de grados (si se deja presionada, se repite).

   | Tecla | Acción |
   |-------|--------|
   | `a` / `d` | Base izquierda / derecha |
   | `w` / `s` | Hombro sube / baja |
   | `i` / `k` | Codo sube / baja |
   | `j` / `l` | Pinza abre / cierra |
   | `p` | Cambiar los grados por paso |
   | `g` | Llevar un servo a un ángulo exacto |
   | `c` | Regresar a home |
   | `q` | Salir al menú |

3. **Regresar a home**.

Ctrl+C sale limpio en cualquier momento (apaga el PWM y restaura la terminal).

## Cómo funciona

**PWM por software (`servo.c`)**
Un hilo (`pthread`) genera la señal de 50 Hz (periodo de 20 ms) para los 4 servos:
1. pone en alto los 4 pines,
2. ordena los anchos de pulso y baja cada pin cuando se cumple su tiempo (500–2400 µs ⇒ 0–180°),
3. espera a completar los 20 ms.

Se usa `clock_nanosleep` con tiempo absoluto para que el error no se acumule. El ángulo se convierte así:
`pulso = 500 + angulo * (2400 - 500) / 180`.
El `mutex` protege los pulsos porque el menú los escribe desde otro hilo.
Con `sudo` el hilo toma prioridad de tiempo real (`SCHED_FIFO`) y los servos vibran menos.

**Teclado (`terminal.c`)**
Con `termios` se quita el modo canónico y el eco, así cada tecla llega al programa
sin presionar Enter, y `read()` no se bloquea si no hay tecla.

**Modo manual (`brazo.c`)**
Cada tecla suma o resta *paso* grados al servo correspondiente. Los ángulos siempre
se limitan a los rangos de `config.h` para no forzar la mecánica.

## Ajustes comunes (`include/config.h`)

- El servo no llega a 0° o 180° → ajustar `PULSO_MIN_US` / `PULSO_MAX_US`.
- El brazo choca → ajustar `*_MIN` / `*_MAX` de esa articulación.
- Pinza abierta/cerrada del demo → `PINZA_ABIERTA` / `PINZA_CERRADA`.
- Demo más lento o rápido → `DEMO_MS_POR_GRADO`.
- Cambiar pines → `PIN_BASE`, `PIN_HOMBRO`, etc.

## Commits sugeridos (mínimo 5)

```bash
git init
git add CMakeLists.txt .gitignore README.md          && git commit -m "Estructura inicial del proyecto con CMake"
git add include/config.h include/servo.h src/servo.c && git commit -m "Libreria de PWM por software para servos con libgpiod"
git add include/terminal.h src/terminal.c           && git commit -m "Lectura de teclado sin Enter con termios"
git add include/brazo.h src/brazo.c src/main.c      && git commit -m "Menu, modo demo y modo manual con teclado"
git add scripts/                                    && git commit -m "Scripts de compilacion, ejecucion y revision de hardware"
git remote add origin https://github.com/USUARIO/brazo_robot.git
git branch -M main
git push -u origin main
```
