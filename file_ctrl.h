#include <stdio.h>
#include <stdlib.h>

// file_ctrl.h

static inline FILE* __NoxFile_open(const char* path, const char* modes) {
    FILE* file = fopen(path, modes);

    if (file == NULL) {
        printf("open(): não foi possível abrir '%s'\n", path);
        return NULL;
    }

    return file;
}

static inline void __NoxFile_close(FILE* file) {
    if (file == NULL) {
        printf("__NoxFile_close(): arquivo inválido\n");
        return;
    }

    fclose(file);
}

static inline void __NoxFile_read_show(FILE* file) {
    int c;
    c = fgetc(file);
    while(c != EOF) {
        putchar(c);
        c = fgetc(file);
    }

    printf("\n");
}

static inline long __NoxFile_get_size(FILE* file) {
    int r = fseek(file, 0, SEEK_END);
    if (r != 0) {
        printf("get_file_size(): falha ao mover cursor");
        return -1;
    }

    long size = ftell(file);
    if (size == -1L) {
        printf("get_file_size(): falha ao obter posição do cursor");
        return -1;
    }

    r = fseek(file, 0, SEEK_SET);
    if (r != 0) {
        printf("get_file_size(): falha ao retornar cursor para início");
        return -1;
    }

    return size;
}

static inline char* read_file(const char* file) {
    FILE* f = __NoxFile_open(file, "r");
    if (f == NULL) {
        printf("read_file(): falha na abertura do arquivo");
        return NULL;
    }

    long size = __NoxFile_get_size(f);
    if (size < 0) {
        printf("read_file(): tamanho do arquivo menor que 0");
        __NoxFile_close(f);
        return NULL;
    }

    void* addr = malloc(size + 1);
    if (addr == NULL) {
        printf("read_file(): falha na alocação");
        __NoxFile_close(f);
        return NULL;
    }

    long lidos = fread(addr, sizeof(char), size, f);
    if (lidos < size || ferror(f)) {
        printf("read_file(): erro de leitura\n");
        free(addr);
        __NoxFile_close(f);
        return NULL;
    }

    char* txt = (char*)addr;
    txt[lidos] = '\0';

    __NoxFile_close(f);

    return txt;
}

static inline void __NoxFile_free(char* txt) {
    if (txt == NULL) {
        printf("free_file(): parâmetro nulo");
        return;
    }

    free(txt);
}