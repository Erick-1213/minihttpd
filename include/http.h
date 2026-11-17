#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define BUFFER_SIZE 4096

typedef struct {
    int fd;
    int es_servidor;
    char buffer[BUFFER_SIZE];
    size_t usados;
}Cliente;

size_t fin_cabezera(const char *buffer, size_t usados);
int construir_error(char *buffer, size_t capacidad, int codigo, const char *mensaje);

#endif