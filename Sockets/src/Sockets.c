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

int verificarErrorSocket(int socket) {
	if (socket == -1) {
		perror("Error de socket");
		return -1;
	}
	return NULL;
}
int verificarErrorSetsockopt(int socket) {
	int yes = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("Error de setsockopt");
		return -1;
	}
	return NULL;
}
int verificarErrorBind(int socket, struct sockaddr_in mySocket) {
	if (bind(socket, (struct sockaddr *) &mySocket, sizeof(mySocket)) == -1) {
		perror("Error de bind");
		return -1;
	}
	return NULL;
}
int verificarErrorListen(int socket) {
	if (listen(socket, backlog) == -1) {
		perror("Error de listen");
		return -1;
	}
	return NULL;
}
int ponerseAEscuchar(int puerto, int protocolo) {
	struct sockaddr_in mySocket;
	int socketListener = socket(AF_INET, SOCK_STREAM, protocolo);
	verificarErrorSocket(socketListener);
	verificarErrorSetsockopt(socketListener);
	mySocket.sin_family = AF_INET;
	mySocket.sin_port = htons(puerto);
	mySocket.sin_addr.s_addr = htonl(INADDR_ANY); //Ver htonl
	memset(&(mySocket.sin_zero), '\0', 8);
	verificarErrorBind(socketListener, mySocket);
	verificarErrorListen(socketListener);
//	return 1;
	return socketListener;
}

int aceptarConexion(int socketListener) {
	int socketAceptador;
	struct sockaddr_in su_addr;
	int sin_size;
	while (1) {
		sin_size = sizeof(struct sockaddr_in); //VER COMO IMPLEMENTAR SELECT!!
		if ((socketAceptador = accept(socketListener,
				(struct sockaddr *) &su_addr, &sin_size)) == -1) {
			perror("Error de accept");
		}
	}
	return socketAceptador;
}

bool enviarMensaje(int socket, char* mensaje) {

	int longitud = string_length(mensaje);
	int i = 0;
	for (; i < longitud; i++) {
		if (send(socket, mensaje, longitud, 0) == -1) {
			perror("Error de send");
			close(socket);
			return false;
		}
	}

	free(mensaje);
	return true;
}

int conectarServer(char * ip, int puerto) {

	int socket_server = socket(AF_INET, SOCK_STREAM, 0);
	struct hostent *info_server;
	struct sockaddr_in direccion_server; // información del server

	//Obtengo info del server
	if ((info_server = gethostbyname(ip)) == NULL) {
		perror("Error al obtener datos del server.");
		return -1;
	}

	//Guardo datos del server
	direccion_server.sin_family = AF_INET;
	direccion_server.sin_port = htons(puerto);
	direccion_server.sin_addr = *((struct in_addr *)info_server->h_addr); //Error
	memset(&(direccion_server.sin_zero), 0, 8);

	//Conecto con servidor, si hay error finalizo
	if (connect(socket_server, (struct sockaddr *) &direccion_server,
			sizeof(struct sockaddr)) == -1) {
		perror("Error al conectar con el servidor.");
		return -1;
	}

	return socket_server;

}

int main() {

	return EXIT_SUCCESS;
}
