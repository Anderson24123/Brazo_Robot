cmake_minimum_required(VERSION 3.13)
project(brazo_robot C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -O2)
add_compile_definitions(_GNU_SOURCE)

find_package(Threads REQUIRED)
find_library(GPIOD_LIB gpiod)
if(NOT GPIOD_LIB)
    message(FATAL_ERROR "No se encontro libgpiod. Instala: sudo apt install libgpiod-dev")
endif()

include_directories(${CMAKE_SOURCE_DIR}/include)

# ---- Librerias propias ----
add_library(servo    STATIC src/servo.c)
add_library(terminal STATIC src/terminal.c)
add_library(brazo    STATIC src/brazo.c)

target_link_libraries(servo PUBLIC ${GPIOD_LIB} Threads::Threads)
target_link_libraries(brazo PUBLIC servo terminal)

# ---- Ejecutable ----
add_executable(brazo_robot src/main.c)
target_link_libraries(brazo_robot PRIVATE brazo)
