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
#include "socket.h"


#ifndef FUNCIONESCPU_H_
#define FUNCIONESCPU_H_

#define HANDSHAKE_MEMORIA 1001
#define HANDSHAKE_KERNEL 1002
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define PETICION_LECTURA_MEMORIA 104
#define PETICION_ESCRITURA_MEMORIA 105
#define MENSAJE_DIRECCION 110
#define ERROR -1
#define CORTO 0
#define PETICION_VARIABLE_COMPARTIDA_KERNEL 111
#define PETICION_CAMBIO_VALOR_KERNEL 112
#define DATOS_VARIABLE_COMPARTIDA_KERNEL 1011
#define DATOS_VARIABLE_MEMORIA 1013
#define FINALIZO_PROCESO 1000
#define PIDO_SEMAFORO 13
#define DOY_SEMAFORO 12
#define PIDO_RESERVA 11
#define PIDO_LIBERACION 10
#define ABRIR_ARCHIVO 9
#define NO_SE_PUDO_ABRIR 8
#define BORRAR_ARCHIVO 7
#define CERRAR_ARCHIVO 6
#define MOVEME_CURSOR 5
#define ESCRIBIR_ARCHIVO 4
#define LEER_DE_ARCHIVO 3
#define LEER_DATOS 2
#define DATOS_DE_PAGINA 1


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
	codeIndex *cod;
	char* etiquetas;
	int exitCode;
	t_list *contextoActual;
	int tamContextoActual;
	int tamEtiquetas;
	TablaKernel tablaKernel;
} PCB;

typedef struct{
  int numPagina;
  int offset;
  int tamanioPuntero;
}direccion;

typedef struct __attribute__((packed))t_contexto
{
	int pos;
	t_list *args;
	t_list *vars;
	int retPos;
	direccion retVar;
	int tamArgs;
	int tamVars;
}t_contexto;

typedef struct{
	char* algoritmo;
	int quantum;
}datosDeKernel;

typedef struct{
  char nombreVariable;
  direccion *direccionDeVariable;
}variable;

//------------------------DEFINE--------------------------------------
t_log* loggerCPU;
PCB * pcbEnProceso;
char* IP_KERNEL;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
int PUERTO_KERNEL;
int tamPaginasMemoria;
int variableMaxima;
int socketMemoria;
int socketKernel;
bool programaEnEjecucionAbortado = false;
bool programaEnEjecucionFinalizado = false;
bool programaEnEjecucionBloqueado =false;
bool sigusr1EstaActivo = false;
bool yaMePuedoDesconectar = false;
datosDeKernel *datosParaEjecucion;

//-----------------------------FUNCIONES------------------------------
t_config *generarT_ConfigParaCargar(char *);
void inicializarCPU(char*);
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
