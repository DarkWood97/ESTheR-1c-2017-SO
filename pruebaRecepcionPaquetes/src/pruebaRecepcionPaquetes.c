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
	free(mensaje);
	if(noti == -1){
		puts("Anda bien");
	}else{
		puts("Anda mal");
	}
	mensaje = malloc(sizeof(int)*2);
	int cantidad = 1;
	memcpy(mensaje, &pid, sizeof(int));
	memcpy(mensaje + sizeof(int), &cantidad, sizeof(int));
	sendRemasterizado(socket, 502, sizeof(int)*2, mensaje);
	noti = recvDeNotificacion(socket);
	if(noti == -1){
			puts("Anda bien");
		}else{
			puts("Anda mal");
		}
	int offset = 10;
	int numPag = 2;
	void *info = malloc(20);
	int tam = 20;
	mensaje = malloc(sizeof(int)*4+tam);
	memcpy(mensaje, &pid, sizeof(int));
	memcpy(mensaje + sizeof(int), &numPag, sizeof(int));
	memcpy(mensaje + sizeof(int)*2, &offset, sizeof(int));
	memcpy(mensaje +sizeof(int)*3, &tam, sizeof(int));
	memcpy(mensaje + sizeof(int)*4, info, tam);
	sendRemasterizado(socket, 505, sizeof(int)*4+tam, mensaje);
	noti = recvDeNotificacion(socket);
	if(noti == -1){
			puts("Anda bien");
		}else{
			puts("Anda mal");
		}
	free(mensaje);
	mensaje = malloc(sizeof(int)*2);
	memcpy(mensaje, &pid, sizeof(int));
	memcpy(mensaje + sizeof(int), &numPag, sizeof(int));
	sendRemasterizado(socket, 506, sizeof(int)*2, mensaje);
	noti = recvDeNotificacion(socket);
		if(noti == -1){
				puts("Anda bien");
			}else{
				puts("Anda mal");
			}
		free(mensaje);
	mensaje = malloc(sizeof(int));
	memcpy(mensaje, &pid, sizeof(int));
	sendRemasterizado(socket, 503, sizeof(int), mensaje);
	noti = recvDeNotificacion(socket);
		if(noti == -1){
				puts("Anda bien");
			}else{
				puts("Anda mal");
			}
		free(mensaje);
	while(1){
		/*unPaquete = recvRemasterizado(socketConsola);
		printf("Mensaje recibido: %p"*//*,unPaquete->mensaje);*/
	}
}
