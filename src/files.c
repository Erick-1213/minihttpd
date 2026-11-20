#include "files.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>


int leer_archivo(const char *ruta, char *contenido,size_t capacidad, size_t *longitud) {
    if (ruta == NULL || contenido == NULL || longitud == NULL || capacidad == 0) {
        return -1; 
    }
    *longitud = 0;
    if(ruta[0] == '\0' || strstr(ruta, "..") != NULL || strchr(ruta, '\\') != NULL) {
        return FILES_FORBIDDEN; 
    }

    char ruta_completa[512];
    int n = snprintf(ruta_completa, sizeof(ruta_completa), "www/%s", strcmp(ruta, "/") == 0 ? "/index.html" : ruta);
    if (n < 0 || (size_t)n >= sizeof(ruta_completa)) {
        return -1;
    }
    int fd = open(ruta_completa, O_RDONLY | O_NOFOLLOW);

    if (fd == -1) {
    if (errno == ENOENT || errno == ENOTDIR) {
        return FILES_NOT_FOUND;
    }
    if (errno == EACCES || errno == EPERM || errno == ELOOP) {
        return FILES_FORBIDDEN;
    }
    return FILES_INTERNAL_ERROR;
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
            return FILES_OK;
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

    return leidos == 0 ? FILES_OK : FILES_INTERNAL_ERROR;
}

int files_abrir(const char *ruta, int *fd_archivo, off_t *tamano){
    if (ruta == NULL || fd_archivo == NULL || tamano == NULL) {
        return FILES_INTERNAL_ERROR;
    }
    *fd_archivo = -1;
    *tamano = 0;

    if (ruta[0] != '/' ||
        strstr(ruta, "..") != NULL || strchr(ruta, '\\') != NULL) {
        return FILES_FORBIDDEN;
    }
    char ruta_relativa[512];    
    const char *archivo = strcmp(ruta, "/") == 0 ? "index.html" : ruta + 1;
    if (strlen(archivo) >= sizeof(ruta_relativa)) {
        return FILES_FORBIDDEN;
    }
    strcpy(ruta_relativa, archivo);
    int directorio = open("www", O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (directorio == -1) {
        return FILES_INTERNAL_ERROR;
    }
    char *guardar = NULL;
    char *parte = strtok_r(ruta_relativa, "/", &guardar);
    if (parte == NULL) {
        close(directorio);
        return FILES_FORBIDDEN;
    }
    while (parte != NULL) {

        char *siguiente = strtok_r(NULL, "/", &guardar);

        int opciones;

        if (siguiente == NULL) {
            opciones = O_RDONLY | O_NOFOLLOW | O_NONBLOCK;
        } else {
            opciones = O_RDONLY | O_DIRECTORY | O_NOFOLLOW;
        }

        int fd = openat(directorio, parte, opciones);

        if (fd == -1) {

            int error = errno;
            close(directorio);

            if (error == ENOENT) {
                return FILES_NOT_FOUND;
            }

            if (error == EACCES ||
                error == EPERM ||
                error == ELOOP ||
                error == ENOTDIR) {
                return FILES_FORBIDDEN;
            }

            return FILES_INTERNAL_ERROR;
        }

        close(directorio);
        directorio = fd;

        parte = siguiente;
    }

    struct stat datos;

    if (fstat(directorio, &datos) == -1) {
        close(directorio);
        return FILES_INTERNAL_ERROR;
    }
    if (!S_ISREG(datos.st_mode)) {
        close(directorio);
        return FILES_FORBIDDEN;
    }
    *fd_archivo = directorio;
    *tamano = datos.st_size;
    return FILES_OK;
}


