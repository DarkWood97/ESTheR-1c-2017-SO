/*
 * ansiSop.h
 *
 *  Created on: 28/4/2017
 *      Author: utnso
 */
#include "funcionesCpu.h"
#ifndef ANSISOP_H_
#define ANSISOP_H_
typedef struct __attribute__((packed))t_direccion{
	int pagina;
	int offset;
	int tam;
}t_direccion;
typedef struct __attribute__((packed))t_variable
{
	char etiqueta;
	t_direccion *direccion;
} t_variable;
typedef struct __attribute__((packed))t_contexto
{
	int posicion;
	t_list *args;
	t_list *vars;
	int retPos;
	t_direccion retVar;
	int tamArgs;
	int tamVars;
}t_contexto;
int sizeT_direccion;


void proximaDireccion(t_direccion* ,int, int);
void armarDireccionPagina(t_direccion *);
void armarDireccionDeArgumento(t_direccion *);
void armarProximaDireccion(t_direccion*);
void proximaDireccionArgumento(t_direccion*, int, int);
void armarDireccionDeFuncion(t_direccion* );
void destruirPaquete(paquete *);
void enviarDirAMemoria(t_direccion*, long );
void punteroADir(int, t_direccion*);
void inicializarVariable(t_variable *, t_nombre_variable ,t_direccion *);
#endif /* ANSISOP_H_ */
