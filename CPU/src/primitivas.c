/*
 * primitivas.c
 *
 *  Created on: 6/5/2017
 *      Author: utnso
 */
#include "ansiSop.h"
#define VARIABLE 1534
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
t_valor_variable obtenerValor(int socketKernel, t_nombre_compartida valor)
{
	char *nombre= malloc(string_length(valor)+1);
	//char* barra_cero="\0";
	int loQueVale;
	memcpy(nombre, valor, string_length(valor));
	memcpy(nombre+(string_length(valor)), "\0", 1);
	paquete paquete_nuevo;
	paquete_nuevo = serializar(valor, VARIABLE);
	enviar(socketKernel, paquete_nuevo);
	intMultiproposito=loQueVale;
	deserealizarMensaje(socketKernel);
	free(nombre);
	destruirPaquete(&paquete_nuevo);
	return loQueVale;
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
		contexto= (t_contexto*)(list_get(pcb->contextoActual, pcb->tamContextoActual-1));
		if(pcb->tamContextoActual==1 &&  contexto->tamVars==0 )
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
					log_info(log,"Creando el argumento %c\n", identificador_variable);
					armarDireccionDeArgumento(direccion_variable);
					list_add(contexto->args, direccion_variable);
					log_info(log,"La direccion de argumento %c es %d %d %d\n", identificador_variable, direccion_variable->pagina, direccion_variable->offset, direccion_variable->tam);
					contexto->tamArgs++;
				}
			else
			{
				if(contexto->tamVars == 0 && (pcb->tamContextoActual)>1)
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
	posicionStack=pcb->tamContextoActual;
	retVar * auxiliar;
	bool _comparacionDeIdentificadorVariable_()
	{
		return identificador_variable>='0' && identificador_variable<='9';
	}
	if(_comparacionDeIdentificadorVariable_())
	{
		auxiliar = (retVar*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->args, (int)identificador_variable-48));
		direccionDeRetorno=convertirDireccionAPuntero(auxiliar);
		log_info(log,"Obtuve el valor de %c: %d %d %d\n", identificador_variable, auxiliar->pagina, auxiliar->offset, auxiliar->tam);
		return(direccionDeRetorno);
	}
	else
	{
		t_variable * nueva;
		int posicionMaxima;
		posicionMaxima=(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->tamVars)-1;
		for(;posicionMaxima>=0;posicionMaxima--)
			{
				nueva=((t_variable*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->vars, posicionMaxima)));
				log_info(log,"Etiqueta de variable: %c\n", nueva->etiqueta);
				if(nueva->etiqueta==identificador_variable)
				{
					retVar* auxiliar;
					auxiliar= ((t_variable*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->vars, posicionMaxima)))->direccion;
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

t_valor_variable obtenerValorCompartida (t_nombre_compartida variable)
{
	t_valor_variable valor = obtenerValor(socketKernel,variable);
	return valor;
}

t_valor_variable asignarValorCompartida (t_nombre_compartida variable, t_valor_variable valor)
{
	paquete  paqueteAEnviar;
	int tamVariable=sizeof(variable);
	char* variable_compartida=malloc(tamVariable);
	memcpy(variable_compartida, &valor, tamVariable);
	log_info(log,"Variable %s le asigno %d\n", variable, variable_compartida[0]);
	paqueteAEnviar=serializar(variable_compartida, MENSAJE_VARIABLE_COMPARTIDA);
	enviar(socketKernel,paqueteAEnviar);
	realizarHandshake(socketKernel,1002);
	destruirPaquete(&paqueteAEnviar);
	free(variable_compartida);
	return valor;

}
void irAlLabel (t_nombre_etiqueta t_nombre_etiqueta)
{
	log_info(log,"Se encuentra ejecutando ir a label: %s", t_nombre_etiqueta);
	t_puntero_instruccion instruccion;
	instruccion = metadata_buscar_etiqueta(t_nombre_etiqueta, pcb->etiquetas, pcb->tamEtiquetas);
	pcb->ProgramCounter=instruccion-1;
}
void llamarSinRetorno (t_nombre_etiqueta etiqueta)
{
	log_info(log, "Se encuentra ejecutando Llamar sin retorno en la funcion: %s \n",etiqueta);

}
void llamarConRetorno (t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{
	log_info(log, "Se encuentra ejecutando Llamar con retorno a funcion: %s, Retorno: %d\n",etiqueta,donde_retornar);
	t_puntero_instruccion posicion= pcb->tamContextoActual;
	log_info(log, "El indice de la funcion es: %d",posicion);
	t_contexto * nuevoContexto= crearContexto();
	log_info(log,"Se ha creado el contexto con la posicion: %d que debe volver en la sentencia %d y retorno en la variable de pos %d %d\n", nuevoContexto->posicion, nuevoContexto->retPos, nuevoContexto->retVar.pagina, nuevoContexto->retVar.offset);
	nuevoContexto->retPos = pcb->ProgramCounter;
	pcb->ProgramCounter = posicion;
	retVar * direccionDeRetorno;
	punteroADir(donde_retornar, direccionDeRetorno);
	nuevoContexto->retVar.pagina = direccionDeRetorno->pagina;
	nuevoContexto->retVar.offset = direccionDeRetorno->offset;
	nuevoContexto->retVar.tam = direccionDeRetorno->tam;
	free(direccionDeRetorno);
}
void finalizar ()
{
	log_info(log,"Se encuentra ejecutando finalizar");
	programaFinalizado = 1;
}
void retornar(t_valor_variable retorno)
{
		log_info(log,"Se encuentra ejecutando retornar\n");
		retVar * direccion= pcb->cod.offset-1;
		log_info(log,"Escribir en variable (%d,%d,%d) el valor de retorno: %d",direccion->pagina,direccion->offset,direccion->tam,retorno);
		destruirContextoActual(tamPagina);
		//escribirMemoria(retorno,direccion)
}

