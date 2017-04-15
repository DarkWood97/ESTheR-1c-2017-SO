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

int ponerseAEscucharClientes(int puerto, int protocolo) {
	struct sockaddr_in mySocket;
	int socketListener = socket(AF_INET, SOCK_STREAM, protocolo);
	verificarErrorSocket(socketListener);
	verificarErrorSetsockopt(socketListener);
	mySocket.sin_family = AF_INET;
	mySocket.sin_port = htons(puerto);
	mySocket.sin_addr.s_addr = INADDR_ANY;
	memset(&(mySocket.sin_zero), '\0', 8);
	verificarErrorBind(socketListener, mySocket);
	verificarErrorListen(socketListener);
	return socketListener;
}

int aceptarConexionDeCliente(int socketListener) {
	int socketAceptador;
	struct sockaddr_in su_addr;
	socklen_t sin_size;
	sin_size = sizeof(struct sockaddr_in); //VER COMO IMPLEMENTAR SELECT!!

	//while (1) {
	if ((socketAceptador = accept(socketListener, (struct sockaddr *) &su_addr,
			&sin_size)) == -1) {
		perror("Error al aceptar conexion");
		exit(-1);
	} else {
		printf("Se ha conectado a: %s\n", inet_ntoa(su_addr.sin_addr));
	}
	//}

	return socketAceptador;
}

fd_set seleccionarYAceptarConexiones(fd_set master, int *socketMax, int socketEscucha){
	int socketAceptador, socketRevisado;
	fd_set read_sockets;
	FD_ZERO(&read_sockets);
	read_sockets = master;
	if(select(socketMax+1, &read_sockets, NULL, NULL,NULL)==-1){
		perror("Error de Select");
		exit(-1);
	}
	for(socketRevisado = 0; socketRevisado <= socketMax ; socketRevisado++){
		if(FD_ISSET(socketRevisado,&read_sockets)){
			if(socketRevisado == socketEscucha){
				socketAceptador = aceptarConexionDeCliente(socketEscucha);
				FD_SET(socketAceptador, &master);
				if(socketAceptador>socketMax){
					socketMax = socketAceptador;
				}
			}
		}
	}
	return master;
}

bool enviarMensaje(int socket, char* mensaje) { //Socket que envia mensaje

	int longitud =	sizeof(mensaje)+1; //sino no lee \0
	//int i = 0;
	//for (; i < longitud; i++) {
		if (send(socket, mensaje, longitud, 0) == -1) {
			perror("Error de send");
			close(socket);
			exit(-1);
		//}
	}
	return true;
}

void chequearErrorDeSend (int socketAEnviarMensaje, int bytesAEnviar, char* cadenaAEnviar){
	if(send(socketAEnviarMensaje,cadenaAEnviar,bytesAEnviar,0) == -1){
		perror("Error al enviar mensaje a cliente");
		close(socketAEnviarMensaje);
		exit(-1);
	}
}

void revisarSiCortoCliente(int socketCliente, int bytesRecibidos){
	if(bytesRecibidos == 0){
		printf("El socket cliente %d\n corto", socketCliente);
	}
	else{
		perror("Error al recibir mensaje de cliente");
		exit(-1);
	}
}

void recibirYReenviarMensaje(int socketMax,fd_set master, int socketEscucha){
	int socketAChequear, socketsAEnviarMensaje, bytesRecibidos = 0;
	char *buff = malloc(sizeof(char)*16);
	for(socketAChequear=0; socketAChequear<=socketMax; socketAChequear++){
		if((bytesRecibidos = recv(socketAChequear,buff,sizeof(buff),0))<=0){
			revisarSiCortoCliente(socketAChequear, bytesRecibidos);
			close(socketAChequear);
			FD_CLR(socketAChequear, &master);
		}else{
			for(socketsAEnviarMensaje=0;socketsAEnviarMensaje<=socketMax;socketsAEnviarMensaje++){
				if(FD_ISSET(socketsAEnviarMensaje, &master)){
					if(socketsAEnviarMensaje != socketEscucha){
						chequearErrorDeSend(socketsAEnviarMensaje, bytesRecibidos, buff);
					}
				}
			}
			printf("%s\n",buff);
		}
	}
}

int conectarAServer(char *ip, int puerto) { //Recibe ip y puerto, devuelve socket que se conecto

	int socket_server = socket(AF_INET, SOCK_STREAM, 0);
	struct hostent *infoDelServer;
	struct sockaddr_in direccion_server; // información del server

	//Obtengo info del server
	if ((infoDelServer = gethostbyname(ip)) == NULL) {
		perror("Error al obtener datos del server.");
		exit(-1);
	}

	verificarErrorSetsockopt(socket_server);

	//Guardo datos del server
	direccion_server.sin_family = AF_INET;
	direccion_server.sin_port = htons(puerto);
	direccion_server.sin_addr = *(struct in_addr *) infoDelServer->h_addr; //h_addr apunta al primer elemento h_addr_lista
	memset(&(direccion_server.sin_zero), 0, 8);

	//Conecto con servidor, si hay error finalizo
	if (connect(socket_server, (struct sockaddr *) &direccion_server,
			sizeof(struct sockaddr)) == -1) {
		perror("Error al conectar con el servidor.");
		close(socket_server);
	//	exit(-1);
	}

	return socket_server;

}
