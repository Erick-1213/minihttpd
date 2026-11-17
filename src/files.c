#include "files.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


int leer_archivo(const char *ruta, char *contenido,size_t capacidad, size_t *longitud) {
    if (ruta == NULL || contenido == NULL || longitud == NULL || capacidad == 0) {
        return -1; 
    }
    *longitud = 0;
    if(ruta[0] == '\0' || strstr(ruta, "..") != NULL || strchr(ruta, '\\') != NULL) {
        return -1; 
    }

    char ruta_completa[512];
    int n = snprintf(ruta_completa, sizeof(ruta_completa), "www/%s", strcmp(ruta, "/") == 0 ? "index.html" : ruta);
    if (n < 0 || (size_t)n >= sizeof(ruta_completa)) {
        return -1;
    }
    int fd = open(ruta_completa, O_RDONLY | O_NOFOLLOW);

    if (fd == -1) {
        return -1;
    }

    while (*longitud < capacidad) {

        ssize_t leidos = read(
            fd,
            contenido + *longitud,
            capacidad - *longitud
        );

        if (leidos > 0) {
            *longitud += (size_t)leidos;
            continue;
        }

        if (leidos == 0) {
            close(fd);
            return 0;
        }

        if (errno == EINTR) {
            continue;
        }

        close(fd);
        return -1;
    }

    char extra;
    ssize_t leidos;

    do {
        leidos = read(fd, &extra, 1);
    } while (leidos == -1 && errno == EINTR);

    close(fd);

    return leidos == 0 ? 0 : -1;
}


