#!/bin/bash
# Ejecuta el programa con sudo (prioridad de tiempo real = PWM mas estable)
cd "$(dirname "$0")/.."
if [ ! -f build/brazo_robot ]; then
    ./scripts/compilar.sh
fi
sudo ./build/brazo_robot
