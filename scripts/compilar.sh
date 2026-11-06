#!/bin/bash
# Compila el proyecto con CMake en la carpeta build/
set -e
cd "$(dirname "$0")/.."
mkdir -p build
cd build
cmake ..
make
echo "Listo: ./build/brazo_robot"
