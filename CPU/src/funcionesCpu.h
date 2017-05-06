/*
 * funcionesCpu.h
 *
 *  Created on: 3/5/2017
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
#include <stdbool.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <parser/sintax.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>



#ifndef FUNCIONESCPU_H_
#define FUNCIONESCPU_H_

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

typedef struct __attribute__((__packed__)){
	int tamMsj;
	int tipoMsj;
	void* mensaje;
}paquete;

//------------------------DEFINE--------------------------------------
t_log* log;
pcb * PCB;
int tamPagina;
int variableMaxima;
int socketMemoria;

//-----------------------------FUNCIONES------------------------------
t_config generarT_ConfigParaCargar(char *);
void recibirMensajeDeKernel(int);
void verificarParametrosInicio(int);
void verificarParametrosCrear(t_config *,int);
paquete serializar(void*,int);
void realizarHandshake(int, paquete);
void destruirPaquete(paquete *) ;

#endif /* FUNCIONESCPU_H_ */
