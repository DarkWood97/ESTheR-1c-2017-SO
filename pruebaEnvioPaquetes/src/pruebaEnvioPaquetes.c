/*
 ============================================================================
 Name        : pruebaEnvioPaquetes.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "socket.h"
#include "funcionesGenericas.h"

#define PORT 5000
#define IP "127.0.0.1"
#define ES_CPU 1001
#define ES_KERNEL 1002
#define INICIALIZAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define PRUEBA 30


typedef struct __attribute__((__packed__)){
	int tipoMsj;
	int tamMsj;
	void* mensaje;
}paquete;


//-----------------------HILO KERNEL-----------------------------------------------------//
/*int calcularTamanioTotalPaquete(paquete paqueteACalcular){
  int tamanio = sizeof(int)*2 + paqueteACalcular.tamMsj;
  return tamanio;
}

void sendRemasterizado(int aQuien, int tipo, int tamanio, void* que){
  paquete paqueteAEnviar;
  paqueteAEnviar.tipoMsj = tipo;
  paqueteAEnviar.tamMsj = tamanio;
  paqueteAEnviar.mensaje = malloc(tamanio);
  paqueteAEnviar.mensaje = que;
  int tamanioDelQue = calcularTamanioTotalPaquete(que);
  if(send(aQuien, &que, tamanioDelQue)==-1){
    perror("Error al enviar.");
    exit(-1);
  }
  free(paqueteAEnviar.mensaje);
}*/

typedef struct __attribute__((packed)) {
	int comienzo;
	int offset;
} codeIndex;

int main(void){
	int socketServidor = conectarAServer(IP,PORT);
	codeIndex *codigo = malloc(sizeof(codeIndex));
	paquete *paqueteConCodigo = recvRemasterizado(socketServidor);
	codigo = paqueteConCodigo->mensaje;
	printf("%d %d", codigo[1].comienzo, codigo[2].comienzo);
}
