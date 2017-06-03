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
typedef struct __attribute__((packed)) {
	int pid;
	int tamaniosPaginas;
	int paginas;
} TablaKernel;
typedef struct {
	int comienzo;
	int offset;
} codeIndex;
typedef struct __attribute__((packed)){
	int pagina;
	int offset;
	int tam;
} retVar;
typedef struct {
	int pos;
	t_list* args;
	t_list* vars;
	int retPos;
	retVar retVar;
} Stack;
typedef struct __attribute__((packed)) {
	int PID;
	int ProgramCounter;
	int paginas_Codigo;
	codeIndex cod;
	char* etiquetas;
	int exitCode;
	t_list *contextoActual;
	int tamContextoActual;
	int tamEtiquetas;
	TablaKernel tablaKernel;
} PCB;

typedef struct __attribute__((__packed__)){
	int tamMsj;
	int tipoMsj;
	void* mensaje;
}paquete;

//------------------------DEFINE--------------------------------------
t_log* log;
PCB * pcb;
int tamPagina;
int variableMaxima;
int socketMemoria;
int socketKernel;
int programaAbortado;
int programaFinalizado;
int intMultiproposito;

//-----------------------------FUNCIONES------------------------------
t_config generarT_ConfigParaCargar(char *);
void recibirMensajeDeKernel(int);
void verificarParametrosInicio(int);
void verificarParametrosCrear(t_config *,int);
paquete serializar(void*,int);
void enviar(int,paquete);
void realizarHandshake(int, int);
void destruirPaquete(paquete *) ;
int _obtenerSizeActual_(int);
//void deserealizarPCB(paquete,PCB);
void lineaParaElAnalizador(char * ,char *);
bool estadoDelPrograma();
#endif /* FUNCIONESCPU_H_ */
