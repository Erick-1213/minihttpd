#include "server.h"
#include <sys/epoll.h>
#include <stdio.h>

#define MAX_EVENTS 20

int main(void){

	int sd, epsd, nfds;
	struct epoll_event ev, events[MAX_EVENTS];

	sd = crear_socket();
	if(sd ==-1){	
		printf("Error al prepar el socket");
		return -1;
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
           perror("epoll_ctl: listen_sock");
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
			// pendiente
		} else {
			// pendiente
		}
	}
}
