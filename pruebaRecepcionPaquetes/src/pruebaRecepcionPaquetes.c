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

#define PORT 5000
#define IP 127.0.0.1
#define IMPRIMIR 1

typedef struct{
  int tamMsj;
  int tipoMsj;
} header;

typedef struct{
  header header;
  void *mensaje;
}paquete __attribute__((packed));



int main(void) {
	int nbytes;
	paquete paquete;
	int socketEscucha = ponerseAEscucharClientes(PORT,0);
	int socketReceptor = aceptarConexionDeCliente(socketEscucha);
	if((nbytes = recv(socketReceptor, paquete.header.tipoMsj, sizeof(int),0))<=0){
		perror("error de recv");
		exit(-1);
	}
	switch (paquete.header.tipoMsj) {
		case IMPRIMIR:
				if((nbytes=recv(socketReceptor,paquete.header.tamMsj,sizeof(int),0))<=0){
					perror("error de recv");
					exit(-1);
				}
				else{
					if((nbytes=recv(socketReceptor,paquete.mensaje,paquete.header.tamMsj,0))<=0){
						perror("error de send");
						exit(-1);
					}
					else{
						printf("%s",(char*)paquete.mensaje);
					}
				}
			break;
		default:
			perror("No se recibio nada");
			exit(-1);
			break;
	}
}
