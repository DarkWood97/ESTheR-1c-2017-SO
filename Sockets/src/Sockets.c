/*
 ============================================================================
 Name        : Sockets.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <commons/string.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>

#define backlog 10 //fijarse las conexiones que queremos

typedef struct {
	int puerto;
	int socket_cliente;
	void (*fn_newClient)();
	t_dictionary * fns_receipts;
	void (*fn_connectionClosed)();
	void * data;
} arg_escucharclientes;

int ponerseAEscuchar(int puerto, int protocolo) {
	struct sockaddr_in mySocket;
	int yes = 1;
	int socketListenner = socket(AF_INET, SOCK_STREAM, protocolo);
	if (socketListenner == -1) {
		perror("Error de socket");
		return -1;
	}
	if (setsockopt(socketListenner, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int))
			== -1) {
		perror("Error de setsockopt");
		return -1;
	}
	mySocket.sin_family = AF_INET;
	mySocket.sin_port = htons(puerto);
	mySocket.sin_addr.s_addr = htonl(INADDR_ANY); //Ver htonl
	memset(&(mySocket.sin_zero), '\0', 8);

	if (bind(socketListenner, (struct sockaddr *) &mySocket, sizeof(mySocket))
			== -1) {
		perror("Error de bind");
		return -1;
	}

	if (listen(socketListenner, backlog) == -1) {
		perror("Error de listen");
		return -1;
	}

	//Falta eliminar procesos muertos xD

	arg_escucharclientes *args = malloc(sizeof(arg_escucharclientes));
	args->puerto = puerto;
	args->socket_cliente = socketListenner;

	return 1;
}

bool enviarMensaje(int socket,char* mensaje){

	int longitud = string_length(mensaje);
	int i = 0;
	for(;i<longitud;i++){
		if(send(socket,mensaje,longitud,0)==-1){
			perror("Error de send");
			close(socket);
			return false;
		}
	}

	free(mensaje);
	return true;
}

void aceptarMensaje(int socket){

}
