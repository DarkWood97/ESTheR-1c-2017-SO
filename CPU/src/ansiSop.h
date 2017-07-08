/*
 * ansiSop.h
 *
 *  Created on: 28/4/2017
 *      Author: utnso
 */
#include "funcionesCpu.h"

#ifndef ANSISOP_H_
#define ANSISOP_H_

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


direccion *obtenerDireccionVirtual(t_puntero);
void realizarPeticionDeLecturaAMemoria(direccion* direccionVirtual, t_valor_variable);
t_valor_variable recibirLecturaDeMemoria();
void realizarPeticionDeEscrituraEnMemoria(direccion*, t_valor_variable);
t_valor_variable recibirValorCompartida();
t_valor_variable consultarVariableCompartida(char*);
void enviarModificacionDeValor(char*, t_valor_variable);
void finalizarProceso(PCB *);
int obtenerPCAnterior(PCB *);
void liberarElemento(void*);

#endif /* ANSISOP_H_ */
