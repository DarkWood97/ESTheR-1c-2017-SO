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
	int socket = conectarAServer("127.0.0.1", 5002);
	int pid = 3;
	int cantidadPaginas = 4;
	sendDeNotificacion(socket, 1002);
	paquete *paqueteConPaginas = recvRemasterizado(socket);
	free(paqueteConPaginas);
	void *mensaje = malloc(sizeof(int)*2);
	memcpy(mensaje, &pid, sizeof(int));
	memcpy(mensaje+sizeof(int), &cantidadPaginas, sizeof(int));
	sendRemasterizado(socket, 501, sizeof(int)*2, mensaje);
	int noti = recvDeNotificacion(socket);
	if(noti == -1){
		puts("Anda bien");
	}else{
		puts("Anda mal");
	}
	while(1){
		/*unPaquete = recvRemasterizado(socketConsola);
		printf("Mensaje recibido: %p"*//*,unPaquete->mensaje);*/
	}
}
