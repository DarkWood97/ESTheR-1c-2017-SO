/*
 ============================================================================
 Name        : pruebaRecepcionPaquetes.c
 Author      :
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "socket.h"
#include "funcionesGenericas.h"

/*typedef struct __attribute__((__packed__)){
	int tipoMsj;
	int tamMsj;
	void* mensaje;
}paquete;

int recibirEntero(int socket){
  int entero;
  if(recv(socket, entero, sizeof(int),0)==-1){
    puts("Error al recibir entero");
    exit(-1);
  }
  return entero;
}

paquete recvRemasterizado(int deQuien){
  paquete paqueteARecibir;
  paqueteARecibir.tipoMsj = recibirEntero(deQuien);
  paqueteARecibir.tamMsj = recibirEntero(deQuien);
  paqueteARecibir.mensaje = malloc(paqueteARecibir.tamMsj);
  if(recv(deQuien, paqueteARecibir.mensaje, paqueteARecibir.tamMsj,0)==-1){
    perror("Error al recibir mensaje.");
    exit(-1);
  }
  return paqueteARecibir;
}*/

typedef struct __attribute__((packed)) {
	int comienzo;
	int offset;
} codeIndex;

int main(void) {
	paquete *unPaquete;
	codeIndex *codigo = malloc(sizeof(codeIndex));
	codigo[1].comienzo = 1;
	codigo[2].comienzo = 2;
	int socketConsola = ponerseAEscucharClientes(5000, 0);
	aceptarConexionDeCliente(socketConsola);
	sendRemasterizado(socketConsola, 1, sizeof(codeIndex), codigo);
	while(1){
		/*unPaquete = recvRemasterizado(socketConsola);
		printf("Mensaje recibido: %p"*//*,unPaquete->mensaje);*/
	}
}
