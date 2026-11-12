gcc -Wall -Wextra -Wpedantic -std=c11 -fsanitize=address -g main.c -o main_nox
DEBUG="$1"
./main_nox teste.nox "$DEBUG"
rm main_nox