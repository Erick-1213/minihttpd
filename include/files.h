#ifndef FILES_H
#define FILES_H

#include <stddef.h>

int leer_archivo(
    const char *ruta,
    char *contenido,
    size_t capacidad,
    size_t *longitud
);

#endif