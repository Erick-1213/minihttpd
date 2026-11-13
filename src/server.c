#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

int crear_socket() {

    struct addrinfo hints = {0};
    struct addrinfo *res;
    int s, sd, resultBind, resultListen;

	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

    s = getaddrinfo(NULL, "8080", &hints, &res);

    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        return -1;
    }

    sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sd == -1) {
        perror("socket no creado");
        freeaddrinfo(res);
		return -1;
    }

    resultBind = bind(sd, res->ai_addr, res->ai_addrlen);
	if (resultBind == -1){
		perror("error en bind");
		close (sd);
         freeaddrinfo(res);
		return -1;
	}

    resultListen = listen(sd, 5);
	if (resultListen == -1){
		perror("error en listen");
		close (sd);
		freeaddrinfo(res);
		return -1;
	}
		
	freeaddrinfo(res);
	return sd;


}
