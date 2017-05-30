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

#define PORT 5000
#define IP 127.0.0.1
#define IMPRIMIR 1



void copiarAArchivoDump(char* mensajeACopiar){
  int tamanioAEscribir = strlen(mensajeACopiar);
  FILE* dump = fopen("./dump.txt","a");
  fwrite(mensajeACopiar,tamanioAEscribir,1,dump);
  fclose(dump);
}

int main(void) {
//	char* cadenaDeEstructuracionDump = string_new();
//	  string_append(&cadenaDeEstructuracionDump,"\n             INICIO DUMP MEMORIA             \n");
//	  puts("\n             INICIO DUMP MEMORIA             \n");
//	  copiarAArchivoDump(cadenaDeEstructuracionDump);
//	  free(cadenaDeEstructuracionDump);

//	int a = 7;
//	int b = 3;
//	int c = a%b;
//	printf("%d",c);

	char* bufferDeData = "Hola como estas";
	int tamMsj = string_length(bufferDeData)+1;
	printf("%d", tamMsj);
}
