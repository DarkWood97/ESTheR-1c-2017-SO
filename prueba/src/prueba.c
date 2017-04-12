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
	socketListener = ponerseAEscuchar(PORT,0);
	seleccionarYAceptarSockets(socketListener);
	return 0;
}
