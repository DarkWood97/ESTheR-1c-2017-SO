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
#include <parser/metadata_program.h>

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
	int posicionStackActual;
}PCB;


void desserializarVariables(t_list *variables, paquete *paqueteConVariables, int* dondeEstoy){
    int cantidadDeVariables;
    memcpy(&cantidadDeVariables, paqueteConVariables->mensaje+*dondeEstoy, sizeof(int));
    *dondeEstoy += sizeof(int);
    int i;
    for(i = 0; i<cantidadDeVariables; i++){
        variable *variable = malloc(sizeof(variable));
        memcpy(&variable->nombreVariable, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(char));
        *dondeEstoy += sizeof(char);
        variable->direccionDeVariable = malloc(sizeof(direccion));
        memcpy(&variable->direccionDeVariable->pagina, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        memcpy(&variable->direccionDeVariable->offset, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        memcpy(&variable->direccionDeVariable->tamanio, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        list_add(variables, variable);
    }
}

void desserializarContexto(PCB* pcbConContexto, paquete* paqueteConContexto, int dondeEstoy){
    int nivelDeContexto;
    for(nivelDeContexto = 0; nivelDeContexto<pcbConContexto->tamanioContexto; nivelDeContexto++){
        stack *contextoAAgregarAPCB = malloc(sizeof(stack));
        contextoAAgregarAPCB->args = list_create();
        contextoAAgregarAPCB->vars = list_create();
        memcpy(&contextoAAgregarAPCB->posicion, paqueteConContexto->mensaje+dondeEstoy, sizeof(int));
        dondeEstoy +=sizeof(int);
        desserializarVariables(contextoAAgregarAPCB->args, paqueteConContexto, &dondeEstoy);
        desserializarVariables(contextoAAgregarAPCB->vars, paqueteConContexto, &dondeEstoy);
        memcpy(&contextoAAgregarAPCB->retPos, paqueteConContexto->mensaje + dondeEstoy, sizeof(int));
        memcpy(&contextoAAgregarAPCB->retVar, paqueteConContexto->mensaje + dondeEstoy + sizeof(int), sizeof(direccion));
        list_add(pcbConContexto->indiceStack, contextoAAgregarAPCB);
        dondeEstoy += sizeof(int);
    }
}

int desserializarTIntructions(PCB* pcbDesserializada, paquete *paqueteConPCB){
    int i, auxiliar = 0;
    memcpy(&pcbDesserializada->cantidadTIntructions, paqueteConPCB->mensaje+(sizeof(int)*3), sizeof(int));
    pcbDesserializada->indiceCodigo = malloc(pcbDesserializada->cantidadTIntructions*sizeof(t_intructions));
    for(i = 0; i<pcbDesserializada->cantidadTIntructions; i++){
        memcpy(&pcbDesserializada->indiceCodigo[i].start, paqueteConPCB->mensaje+sizeof(int)*(3+auxiliar), sizeof(int));
        auxiliar++;
        memcpy(&pcbDesserializada->indiceCodigo[i].offset, paqueteConPCB->mensaje+sizeof(int)*(3+auxiliar), sizeof(int));
        auxiliar++;
    }
    int retorno = sizeof(int)*(3+auxiliar);
    return retorno;
}

PCB* deserializarPCB(paquete* paqueteConPCB){
    PCB* pcbDesserializada = malloc(paqueteConPCB->tamMsj);
    memcpy(&pcbDesserializada->pid, paqueteConPCB->mensaje, sizeof(int));
    memcpy(&pcbDesserializada->programCounter, paqueteConPCB->mensaje+sizeof(int), sizeof(int));
    memcpy(&pcbDesserializada->cantidadPaginasCodigo, paqueteConPCB->mensaje+sizeof(int)*2, sizeof(int));
    int dondeEstoy = desserializarTIntructions(pcbDesserializada, paqueteConPCB);
    memcpy(&pcbDesserializada->tamanioEtiquetas, paqueteConPCB->mensaje+dondeEstoy, sizeof(int));
    pcbDesserializada->indiceEtiquetas = string_new();//etiquetas
    if(pcbDesserializada->tamanioEtiquetas!=0){
    	 memcpy(pcbDesserializada->indiceEtiquetas, paqueteConPCB->mensaje+sizeof(int)+dondeEstoy, pcbDesserializada->tamanioEtiquetas);
    }
    memcpy(&pcbDesserializada->rafagas,paqueteConPCB->mensaje+sizeof(int)+dondeEstoy+pcbDesserializada->tamanioEtiquetas,sizeof(int));
    memcpy(&pcbDesserializada->tamanioContexto, paqueteConPCB->mensaje+sizeof(int)*2+dondeEstoy+pcbDesserializada->tamanioEtiquetas, sizeof(int));
    memcpy(&pcbDesserializada->posicionStackActual, paqueteConPCB->mensaje+dondeEstoy+sizeof(int)*3+pcbDesserializada->tamanioEtiquetas, sizeof(int));
    dondeEstoy +=sizeof(int)*4+pcbDesserializada->tamanioEtiquetas;
    pcbDesserializada->indiceStack = list_create();
    desserializarContexto(pcbDesserializada, paqueteConPCB, dondeEstoy);
    return pcbDesserializada;
}

int main(int argc, char *argv[]) {
	int socket = ponerseAEscucharClientes(5002, 0);
	int socketAceptado = aceptarConexionDeCliente(socket);
	paquete* unPaquete = recvRemasterizado(socketAceptado);
	PCB* unaPCB = deserializarPCB(unPaquete);
	printf("PID = %d \n",unaPCB->pid);
	printf("PC = %d \n",unaPCB->programCounter);
	printf("Cantidad TIntructions = %d \n",unaPCB->cantidadTIntructions);
	printf("Tintruction start = %d \n",unaPCB->indiceCodigo[0].start);
	printf("Tintruction offset = %d \n",unaPCB->indiceCodigo[2].offset);
	printf("Indice Etiquetas = %s \n",(char*)unaPCB->indiceEtiquetas);
	printf("Tam Etiquetas = %d \n",unaPCB->tamanioEtiquetas);
	printf("Rafagas = %d \n",unaPCB->rafagas);
	printf("Tam Contexto = %d \n",unaPCB->tamanioContexto);

	return 0;
}
