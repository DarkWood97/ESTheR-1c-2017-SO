/*
 * ansiSop.h
 *
 *  Created on: 28/4/2017
 *      Author: utnso
 */
#include "funcionesCpu.h"
#ifndef ANSISOP_H_
#define ANSISOP_H_

#define MENSAJE_VARIABLE_COMPARTIDA 111

typedef struct __attribute__((packed))t_variable
{
	char etiqueta;
	retVar *direccion;
} t_variable;
typedef struct __attribute__((packed))t_contexto
{
	int posicion;
	t_list *args;
	t_list *vars;
	int retPos;
	retVar retVar;
	int tamArgs;
	int tamVars;
}t_contexto;
int size_retVar;


void proximaDireccion(retVar* ,int, int);
void armarDireccionPagina(retVar *);
void armarDireccionDeArgumento(retVar *);
void armarProximaDireccion(retVar*);
void proximaDireccionArgumento(retVar*, int, int);
void armarDireccionDeFuncion(retVar* );
void destruirPaquete(paquete *);
void enviarDirAMemoria(retVar*, long );
void punteroADir(int, retVar*);
void inicializarVariable(t_variable *, t_nombre_variable ,retVar *);
void enviarDireccionALeerKernel(retVar*, int);
void deserealizarConRetorno(int, paquete*);
#endif /* ANSISOP_H_ */
