#include "server.h"
#include <sys/epoll.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#define MAX_EVENTS 20

int main(void){

	int sd, epsd, nfds, flags, nsd;
	struct epoll_event ev, events[MAX_EVENTS];

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

	ev.events = EPOLLIN;
	ev.data.fd = sd;
	if (epoll_ctl(epsd, EPOLL_CTL_ADD, sd, &ev) == -1) {
		perror("Error en epoll_ctl");
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
			if(events[i].data.fd == sd){
				while(1){
					nsd = accept(sd, NULL, NULL);
					if (nsd == -1) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							break;
						}
						if (errno == EINTR) {
							continue;
						}
						break;
					}
					close(nsd);
			} else {
						
				}
			}
		}	
	}
}
