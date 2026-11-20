#ifndef FILES_H
#define FILES_H

#include <stddef.h>

#define FILES_OK 0
#define FILES_FORBIDDEN 403
#define FILES_NOT_FOUND 404
#define FILES_INTERNAL_ERROR 500

int leer_archivo(
    const char *ruta,
    char *contenido,
    size_t capacidad,
    size_t *longitud
);

#endif