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
	char *mensaje = malloc(5);
	char *buff=malloc(5);
	strcpy(mensaje,"HOLA");
	socketDeConexion = conectarServer(IP,PORT);
	if(!(enviarMensaje(socketDeConexion, mensaje))){
		perror("Error al enviar");
		exit(-1);
	}
	if((recv(socketDeConexion,buff,sizeof(buff),0))==-1){
		perror("Error al recibir");
		exit(-1);
	}
	printf("%s\n",buff);
	close(socketDeConexion);
	return 0;
	free(mensaje);
	free(buff);
}
