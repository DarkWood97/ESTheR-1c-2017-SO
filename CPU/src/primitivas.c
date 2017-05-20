/*
 * primitivas.c
 *
 *  Created on: 6/5/2017
 *      Author: utnso
 */
#include "ansiSop.h"
//--------------------------------Algo---------------------------------------
int convertirDireccionAPuntero(retVar * dire)
{
	int direccionOriginal,pag,desplazamiento,auxiliar;
	auxiliar=(dire->pagina);
	pag=auxiliar*tamPagina;
	desplazamiento=dire->offset;
	direccionOriginal=pag+desplazamiento;
	return direccionOriginal;
}
//--------------------------------------------------------------------------
void asignar (t_puntero direccion_variable, t_valor_variable valor)
{
	retVar *dir= malloc(size_retVar);
	punteroADir(direccion_variable, dir);
	log_info(log, "%ld\n", valor);
	enviarDirAMemoria(dir, valor);
	free(dir);
}
t_puntero definirVariable(t_nombre_variable identificador_variable)
{
		log_info(log,"Definir una variable %c\n", identificador_variable);
		retVar *direccion_variable= malloc(size_retVar);
		t_variable *variable= malloc(sizeof(t_variable));
		t_contexto *contexto = malloc(sizeof(t_contexto));
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
			long valor=0;
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


t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable)
{
	log_info(log,"Quiero obtener la variable %s", identificador_variable);
	int posicionStack, direccionDeRetorno;
	posicionStack=PCB->tamContextoActual;
	retVar * auxiliar;
	bool _comparacionDeIdentificadorVariable_()
	{
		return identificador_variable>='0' && identificador_variable<='9';
	}
	if(_comparacionDeIdentificadorVariable_())
	{
		auxiliar = (retVar*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->args, (int)identificador_variable-48));
		direccionDeRetorno=convertirDireccionAPuntero(auxiliar);
		log_info(log,"Obtuve el valor de %c: %d %d %d\n", identificador_variable, auxiliar->pagina, auxiliar->offset, auxiliar->tam);
		return(direccionDeRetorno);
	}
	else
	{
		t_variable * nueva;
		int posicionMaxima;
		posicionMaxima=(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->tamVars)-1;
		for(;posicionMaxima>=0;posicionMaxima--)
			{
				nueva=((t_variable*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->vars, posicionMaxima)));
				log_info(log,"Etiqueta de variable: %c\n", nueva->etiqueta);
				if(nueva->etiqueta==identificador_variable)
				{
					retVar* auxiliar;
					auxiliar= ((t_variable*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->vars, posicionMaxima)))->direccion;
					direccionDeRetorno= convertirDireccionAPuntero(auxiliar);
					log_info(log,"Obtuve el valor de %c: %d %d %d\n", nueva->etiqueta, nueva->direccion->pagina, nueva->direccion->offset, nueva->direccion->tam);
					return (direccionDeRetorno);
			}
		}
	}
	programaAbortado=1;
	return -1;
}
t_puntero dereferenciar (t_puntero direccion_variable)
{
	retVar * direccion =malloc(size_retVar);
	paquete * paquete, *auxiliar;
	paquete=malloc(sizeof(paquete));
	auxiliar=malloc(sizeof(paquete));
	int valor;
	punteroADir(direccion_variable,direccion);
	enviarDireccionALeerKernel(direccion,direccion_variable);
	free(direccion);
	deserealizarConRetorno(socketKernel,auxiliar);
	memcpy(&valor, paquete->mensaje, 4);
	destruirPaquete(paquete);
	log_info(log,"El valor dereferenciado es: %d", valor);
	destruirPaquete(auxiliar);
	return valor;
}

/*t_valor_variable obtenerValorCompartida (t_nombre_compartida variable)
{
	return NULL;
}*/
t_valor_variable asignarValorCompartida (t_nombre_compartida variable, t_valor_variable valor)
{
	paquete  paqueteAEnviar;
	int tamVariable=sizeof(variable);
	char* variable_compartida=malloc(tamVariable);
	memcpy(variable_compartida, &valor, tamVariable);
	log_info(log,"Variable %s le asigno %d\n", variable, variable_compartida[0]);
	paqueteAEnviar=serializar(variable_compartida, MENSAJE_VARIABLE_COMPARTIDA);
	realizarHandshake(socketKernel,paqueteAEnviar);
	destruirPaquete(&paqueteAEnviar);
	free(variable_compartida);
	return valor;

}
void irAlLabel (t_nombre_etiqueta t_nombre_etiqueta)
{
	t_puntero_instruccion instruccion;
	int longitud_etiqueta=strlen(t_nombre_etiqueta);
	log_info(log, "Busco la etiqueta: %s y mide: %d\n", t_nombre_etiqueta, longitud_etiqueta);
	instruccion = metadata_buscar_etiqueta(t_nombre_etiqueta, PCB->etiquetas, PCB->tamEtiquetas);
	log_info(log,"Ir a la instruccion %d\n", instruccion);
	PCB->programCounter=instruccion-1; //(contador/2);
	log_info(log,"Saliendo de label\n");
}
void llamarSinRetorno (t_nombre_etiqueta etiqueta)
{

}
void llamarConRetorno (t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{

}
void finalizar ()
{

}
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
				free((retVar*)list_get(contextoFinal->args, contextoFinal->tamArgs-1));
				log_info(log,"Despues del free\n");
				contextoFinal->tamArgs--;
			}
		list_destroy(contextoFinal->args);
		log_info(log,"Destrui args de funcion\n");
		free(list_get(PCB->contextoActual, PCB->tamContextoActual-1));
		log_info(log,"Contexto Destruido\n");
		PCB->tamContextoActual--;

	}



