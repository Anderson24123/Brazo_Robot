#!/bin/bash
# Revisa que los pines de los servos esten disponibles
echo "== Chips GPIO =="
gpiodetect

echo
echo "== Lineas usadas por los servos (17, 27, 22, 23) =="
gpioinfo gpiochip0 | grep -E "line +(17|22|23|27):"
