gcc -std=c11 -fsanitize=address -g teste.c -o teste
./teste teste.nox

rm teste