
#include "funcionesCpu.h"


#define HANDSHAKE_MEMORIA 1001
#define HANDSHAKE_KERNEL 1002
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define MENSAJE_DIRECCION 110
#define ERROR -1
#define CORTO 0
#define MENSAJE_VARIABLE_COMPARTIDA 111



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
}

void enviarDirAMemoria(t_direccion* direccion, long valor)
{
		char* escritura= malloc(sizeof(char)*16);
		paquete paqueteAEnviar,auxiliar;
		memcpy(escritura, &direccion->pagina , 4);
		memcpy(escritura+4, &direccion->offset , 4);
		memcpy(escritura+8, &direccion->tam , 4);
		memcpy(escritura+12, &valor , 4);
		auxiliar=serializar(escritura,MENSAJE_DIRECCION);
		memcpy(&paqueteAEnviar, &auxiliar, sizeof(auxiliar)); /*no se si es sizeof(paquete)*/
		realizarHandshake(socketMemoria,paqueteAEnviar);
		destruirPaquete(&paqueteAEnviar);
		destruirPaquete(&auxiliar);
		free(escritura);
}

void punteroADir(int puntero, t_direccion* dir)
{
	if(tamPagina>puntero)
	{
		dir->pagina=0;
		dir->offset=puntero;
		dir->tam=4;
	}
	else
	{
		dir->pagina = (puntero/tamPagina);
		dir->offset = puntero%tamPagina;
		dir->tam=4;
	}
}
void inicializarVariable(t_variable *variable, t_nombre_variable identificador_variable,t_direccion *direccion_variable)
{
	variable->direccion=direccion_variable;
	variable->etiqueta=identificador_variable;
}

