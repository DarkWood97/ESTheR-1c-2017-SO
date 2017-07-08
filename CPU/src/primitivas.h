/*
 * primitivas.h
 *
 *  Created on: 6/5/2017
 *      Author: utnso
 */
#include "ansiSop.h"
#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

t_valor_variable dereferenciar(t_puntero);
t_puntero definirVariable(t_nombre_variable);
t_puntero obtenerPosicionVariable(t_nombre_variable);
void asignar (t_puntero, t_valor_variable );
t_valor_variable obtenerValorCompartida (t_nombre_compartida );
t_valor_variable asignarValorCompartida (t_nombre_compartida , t_valor_variable );
void irAlLabel (t_nombre_etiqueta );
void llamarSinRetorno (t_nombre_etiqueta );
void llamarConRetorno (t_nombre_etiqueta , t_puntero );
void finalizar ();
void retornar(t_valor_variable);

void pedirWait(t_nombre_semaforo identificador_semaforo);
void pedirSignal(t_nombre_semaforo identificador_semaforo);
t_puntero reservarMemoria(t_valor_variable espacio);
void liberarMemoria(t_puntero puntero);
t_descriptor_archivo abrimeArchivo(t_direccion_archivo direccion, t_banderas flags);
void borrameArchivo(t_descriptor_archivo descriptor_archivo);
void cerrameArchivo(t_descriptor_archivo descriptor_archivo);
void movemeElCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion);
void escribimeArchivo(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio);
void leemeDeArchivo(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio);

#endif /* PRIMITIVAS_H_ */
