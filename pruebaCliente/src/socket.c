/*
 ============================================================================
 Name        : Sockets.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesSockets.h"

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
	int socketListener = socket(AF_INET, SOCK_STREAM, protocolo);
	verificarErrorSocket(socketListener);
	verificarErrorSetsockopt(socketListener);
	mySocket.sin_family = AF_INET;
	mySocket.sin_port = htons(puerto);
	mySocket.sin_addr.s_addr = htonl(INADDR_ANY); //Ver htonl
	memset(&(mySocket.sin_zero), '\0', 8);
	verificarErrorBind(socketListener, mySocket);
	verificarErrorListen(socketListener);
	return socketListener;
}

int aceptarConexion(int socketListener) {
	int socketAceptador;
	struct sockaddr_in su_addr;
	socklen_t sin_size;
/*	while (1) {*/
		sin_size = sizeof(struct sockaddr_in); //VER COMO IMPLEMENTAR SELECT!!
		if ((socketAceptador = accept(socketListener,
				(struct sockaddr *) &su_addr, &sin_size)) == -1) {
			perror("Error de accept");
			exit(-1);
		} else {
			printf("Se ha conectado a: %s\n", inet_ntoa(su_addr.sin_addr));
		}
	/*}*/
	return socketAceptador;
}

void seleccionarYAceptarSockets(int socketListener) {
	int fdmax = socketListener, socketAceptador, nbytes;
	fd_set master, read_fds;
	char* buff = malloc(sizeof(char *));
	FD_ZERO(&master);
	FD_ZERO(&read_fds);
	FD_SET(socketListener, &master);
	int i, j;
	while (1) {
		read_fds = master;
		if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
			perror("Error de select");
			exit(1);
		}
		for (i = 0; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds)) {
				if (i == socketListener) {
					socketAceptador = aceptarConexion(socketListener);
					FD_SET(socketAceptador, &master);
					if (socketAceptador > fdmax) {
						fdmax = socketAceptador;
					}
				} else {
					if ((nbytes = recv(i, buff, sizeof(buff), 0)) <= 0) {
						if (nbytes == 0) {
							printf("El socket %d corto", i);
						} else {
							perror("Error de recv");
						}
						close(i);
						FD_CLR(i, &master);
					} else {
							//atenderPeticion(SocketQuePide, buff);
							for(j = 0; j<=fdmax; j++){
								if(FD_ISSET(j, &master)){
									if(j !=socketListener && j != i){
										if(send(j,buff,nbytes,0)==-1){
											perror("Error de send");
										}
									}
								}
							}



						/*
						 * Falta hacer el send y ver si es necesario verificar si la cantidad enviada es igual a nbytes que devuelve
						 * la funcion. Para la primer entrega se pide unicamente enviar un mensaje de tamaño fijo, pero para las proximas
						 * va a ser de tamaño variable. Ver!
						 * Solucion: implementar el while 1 en el main de kernel, modularizar la parte de select y receive para que sea
						 * funcional para las proximas entregas.
						 */
					}

				}
			}
		}

	}

}

bool enviarMensaje(int socket, char* mensaje) {

	int longitud = string_length(mensaje);
	int i = 0;
	for (; i < longitud; i++) {
		if (send(socket, mensaje, longitud, 0) == -1) {
			perror("Error de send");
			close(socket);
			exit(-1);
			return false;
		}
	}
	return true;
}

int conectarServer(char *ip, int puerto) {

	int socket_server = socket(AF_INET, SOCK_STREAM, 0);
	struct hostent *infoDelServer;
	struct sockaddr_in direccion_server; // información del server

	//Obtengo info del server
	if ((infoDelServer = gethostbyname(ip)) == NULL) {
		perror("Error al obtener datos del server.");
		return -1;
	}

	//Guardo datos del server
	direccion_server.sin_family = AF_INET;
	direccion_server.sin_port = htons(puerto);
	direccion_server.sin_addr = *(struct in_addr *) infoDelServer->h_addr; //h_addr apunta al primer elemento h_addr_lista
	memset(&(direccion_server.sin_zero), 0, 8);

	//Conecto con servidor, si hay error finalizo
	if (connect(socket_server, (struct sockaddr *) &direccion_server,
			sizeof(struct sockaddr)) == -1) {
		perror("Error al conectar con el servidor.");

		return -1;
	}

	return socket_server;

}

