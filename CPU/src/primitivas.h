/*
 * primitivas.h
 *
 *  Created on: 6/5/2017
 *      Author: utnso
 */
#include "ansiSop.h"
#ifndef PRIMITIVAS_H_
#define PRIMITIVAS_H_

t_puntero definirVariable(t_nombre_variable);
t_puntero obtenerPosicionVariable(t_nombre_variable);
t_puntero dereferenciar (t_puntero);
void asignar (t_puntero, t_valor_variable );
t_valor_variable obtenerValorCompartida (t_nombre_compartida );
t_valor_variable asignarValorCompartida (t_nombre_compartida , t_valor_variable );
void irAlLabel (t_nombre_etiqueta );
void llamarSinRetorno (t_nombre_etiqueta );
void llamarConRetorno (t_nombre_etiqueta , t_puntero );
void finalizar ();
void retornar(t_valor_variable);



#endif /* PRIMITIVAS_H_ */
