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
//------HANDHSAKES
#define HANDSHAKE_MEMORIA 1001
#define TAMANIO_PAGINA_MEMORIA 1020
#define HANDSHAKE_KERNEL 1003
#define DATOS_CONFIG_KERNEL 1013
#define ERROR -1
#define CORTO 0
//-----???
//#define MENSAJE_IMPRIMIR 101
//#define MENSAJE_PATH  103
//#define FINALIZO_PROCESO 1000
//#define MENSAJE_DIRECCION 110
//----PETICIONES KERNEL
#define PETICION_VARIABLE_COMPARTIDA_KERNEL 100
#define PETICION_CAMBIO_VALOR_KERNEL 101
#define DATOS_VARIABLE_COMPARTIDA_KERNEL 102
#define PIDO_RESERVA 103
#define PIDO_LIBERACION 104
#define PIDO_SEMAFORO 105
#define DOY_SEMAFORO 106
#define BLOQUEO_POR_SEMAFORO 107
//-----MEMORIA
#define LEER_DATOS 504
#define DATOS_DE_PAGINA 103
//#define PETICION_LECTURA_MEMORIA 202
#define PETICION_ESCRITURA_MEMORIA 505
//------ARCHIVOS
#define ABRIR_ARCHIVO 300
#define NO_SE_PUDO_ABRIR 301
#define BORRAR_ARCHIVO 302
#define CERRAR_ARCHIVO 303
#define MOVEME_CURSOR 304
#define ESCRIBIR_ARCHIVO 305
#define LEER_DE_ARCHIVO 306
#define DATOS_DE_ARCHIVO 307
#define GUARDAR_DATOS_ARCHIVO 308
//----------PCB
#define MENSAJE_PCB 2000
#define PCB_ABORTADO 2001
#define PCB_FINALIZADO 2002
#define PCB_BLOQUEADO 2003
#define PCB_FINALIZO_RAFAGA 2004
//---------MENSAJES GENERICOS DE EXITO Y ERROR
#define OPERACION_CON_ARCHIVO_EXITOSA 10
#define OPERACION_CON_MEMORIA_EXITOSA 1
#define OPERACION_CON_KERNEL_EXITOSA 12



typedef struct __attribute__((packed)) {
	int pid;
	int tamaniosPaginas;
	int paginas;
} TablaKernel;

typedef struct {
	int comienzo;
	int offset;
} codeIndex;

typedef struct{
  int numPagina;
  int offset;
  int tamanioPuntero;
}direccion;

typedef struct {
	int pos;
	t_list* args;
	t_list* vars;
	int retPos;
	direccion retVar;
}stack;


typedef struct __attribute__((__packed__)) {
	int pid;
	//int estado;
	int programCounter;
	t_intructions *indiceCodigo;
	int cantidadTIntructions;
	int cantidadPaginasCodigo;
	int posicionStackActual;
	t_list* indiceStack;
	char* indiceEtiquetas;
	//tablaHeap* tablaHeap;
	int exitCode;
	//int socket;
	int tamanioEtiquetas;
	//bool estaAbortado;
	int rafagas;
	int tamanioContexto;
	//LO COMENTADO NO LO UTILIZA CPU
}PCB;



//typedef struct __attribute__((packed))t_contexto
//{
//	int pos;
//	t_list *args;
//	t_list *vars;
//	int retPos;
//	direccion retVar;
//	int tamArgs;
//	int tamVars;
//}t_contexto;

typedef struct{
	char* algoritmo;
	int quantum;
	int quantumSleep;
	int tamanioStack;
}datosDeKernel;

typedef struct{
  char nombreVariable;
  direccion *direccionDeVariable;
}variable;

//------------------------DEFINE--------------------------------------
t_log* loggerCPU;
t_log* loggerProgramas;
PCB * pcbEnProceso;
char* IP_KERNEL;
char* IP_MEMORIA;
int PUERTO_MEMORIA;
int PUERTO_KERNEL;
int tamPaginasMemoria;
int variableMaxima;
int socketMemoria;
int socketKernel;
long int punteroAPosicionEnStack;
bool programaEnEjecucionAbortado;
bool programaEnEjecucionFinalizado;
bool programaEnEjecucionBloqueado;
bool sigusr1EstaActivo;
bool yaMePuedoDesconectar;
datosDeKernel *datosParaEjecucion;

//-----------------------------FUNCIONES------------------------------
t_config *generarT_ConfigParaCargar(char *);
void inicializarCPU(char*);
void recibirMensajeDeKernel(int);
void verificarParametrosInicio(int);
void verificarParametrosCrear(t_config *,int);
void obtenerUltimoPunteroStack();
//paquete serializar(void*,int);
//void enviar(int,paquete);
//void realizarHandshake(int, int);
//void destruirPaquete(paquete *) ;
//int _obtenerSizeActual_(int);
////void deserealizarPCB(paquete,PCB);
//void lineaParaElAnalizador(char * ,char *);
//bool estadoDelPrograma();
void *serializarPCB(PCB*);
int sacarTamanioPCB(PCB*);
PCB* deserializarPCB(paquete*);

#endif /* FUNCIONESCPU_H_ */
