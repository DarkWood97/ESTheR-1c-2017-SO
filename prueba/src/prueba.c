/*
 ============================================================================
 Name        : prueba.c
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

void imprimirArrayDeChar(char* arrayDeChar[]){
	int i = 0;
	for(; arrayDeChar[i]!=NULL; i++){
		printf("%s/n",arrayDeChar[i]);
	}
}

int main(void) {
	int socketListener, socketAceptador;
	char *buff = malloc(16);
	socketListener = ponerseAEscuchar(PORT,0);
	socketAceptador = aceptarConexion(socketListener);
	if((recv(socketAceptador,buff,sizeof(buff),0))==-1){
		perror("Error al recibir");
		exit(-1);
	}
	else{
		puts("Recibido");
	}
	printf("%s\n",buff);
	if(!(enviarMensaje(socketAceptador,buff))){
		perror("Error al reenviar");
		exit(-1);
	}
	return 0;
}
