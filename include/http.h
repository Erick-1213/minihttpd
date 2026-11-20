#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

#define BUFFER_SIZE 8192

typedef struct {
    int fd;
    int es_servidor;
    char buffer[BUFFER_SIZE];
    size_t usados;
    char respuesta[BUFFER_SIZE];
    size_t longitud_respuesta;
    size_t enviados;
    int cerrar;
}Cliente;

size_t fin_cabezera(const char *buffer, size_t usados);
int construir_error(char *buffer, size_t capacidad, int codigo, const char *mensaje);
int construir_respuesta(char *respuesta, size_t capacidad, const char *contenido, size_t longitud, const char *mime);


typedef struct {
    char metodo[16];
    char ruta[256];
    char version[16];

    char host[256];
    char connection[32];
    char user_agent[512];
} SolicitudHTTP;

int analizar_solicitud(const char *buffer, size_t longitud, SolicitudHTTP *solicitud);

#endif