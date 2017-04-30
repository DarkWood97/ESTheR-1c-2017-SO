
#include "funcionesCPU.h"

//-----------------ESTRUCTURAS---------------------------------------
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
//-----------------------VARIABLES GLOBALES------------------------------
int sizeT_direccion= sizeof(t_direccion);
//----------------------------FUNCIONES AUXILIARES----------------------
//*****************BOOL*************************************************
bool estaEntre(void* menor, void* mayor, void* queComparar )
{
	return ((queComparar>=menor) && (queComparar<=mayor));
}
bool esMenor (void* a, void* queComparar)
{
	return a>queComparar;
}
//******************T_VARIABLE*******************************************

//****************VOID*********************************************
void proximaDireccion(t_direccion* direccionOriginal,int posicionStack, int ultimaPosicion)
{
	t_direccion *auxiliar;
	auxiliar=malloc(sizeT_direccion);
	int offsetAux=((t_variable*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->vars, ultimaPosicion)))->direccion->offset + 4;
	if(offsetAux>=tamPagina)
	{
				auxiliar->pagina = ((t_variable*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->vars, ultimaPosicion)))->direccion->pagina + 1;
				auxiliar->offset = 0;
				auxiliar->tam=4;
				memcpy(direccionOriginal, auxiliar , sizeof(t_direccion));
				free(auxiliar);
	}
	else
	{			auxiliar->pagina = ((t_variable*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->vars, ultimaPosicion)))->direccion->pagina;
				auxiliar->offset = offsetAux;
				auxiliar->tam=4;
				memcpy(direccionOriginal, auxiliar , sizeof(t_direccion));
				free(auxiliar);
	}
}
void armarDireccionPagina(t_direccion * direccionOriginal)
{
	t_direccion *auxiliar;
	auxiliar=malloc(sizeT_direccion);
	auxiliar->tam=4;
	auxiliar->offset=0;
	auxiliar->pagina = PCB->paginasCodigo;
	memcpy(direccionOriginal,auxiliar,sizeT_direccion);
	free(auxiliar);
}
void armarDireccionDeArgumento(t_direccion * direccionOriginal)
{
	if(((t_contexto*)list_get(PCB->contextoActual, PCB->tamContextoActual-1))->tamArgs == 0){
		log_info(log,"No hay argumentos\n");
		int posicionAnterior = PCB->tamContextoActual-2;
		int UltimaPocisionVariable = ((t_contexto*)(list_get(PCB->contextoActual, PCB->tamContextoActual-2)))->tamVars-1;
		proximaDireccion(direccionOriginal,posicionAnterior, UltimaPocisionVariable);
		}
		else {
		log_info(log,"Busco ultimo argumento\n");
		int posicionStackActual = PCB->tamContextoActual-1;
		int posicionUltimoArgumento = ((t_contexto*)(list_get(PCB->contextoActual, PCB->tamContextoActual-1)))->tamArgs-1;
		proximaDireccion(direccionOriginal,posicionStackActual, posicionUltimoArgumento);
		}
}
void armarProximaDireccion(t_direccion* direccionOriginal){
	int ultimaPosicionStack;
	ultimaPosicionStack= PCB->tamContextoActual-1;
	int posicionUltimaVariable;
	posicionUltimaVariable= ((t_contexto*)(list_get(PCB->contextoActual, ultimaPosicionStack)))->tamVars-1;
	proximaDireccion(direccionOriginal,ultimaPosicionStack, posicionUltimaVariable);
}
void proximaDireccionArgumento(t_direccion* direccionOriginal, int posicionStack, int posicionUltimaVariable ){
	t_direccion *direccion;
	int offset;
	direccion= malloc(sizeT_direccion);
	offset= ((t_direccion*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->args, posicionUltimaVariable)))->offset + 4;
	log_info(log,"Entre a proxima direccion Argumento\n");
	log_info(log,"Offset siguiente es %d\n", offset);
		if(offset>=tamPagina){
			direccion->pagina = ((t_direccion*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->args, posicionUltimaVariable)))->pagina + 1;
			direccion->offset = 0;
			direccion->tam=4;
			memcpy(direccionOriginal, direccion ,sizeT_direccion);
			free(direccion);
		}
		else
			{
				direccion->pagina = ((t_direccion*)(list_get(((t_contexto*)(list_get(PCB->contextoActual, posicionStack)))->args, posicionUltimaVariable)))->pagina;
				direccion->offset = offset;
				direccion->tam=4;
				memcpy(direccionOriginal, direccion , sizeT_direccion);
				free(direccion);
		}

		return;
}

void armarDireccionDeFuncion(t_direccion* direccionOriginal)
{
	if(((t_contexto*)list_get(PCB->contextoActual, PCB->tamContextoActual-1))->tamArgs == 0 && ((t_contexto*)list_get(PCB->contextoActual, PCB->tamContextoActual-1))->tamVars == 0)
		{
			log_info(log,"Entrando a definir variable en contexto sin argumentos y sin vars\n");
			int posicionStackAnterior = PCB->tamContextoActual-2;
			int posicionUltimaVariable = ((t_contexto*)(list_get(PCB->contextoActual,PCB->tamContextoActual-2)))->tamVars-1;
			proximaDireccion(direccionOriginal,posicionStackAnterior, posicionUltimaVariable);
		}
	else
		{
			if(((t_contexto*)list_get(PCB->contextoActual, PCB->tamContextoActual-1))->tamArgs != 0 && ((t_contexto*)list_get(PCB->contextoActual, PCB->tamContextoActual-1))->tamVars == 0)
				{

			log_info(log,"Entrando a definir variable a partir del ultimo argumento\n");
			int posicionStackActual = PCB->tamContextoActual-1;
			int posicionUltimoArgumento = ((t_contexto*)(list_get(PCB->contextoActual, PCB->tamContextoActual-1)))->tamArgs-1;
			proximaDireccionArgumento(direccionOriginal,posicionStackActual, posicionUltimoArgumento);
			}
			else
			{
				if(((t_contexto*)list_get(PCB->contextoActual, PCB->tamContextoActual-1))->tamVars != 0){

				log_info(log,"Entrando a definir variable a partir de la ultima variable\n");
				int posicionStackActual = PCB->tamContextoActual-1;
				int posicionUltimaVariable = ((t_contexto*)(list_get(PCB->contextoActual, PCB->tamContextoActual-1)))->tamVars-1;
				proximaDireccion(direccionOriginal,posicionStackActual, posicionUltimaVariable);
			}



	}
}
//---------------------------------FUNCIONES-------------------------
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
					variable=(t_variable*)asignar(identificador_variable, direccion_variable);
					list_add(contexto->vars, variable);
					contexto->tamVars++;
				}
				else {
						armarProximaDireccion(direccion_variable);
						variable=(t_variable*)asignar(identificador_variable, direccion_variable);
						list_add(contexto->vars, variable);
						contexto->tamVars++;
					}
			}
			char* escritura= malloc(sizeof(char)*16);
			free(escritura);
			/* falta escritura en memoria*/
		}
return 1;
}
t_puntero obtenerPosicionVariable(t_nombre_variable identifcador_variable)
{
	return NULL;
}
t_puntero dereferenciar (t_puntero direccion_variable)
{
	return NULL;
}
void asignar (t_puntero direccion_variable, t_valor_variable valor)
{

}
t_valor_variable obtenerValorCompartida (t_nombre_compartida variable)
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
}
