#!/bin/bash

set -e 

echo "[BUILD] Removendo executável antigo"
rm -f ./nox

echo "[BUILD] Compilando Nox"
gcc -Wall -Wextra -Werror -Wpedantic -std=c11 -fsanitize=address -g main.c -o nox

echo "[BUILD] Configurando permissões"
chmod +x ./nox

echo "[BUILD] Done"