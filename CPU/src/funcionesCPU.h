/*
 * funcionesGenericas.h
 *
 *  Created on: 6/4/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <parser/sintax.h>
//-------------FIN DE INCLUDES----------------------------------------
#ifndef FUNCIONESGENERICAS_H_
#define FUNCIONESGENERICAS_H_

//---------------------STRUCTS----------------------------------------
typedef struct{
  int tipoMensaje;
  int tamMensaje;
  void* mensaje;
}paquete;
typedef struct  __attribute__((packed)){

	int ProgramID;
	unsigned int programCounter, paginasCodigo;
	void* referenciaTP; //no estoy segura de que tipo es
	int stackPointer;
	int exitCode;
	t_list *contextoActual;
	int tamContextoActual;
	/* faltan cosas, defini lo que necesita CPU NO CAMBIAR!!*/
}pcb;
//------------------------DEFINE--------------------------------------
t_log* log;
pcb * PCB;
int tamPagina;
//-----------------------------FUNCIONES------------------------------
t_config generarT_ConfigParaCargar(char *);
void recibirMensajeDeKernel(int);
void verificarParametrosInicio(int);
#endif /* FUNCIONESGENERICAS_H_ */
