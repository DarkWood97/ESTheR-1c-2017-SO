/*
 ============================================================================
 Name        : pruebaCliente.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "socket.h"

#define PORT 3490
#define IP "127.0.0.1"

int main(void) {
	int socketDeConexion;
	char *mensaje = malloc(16);
	char *buff=malloc(sizeof(mensaje));
	strcpy(mensaje,"HOLA COMO ESTAS");
	socketDeConexion = conectarServer(IP,PORT);
	if(!(enviarMensaje(socketDeConexion, mensaje))){
		perror("Error al enviar");
		exit(-1);
	}
	recv(socketDeConexion,buff,16,0);
	printf("%s",buff);
	return 0;
	free(mensaje);
	free(buff);
}
