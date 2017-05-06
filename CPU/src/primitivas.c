/*
 * primitivas.c
 *
 *  Created on: 6/5/2017
 *      Author: utnso
 */
#include "ansiSop.h"
//--------------------------------Algo---------------------------------------
int convertirDireccionAPuntero(t_direccion* dire)
{
	int direccionOriginal,pag,desplazamiento;
	pag=(dire->pagina)*tamPagina;
	desplazamiento=dire->offset;
	direccionOriginal=pag+desplazamiento;
	return direccionOriginal;
}
//--------------------------------------------------------------------------
void asignar (t_puntero direccion_variable, t_valor_variable valor)
{
	t_direccion *dir= malloc(sizeT_direccion);
	punteroADir(direccion_variable, dir);
	log_info(log, "%ld\n", valor);
	enviarDirAMemoria(dir, valor);
	free(dir);
}
t_puntero definirVariable(t_nombre_variable identificador_variable)
{
		log_info(log,"Definir una variable %c\n", identificador_variable);
		t_direccion *direccion_variable= malloc(sizeT_direccion);
		t_variable *variable= malloc(sizeof(t_variable));
		t_contexto *contexto = malloc(sizeof(t_contexto));
		/* Hasta que no hagan PCB no se puede hacer */
		contexto= (t_contexto*)(list_get(PCB->contextoActual, PCB->tamContextoActual-1));
		if(PCB->tamContextoActual==1 &&  contexto->tamVars==0 )
		{
				armarDireccionPagina(direccion_variable); /* falta funcion*/
				variable->direccion=direccion_variable;
				variable->etiqueta=identificador_variable;
				list_add(contexto->vars, variable);
				contexto->retPos=0;
				contexto->tamVars++;
		}
		else
		{
			if(identificador_variable>='0' && identificador_variable<='9'){
					log_info(log,"Se esta creando el argumento %c\n", identificador_variable);
					armarDireccionDeArgumento(direccion_variable);
					list_add(contexto->args, direccion_variable);
					log_info(log,"La direccion de argumento %c es %d %d %d\n", identificador_variable, direccion_variable->pagina, direccion_variable->offset, direccion_variable->tam);
					contexto->tamArgs++;
				}
			else
			{
				if(contexto->tamVars == 0 && (PCB->tamContextoActual)>1)
				{
					log_info(log,"Declarando variable %c de funcion\n", identificador_variable);
					armarDireccionDeFuncion(direccion_variable);
					inicializarVariable(variable,identificador_variable,direccion_variable);
					variable->direccion=direccion_variable;
					list_add(contexto->vars, variable);
					contexto->tamVars++;
				}
				else {
						armarProximaDireccion(direccion_variable);
						inicializarVariable(variable,identificador_variable,direccion_variable);
						list_add(contexto->vars, variable);
						contexto->tamVars++;
					}
			}
			long valor;
			log_info(log,"Alocado %ld",valor);
			int dirReturn = convertirDireccionAPuntero(direccion_variable);
				if(dirReturn+3>variableMaxima){
					log_info(log,"No hay espacio para definir variable %c. Abortando programa\n", identificador_variable);
					return 1;
				}else{
					enviarDirAMemoria(direccion_variable, valor);
					log_info(log,"Devuelvo direccion: %d\n", dirReturn);
					return (dirReturn);
			}
		}
return 1;
}


/*t_puntero obtenerPosicionVariable(t_nombre_variable identifcador_variable)
{
	return NULL;
}
t_puntero dereferenciar (t_puntero direccion_variable)
{
	return NULL;
}*/

/*t_valor_variable obtenerValorCompartida (t_nombre_compartida variable)
{
	return NULL;
}
t_valor_variable asignarValorCompartida (t_nombre_compartida variable, t_valor_variable valor)
{
	return NULL;
}
void irAlLabel (t_nombre_etiqueta t_nombre_etiqueta)
{

}
void llamarSinRetorno (t_nombre_etiqueta etiqueta)
{

}
void llamarConRetorno (t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{

}
void finalizar ()
{

}*/
void retornar(t_valor_variable retorno)
{
		log_info(log,"La posicion de retorno %d\n", retorno);
		int posicionContextoActual;
		int direccion;
		t_contexto * contextoFinal;
		posicionContextoActual= (PCB->tamContextoActual)-1;
		contextoFinal= list_get(PCB->contextoActual, posicionContextoActual);
		direccion=convertirDireccionAPuntero(&(contextoFinal->retVar));
		asignar(direccion,retorno);
		PCB->programCounter=contextoFinal->retPos;
		while(contextoFinal->tamVars!=0){
			free(((t_variable*)list_get(contextoFinal->vars, contextoFinal->tamVars-1))->direccion);
			free(list_get(contextoFinal->vars, contextoFinal->tamVars-1));
			contextoFinal->tamVars--;
		}
		list_destroy(contextoFinal->vars);
		log_info(log,"Se destruyeron vars de funcion\n");
		while(contextoFinal->tamVars!=0){
				log_info(log,"Antes del free\n");
				free((t_direccion*)list_get(contextoFinal->args, contextoFinal->tamArgs-1));
				log_info(log,"Despues del free\n");
				contextoFinal->tamArgs--;
			}
		list_destroy(contextoFinal->args);
		log_info(log,"Destrui args de funcion\n");
		free(list_get(PCB->contextoActual, PCB->tamContextoActual-1));
		log_info(log,"Contexto Destruido\n");
		PCB->tamContextoActual--;

	}



