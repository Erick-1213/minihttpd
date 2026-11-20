#include "server.h"
#include "http.h"
#include "files.h"
#include "mime.h"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>

#define MAX_EVENTS 20

int main(void){

	int sd, epsd, nfds, flags, nsd;
	struct epoll_event ev, events[MAX_EVENTS];
	ssize_t n;

	signal(SIGPIPE, SIG_IGN);

	sd = crear_socket();
	if(sd ==-1){	
		printf("Error al prepar el socket");
		return -1;
	}

	flags = fcntl(sd, F_GETFL, 0);
	if (flags == -1) {
		perror("Error al obtener las banderas del socket");
		close(sd);
		return 1;
			}

	if (fcntl(sd, F_SETFL, flags | O_NONBLOCK) == -1) {
		perror("Error al establecer el socket en modo no bloqueante");
		close(sd);
		return 1;
		}

	epsd = epoll_create1(0);
	if(epsd == -1){
		perror("No se ha podido crear el epoll");
		close(sd);
		return 1; 
	}

	Cliente servidor = {0};
	servidor.fd = sd;
	servidor.es_servidor = 1;

	ev.events = EPOLLIN;
	ev.data.ptr = &servidor;
	if (epoll_ctl(epsd, EPOLL_CTL_ADD, sd, &ev) == -1) {
		perror("Error en epoll_ctl");
		close(sd);
		close(epsd);
		return 1;
		}

	while(1){
		nfds = epoll_wait(epsd, events, MAX_EVENTS, -1);
		if (nfds == -1) {
			if (errno == EINTR) {
				continue;
			}
			perror("Error en epoll_wait");
			break;
		}

		for (int i = 0; i < nfds; i++) {
			Cliente *actual = events[i].data.ptr;
			if(actual->es_servidor == 1){
				while(1){
					nsd = accept(sd, NULL, NULL);
					if (nsd == -1) {
						if (errno == EINTR) {
							continue;
						}
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							break;
						}
						perror("Error al aceptar la conexión");
						break;
					}

					Cliente *cliente = calloc(1, sizeof(Cliente));
					if (cliente == NULL) {
						perror("Error al asignar memoria para el cliente");
						close(nsd);
						continue;
					}
					cliente->fd = nsd;
					cliente->es_servidor = 0;	
					cliente->usados = 0;

					flags = fcntl(nsd, F_GETFL, 0);
					if (flags == -1) {
						perror("Error al obtener las banderas del socket");
						close(nsd);
						free(cliente);
						continue;
					}
					if (fcntl(nsd, F_SETFL, flags | O_NONBLOCK) == -1) {
						perror("Error al establecer el socket en modo no bloqueante");
						close(nsd);
						free(cliente);
						continue;
					}

					ev.events = EPOLLIN | EPOLLRDHUP;
					ev.data.ptr = cliente;

					if (epoll_ctl(epsd, EPOLL_CTL_ADD, nsd, &ev) == -1) {
						perror("Error en epoll_ctl");
						close(nsd);
						free(cliente);
						continue;
					}
				}
			} else {
				int client_fd = actual->fd;
				int cerrar_cliente = 0;

				if (events[i].events & (EPOLLERR| EPOLLHUP)) {
					cerrar_cliente = 1;
				}
				if (!cerrar_cliente && actual->longitud_respuesta == 0 &&((events[i].events & EPOLLIN) ||((events[i].events & EPOLLOUT) &&fin_cabezera(actual->buffer, actual->usados) > 0))) {
					while (1){
						size_t fin = fin_cabezera(actual->buffer,actual->usados);
						if (fin > 0){
							SolicitudHTTP solicitud = {0};
							int resultado = analizar_solicitud(actual->buffer, fin, &solicitud);
							printf("Resultado del analisis: %d\n", resultado);
							printf("Host encontrado: [%s]\n", solicitud.host);
							fflush(stdout);
							
							actual->cerrar =1;
							if (resultado == -1){
								int longitud = construir_error(actual->respuesta, BUFFER_SIZE, 400, "Bad Request");
								if (longitud == -1){
									cerrar_cliente = 1;
									break;
								}
								
								actual->longitud_respuesta = (size_t)longitud;

							}else {
								int cerrar = 0;
								if(strcasecmp(solicitud.connection, "close") == 0) {
									cerrar = 1;
								}
								if (strcmp(solicitud.metodo, "GET")!=0){
									int longitud = construir_error(actual->respuesta, BUFFER_SIZE,405, "Method NOt Allowed");
										
									if (longitud == -1) {
									cerrar_cliente = 1;
									break;
									} 
									actual->longitud_respuesta = (size_t)longitud;
								} else {
									char contenido[BUFFER_SIZE];
									size_t longitud_archivo = 0;

									int archivo = leer_archivo(solicitud.ruta, contenido, sizeof(contenido), &longitud_archivo);
									if (archivo != FILES_OK) {											
										const char *mensaje;

										if (archivo == FILES_FORBIDDEN) {
											mensaje = "Forbidden";
										} else if (archivo == FILES_NOT_FOUND) {
											mensaje = "Not Found";
											} else {
											archivo = FILES_INTERNAL_ERROR;
											mensaje = "Internal Server Error";
											}
											
											int longitud = construir_error (actual->respuesta, BUFFER_SIZE, archivo,mensaje);
											if (longitud == -1) {
												cerrar_cliente = 1;
												break;
											}					
											actual->longitud_respuesta = (size_t)longitud;
										}else{
											const char *mime = obtener_mime(solicitud.ruta);
											int longitud = construir_respuesta(actual->respuesta,BUFFER_SIZE,contenido,longitud_archivo,mime, cerrar);
											if(longitud==-1){
												cerrar_cliente = 1;
												break;
											}
											actual->cerrar = cerrar;
											actual->longitud_respuesta = (size_t)longitud;
										}
									}
								}
								size_t pendientes = actual->usados - fin;

								memmove(actual->buffer, actual->buffer + fin, pendientes);								
								actual->usados = pendientes;
								actual->enviados = 0;

								ev.events = EPOLLOUT | EPOLLRDHUP;
								ev.data.ptr = actual;

								if (epoll_ctl(epsd, EPOLL_CTL_MOD, client_fd, &ev) == -1) {
									perror("Error en epoll_ctl al modificar el cliente");
									cerrar_cliente = 1;
								}
								break;
							}
							if (actual->usados >= BUFFER_SIZE) {
                              	int longitud = construir_error(actual->respuesta, BUFFER_SIZE, 400, "Bad Request");
								if (longitud == -1) {
                                cerrar_cliente = 1;
                                break;
                            }
							actual->cerrar = 1;
                            actual->longitud_respuesta = (size_t)longitud;
                            actual->enviados = 0;

                            ev.events = EPOLLOUT | EPOLLRDHUP;
                            ev.data.ptr = actual;

                            if (epoll_ctl(epsd, EPOLL_CTL_MOD, client_fd, &ev) == -1) {
                                perror("Error en epoll_ctl al modificar el cliente");
                                cerrar_cliente = 1;
                            }
                            break;
                        }
						n = read(
                            actual->fd,
                            actual->buffer + actual->usados,
                            BUFFER_SIZE - actual->usados
                        );

                        if (n > 0){
                            actual->usados += (size_t)n;
                            continue;
						}else if(n==0){
							cerrar_cliente = 1;
							break;
						}else{
							if(errno == EINTR){
								continue;
							}
							if(errno == EAGAIN ||  errno == EWOULDBLOCK){
								break;
							}
							perror("Error al leer el cliente");
							cerrar_cliente = 1;
							break;
						}
					}
				}
				if((events[i].events & EPOLLRDHUP) && actual->longitud_respuesta == 0){
					cerrar_cliente = 1;
				}
				if(!cerrar_cliente &&(events[i].events & EPOLLOUT) && actual->longitud_respuesta > 0){
					while (actual->enviados < actual->longitud_respuesta){
						
						ssize_t escritos = write(actual->fd, actual->respuesta + actual->enviados, actual->longitud_respuesta - actual->enviados);
						if (escritos > 0){
							actual->enviados += (size_t)escritos;
							continue;							
						}
						if (escritos == -1 &&errno == EINTR){
							continue;
						}
						if (escritos == -1 &&(errno == EAGAIN || errno == EWOULDBLOCK)){
							break;
						}
						perror("Error al enviar respuesta http");
						cerrar_cliente = 1;
						break;
					}
					if (!cerrar_cliente && actual->enviados == actual->longitud_respuesta){
						printf("Respuesta enviada correctamente\n");
						if (actual->cerrar){
							cerrar_cliente = 1;
						}else{
							actual->longitud_respuesta = 0;
							actual->enviados = 0;
							
							if (fin_cabezera(actual->buffer, actual->usados) > 0) {
                                ev.events = EPOLLOUT | EPOLLRDHUP;
                            } else {
                                ev.events = EPOLLIN | EPOLLRDHUP;
                            }
                            ev.data.ptr = actual;
                            if (epoll_ctl(epsd, EPOLL_CTL_MOD, client_fd, &ev) == -1) {
                                perror("Error en epoll_ctl al modificar el cliente");
                                cerrar_cliente = 1;
							}
						}
					}
				}
				if (cerrar_cliente){
					if (epoll_ctl(epsd, EPOLL_CTL_DEL, client_fd, NULL) == -1) {
						perror("Error en epoll_ctl al eliminar el cliente");
					}
					close(client_fd);
					free(actual);
				}
			}
		}
	}
	close(sd);
	close(epsd);
	return 0;
}