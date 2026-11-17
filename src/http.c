#include "http.h"
#include <stdio.h>

size_t fin_cabezera(const char *buffer, size_t usados) {

    for (size_t i = 0; i + 3 < usados; i++) {

        if (buffer[i] == '\r' &&
            buffer[i + 1] == '\n' &&
            buffer[i + 2] == '\r' &&
            buffer[i + 3] == '\n') {

            return i + 4;
        }
    }

    return 0;
}

int construir_error(char *buffer, size_t capacidad, int codigo, const char *mensaje) {
    int n = snprintf(buffer, capacidad,
                    "HTTP/1.1 %d %s\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n"
                    "\r\n",
                    codigo, mensaje);
    if (n<0 || (size_t)n >= capacidad) {
        return -1; 
    }
    return n;
}