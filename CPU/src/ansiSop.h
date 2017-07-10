/*
 * ansiSop.h
 *
 *  Created on: 28/4/2017
 *      Author: utnso
 */
#include "funcionesCpu.h"

#ifndef ANSISOP_H_
#define ANSISOP_H_


direccion *obtenerDireccionDePuntero(t_puntero);
void realizarPeticionDeLecturaAMemoria(direccion* direccionVirtual, t_valor_variable);
t_valor_variable recibirLecturaDeMemoria();
void realizarPeticionDeEscrituraEnMemoria(direccion*, t_valor_variable);
t_valor_variable recibirValorCompartida();
t_valor_variable consultarVariableCompartida(char*);
void enviarModificacionDeValor(char*, t_valor_variable);
void finalizarProceso(PCB *);
int obtenerPCAnterior(PCB *);
void liberarElemento(void*);
int obtenerPosicionDeStackAnterior();
void generarContexto(Stack *);
void agregarVariableAArgs(direccion *, t_nombre_variable);
void agregarVariableAVars(direccion *, t_nombre_variable);
direccion* generarDireccionParaVariable(int);
direccion* generarDireccionParaArgumento(int);
int obtenerCantidadDeArgs(Stack *);
int obtenerCantidadDeVars(Stack *);
direccion *obtenerDireccionDeVariable(variable*);
t_puntero convertirDeDireccionAPuntero(direccion*);
Stack* obtenerContextoAnterior();
void mandarAGuardarDatosDeArchivo(paquete *, t_puntero);

#endif /* ANSISOP_H_ */
