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
#include <parser/metadata_program.h>
#include <math.h>

#define PORT 5002
#define IP "127.0.0.1"
#define ES_CPU 1001
#define ES_KERNEL 1002
#define INICIALIZAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define PRUEBA 30

typedef struct __attribute__((__packed__)) {
	uint32_t size;
	bool estaLibre;
}heap;


typedef struct __attribute__((__packed__)) {
	int pagina;
	int tamanioDisponible;
	t_list* bloqueHeapMetadata;
}paginaHeap;

typedef struct __attribute__((__packed__)) {
	int pid;
	t_list* pagina;
}tablaHeap;

typedef struct __attribute__((__packed_))
{
  int pagina;
  int offset;
  int tamanio;
} direccion;

typedef struct __attribute__((__packed_)){
  int posicion;
  t_list* args;
  t_list* vars;
  int retPos;
  direccion retVar;
} stack;

typedef struct{
  char nombreVariable;
  direccion *direccionDeVariable;
}variable;

typedef struct __attribute__((__packed_)) {
	int pid;
	int estado;
	int programCounter;
	int posicionStackActual;
	t_intructions *indiceCodigo;
	int cantidadTIntructions;
	int cantidadPaginasCodigo; //Lo usa CPU?
	t_list* indiceStack;
	char* indiceEtiquetas;
	tablaHeap* tablaHeap;
	int exitCode;
	int socket;
	int tamanioEtiquetas;
	bool estaAbortado;
	int rafagas;
	int tamanioContexto;
}PCB;

typedef struct __attribute__((__packed__)){
	int tipoMsj;
	int tamMsj;
	void* mensaje;
}paquete;

long int obtenerTamanioArchivo(FILE* archivo){
	long int nTamArch;
	fseek (archivo, 0, SEEK_END);
	nTamArch = ftell (archivo);
	return nTamArch;
}

char* leerArchivo(FILE *archivo, long int tamanio)
{
	fseek(archivo, 0, SEEK_SET);
	char* codigo = malloc(tamanio);
	fread(codigo, tamanio-1, 1, archivo);
	codigo[tamanio-1] = '\0';
	return codigo;
}

void *serializarT_Intructions(PCB *pcbConT_intruction){
  void *serializacion = malloc(sizeof(t_intructions)*pcbConT_intruction->cantidadTIntructions);
  int i, auxiliar1 = 0;
  for(i = 0; i<pcbConT_intruction->cantidadTIntructions; i++){
    memcpy(serializacion+sizeof(int)*auxiliar1, &pcbConT_intruction->indiceCodigo[i].start, sizeof(int));
    auxiliar1++;
    memcpy(serializacion+sizeof(int)*auxiliar1,&pcbConT_intruction->indiceCodigo[i].offset, sizeof(int));
    auxiliar1++;
  }
  return serializacion;
}

void* serializarVariable(t_list* variables){
    void* variableSerializada = malloc(list_size(variables)*sizeof(variable)+sizeof(int));
    int i;
    int cantidadDeVariables = list_size(variables);
    memcpy(variableSerializada, &cantidadDeVariables, sizeof(int));
    for(i = 0; i<cantidadDeVariables; i++){ //Antes i=1
        variable *variableObtenida = list_get(variables, i);
        memcpy(variableSerializada+sizeof(variable)*i, variableObtenida, sizeof(variable));
    }
    return variableSerializada;
}

int sacarTamanioDeLista(t_list* contexto){
    int i, tamanioDeContexto = 0;
    for(i = 0; i<list_size(contexto); i++){
        stack *contextoAObtener = list_get(contexto, i);
        tamanioDeContexto += sizeof(int)*4+sizeof(direccion)+list_size(contextoAObtener->args)*sizeof(variable)+list_size(contextoAObtener->vars)*sizeof(variable);
        //free(contextoAObtener);
    }
    return tamanioDeContexto;
}

void *serializarStack(PCB* pcbConContextos){
    void* contextoSerializado = malloc(sacarTamanioDeLista(pcbConContextos->indiceStack));
  int i;
  for(i = 0; i<pcbConContextos->tamanioContexto; i++){
    stack* contexto = list_get(pcbConContextos->indiceStack, i);
    memcpy(contextoSerializado, &contexto->posicion, sizeof(int));
    void *argsSerializadas = serializarVariable(contexto->args);
    void *varsSerializadas = serializarVariable(contexto->vars);
    memcpy(contextoSerializado+sizeof(int), argsSerializadas, list_size(contexto->args)*sizeof(variable));
    memcpy(contextoSerializado+sizeof(int)+list_size(contexto->args)*sizeof(variable), varsSerializadas, list_size(contexto->vars)*sizeof(variable));
    memcpy(contextoSerializado+list_size(contexto->args)*sizeof(variable)+list_size(contexto->vars)*sizeof(variable), &contexto->retPos, sizeof(int));
  }
  return contextoSerializado;
}

int sacarTamanioPCB(PCB* pcbASerializar){
    int tamaniosDeContexto = sacarTamanioDeLista(pcbASerializar->indiceStack);
    int tamanio= sizeof(int)*8+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+tamaniosDeContexto+pcbASerializar->tamanioEtiquetas;
    return tamanio;
}

void* serializarPCB(PCB* pcbASerializar){
    int tamanioPCB = sacarTamanioPCB(pcbASerializar);
    void* pcbSerializada = malloc(tamanioPCB);
    memcpy(pcbSerializada, &pcbASerializar->pid, sizeof(int));
    memcpy(pcbSerializada+sizeof(int), &pcbASerializar->programCounter, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*2, &pcbASerializar->cantidadPaginasCodigo, sizeof(int));
    void* indiceDeCodigoSerializado = serializarT_Intructions(pcbASerializar);
    memcpy(pcbSerializada+sizeof(int)*3, &pcbASerializar->cantidadTIntructions, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*4, indiceDeCodigoSerializado, sizeof(t_intructions)*pcbASerializar->cantidadTIntructions);
    memcpy(pcbSerializada+sizeof(int)*4+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions, &pcbASerializar->tamanioEtiquetas, sizeof(int));
    if(pcbASerializar->tamanioEtiquetas!=0){
    	memcpy(pcbSerializada+ sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions, pcbASerializar->indiceEtiquetas, pcbASerializar->tamanioEtiquetas); //CHEQUEAR QUE ES TAMETIQUETAS
    }
    memcpy(pcbSerializada+sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->rafagas, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*6+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->posicionStackActual, sizeof(int));
    void* stackSerializado = serializarStack(pcbASerializar);
    memcpy(pcbSerializada+sizeof(int)*7+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->tamanioContexto, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*8+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas+sacarTamanioDeLista(pcbASerializar->indiceStack), stackSerializado, sacarTamanioDeLista(pcbASerializar->indiceStack));
    return pcbSerializada;
}

t_intructions* cargarCodeIndex(char* buffer,t_metadata_program* metadata_program) {
	t_intructions* code = malloc((sizeof(int)*2)*metadata_program->instrucciones_size);
	int i = 0;

	for (; i < metadata_program->instrucciones_size; i++) {
		code[i].start = metadata_program->instrucciones_serializado[i].start;
		code[i].offset = metadata_program->instrucciones_serializado[i].offset;
	}

	return code;
}

void inicializarStack(PCB *pcbNuevo){
  pcbNuevo->indiceStack = list_create();
  stack *primerStack = malloc(sizeof(stack));
  primerStack->args = list_create();
  primerStack->vars = list_create();
  primerStack->posicion = 0;
  list_add(pcbNuevo->indiceStack, primerStack);
}

char* cargarEtiquetas(t_metadata_program* metadata_program,int sizeEtiquetasIndex) {
	char* etiquetas = malloc(sizeEtiquetasIndex * sizeof(char));
	if(metadata_program->cantidad_de_etiquetas>0 || metadata_program->cantidad_de_funciones>0){
		memcpy(etiquetas, metadata_program->etiquetas,sizeEtiquetasIndex * sizeof(char));
		return etiquetas;
	}
	return NULL;
}


PCB* crearPCB(char* codigo,int tamanioCodigo,int cantPag, int socketConsola) {
	t_metadata_program* metadata_program = metadata_desde_literal(codigo);
	PCB* unPCB = malloc(sizeof(PCB));

	unPCB->pid=3;
	unPCB->programCounter= metadata_program->instruccion_inicio;
	unPCB->estaAbortado=false;
	unPCB->exitCode=0;
	unPCB->tamanioContexto = 1;
	unPCB->rafagas = 5;
	unPCB->cantidadTIntructions = metadata_program->instrucciones_size;
	unPCB->indiceCodigo=cargarCodeIndex(codigo,metadata_program);
	unPCB->cantidadPaginasCodigo=ceil((double)tamanioCodigo/(double)cantPag);
	inicializarStack(unPCB);
	unPCB->tamanioEtiquetas=metadata_program->etiquetas_size;
	unPCB->indiceEtiquetas=cargarEtiquetas(metadata_program,unPCB->tamanioEtiquetas);
	unPCB->tablaHeap=malloc(sizeof(tablaHeap));
	unPCB->socket=socketConsola;
	unPCB->estado="NUEVO";

	metadata_destruir(metadata_program);

	return unPCB;
}

int obtenerCantidadPaginas(char* codigoDePrograma){
  long int tamanioCodigo = string_length(codigoDePrograma);
  int cantidadPaginas = tamanioCodigo / 256;
  if((tamanioCodigo % 256) != 0){
    cantidadPaginas++;
  }
  return cantidadPaginas;
}


int main(int argc, char *argv[]){
	int socketServidor = conectarAServer(IP,PORT);
	FILE *archivoRecibido = fopen(argv[1], "r");
	long int tamanio = obtenerTamanioArchivo(archivoRecibido)+1;
	char* mensaje = leerArchivo(archivoRecibido,tamanio);
	int cantPag = obtenerCantidadPaginas(mensaje);
	PCB* unPCB = crearPCB(mensaje,tamanio,cantPag, 6);
	void* pcbSerializada = serializarPCB(unPCB);
	sendRemasterizado(socketServidor,866,sacarTamanioPCB(unPCB),pcbSerializada);

	return 0;
}
