#include "mime.h"
#include <string.h>

const char *obtener_mime(const char *ruta){
    if(ruta == NULL){
        return "application/octet-stream";
    }

    const char *extension = strrchr(ruta, '.');

    if(extension == NULL){
        return "application/octet-stream";
    }
    if(strcmp(extension, ".html")== 0){
        return "text/html";
    } 
    if(strcmp(extension, ".css")== 0){
        return "text/css";
    } 
    if(strcmp(extension, ".js")== 0){
        return "application/javascript";
    } 
    if(strcmp(extension, ".png")== 0){
        return "image/png";
    } 
    if (strcmp(extension, ".jpg") == 0) {
        return "image/jpeg";
    }

    return "application/octet-stream";
}