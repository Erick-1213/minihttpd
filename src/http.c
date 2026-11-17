#include "http.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

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

static int copiar_valor(char *destino, size_t capacidad, const char *origen, size_t longitud){
    if (longitud >= capacidad) {
        return -1;
    }
    memcpy(destino, origen, longitud);
    destino[longitud] = '\0';
    return 0;
}

int analizar_solicitud(const char *buffer, size_t longitud, SolicitudHTTP *solicitud) {
    if(buffer == NULL || solicitud == NULL || longitud == 0) {
        return -1; 
    }

    memset(solicitud, 0, sizeof(SolicitudHTTP));
    size_t fin_linea = 0;
    while (fin_linea +1 < longitud) {
        if (buffer[fin_linea] == '\r' && buffer[fin_linea + 1] == '\n') {
            break;
        }
        fin_linea++;
    }
    if (fin_linea + 1 >= longitud) {
        return -1; 
    }
    size_t espacio1 = 0;
    size_t espacio2 = 0;

    while (espacio1 < fin_linea && buffer[espacio1] != ' ') {
        espacio1++;
    }
    if (espacio1 == fin_linea) {
        return -1; 
    }
    espacio2 = espacio1 + 1;
    while (espacio2 < fin_linea && buffer[espacio2] != ' ') {
        espacio2++;
    }
    if (espacio2 == fin_linea) {
        return -1; 
    }

    size_t longitud_metodo = espacio1;
    size_t longitud_ruta = espacio2 - espacio1 - 1; 
    size_t longitud_version = fin_linea - espacio2 - 1;

    if (longitud_metodo == 0 ||
        longitud_ruta == 0 ||
        longitud_version == 0 ||
        longitud_metodo >= sizeof(solicitud->metodo) ||
        longitud_ruta >= sizeof(solicitud->ruta) ||
        longitud_version >= sizeof(solicitud->version)) {
        return -1;
    }

    memcpy(solicitud->metodo, buffer, longitud_metodo);
    memcpy(solicitud->ruta, buffer + espacio1 + 1, longitud_ruta);
    memcpy(solicitud->version, buffer + espacio2 + 1, longitud_version);

    size_t posicion = fin_linea + 2;
    while (posicion < longitud){
        size_t fin = posicion;
        while (fin + 1 < longitud &&
           !(buffer[fin] == '\r' && buffer[fin + 1] == '\n')) {
        fin++;
        }
        if (fin + 1 >= longitud) {
            return -1;
        }
        if (fin == posicion) {
            posicion = fin + 2;
            break;
        }
        size_t separador = posicion;
        while (separador < fin && buffer[separador] != ':') {
            separador++;    
        }
        if (separador == fin || separador == posicion) {
            return -1; 
        }
        size_t inicio_valor = separador + 1;
        while (inicio_valor < fin && (buffer[inicio_valor] == ' '|| buffer[inicio_valor] == '\t')) {
            inicio_valor++;
        }
        size_t longitud_nombre = separador - posicion;
        size_t longitud_valor = fin - inicio_valor;

        if (longitud_nombre == 4 &&
            strncasecmp(buffer + posicion, "Host", 4) == 0) {

            if (copiar_valor(solicitud->host,sizeof(solicitud->host),buffer + inicio_valor, longitud_valor) == -1) {
                return -1;
            }

        } else if (longitud_nombre == 10 &&
                strncasecmp(buffer + posicion, "Connection", 10) == 0) {

            if (copiar_valor(solicitud->connection,sizeof(solicitud->connection), buffer + inicio_valor, longitud_valor) == -1) {
                return -1;
            }

        } else if (longitud_nombre == 10 &&
                strncasecmp(buffer + posicion, "User-Agent", 10) == 0) {

            if (copiar_valor(solicitud->user_agent, sizeof(solicitud->user_agent), buffer + inicio_valor, longitud_valor) == -1) {
                return -1;
            }
        }
        posicion = fin + 2;
    }
    if (solicitud->host[0] == '\0') {
        return -1;
    }

    return 0; 
}


