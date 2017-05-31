
#include "funcionesCpu.h"


#define HANDSHAKE_MEMORIA 1001
#define HANDSHAKE_KERNEL 1002
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define PETICION_LECTURA 104
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
void proximaDireccion(retVar* direccionOriginal,int posicionStack, int ultimaPosicion)
{
	t_direccion *auxiliar;
	auxiliar=malloc(sizeT_direccion);
	int offsetAux=((t_variable*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->vars, ultimaPosicion)))->direccion->offset + 4;
	if(offsetAux>=tamPagina)
	{
				auxiliar->pagina = ((t_variable*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->vars, ultimaPosicion)))->direccion->pagina + 1;
				auxiliar->offset = 0;
				auxiliar->tam=4;
				memcpy(direccionOriginal, auxiliar , sizeof(retVar));
				free(auxiliar);
	}
	else
	{			auxiliar->pagina = ((t_variable*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->vars, ultimaPosicion)))->direccion->pagina;
				auxiliar->offset = offsetAux;
				auxiliar->tam=4;
				memcpy(direccionOriginal, auxiliar , sizeof(retVar));
				free(auxiliar);
	}
}
void armarDireccionPagina(retVar * direccionOriginal)
{
	retVar *auxiliar;
	auxiliar=malloc(sizeT_direccion);
	auxiliar->tam=4;
	auxiliar->offset=0;
	auxiliar->pagina = pcb->paginas_Codigo;
	memcpy(direccionOriginal,auxiliar,sizeT_direccion);
	free(auxiliar);
}
void armarDireccionDeArgumento(retVar * direccionOriginal)
{
	if(((t_contexto*)list_get(pcb->contextoActual, pcb->tamContextoActual-1))->tamArgs == 0){
		log_info(log,"No hay argumentos\n");
		int posicionAnterior = pcb->tamContextoActual-2;
		int UltimaPocisionVariable = ((t_contexto*)(list_get(pcb->contextoActual, pcb->tamContextoActual-2)))->tamVars-1;
		proximaDireccion(direccionOriginal,posicionAnterior, UltimaPocisionVariable);
		}
		else {
		log_info(log,"Busco ultimo argumento\n");
		int posicionStackActual = pcb->tamContextoActual-1;
		int posicionUltimoArgumento = ((t_contexto*)(list_get(pcb->contextoActual, pcb->tamContextoActual-1)))->tamArgs-1;
		proximaDireccion(direccionOriginal,posicionStackActual, posicionUltimoArgumento);
		}
}
void armarProximaDireccion(retVar* direccionOriginal){
	int ultimaPosicionStack;
	ultimaPosicionStack= pcb->tamContextoActual-1;
	int posicionUltimaVariable;
	posicionUltimaVariable= ((t_contexto*)(list_get(pcb->contextoActual, ultimaPosicionStack)))->tamVars-1;
	proximaDireccion(direccionOriginal,ultimaPosicionStack, posicionUltimaVariable);
}
void proximaDireccionArgumento(retVar* direccionOriginal, int posicionStack, int posicionUltimaVariable ){
	retVar *direccion;
	int offset;
	direccion= malloc(sizeT_direccion);
	offset= ((retVar*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->args, posicionUltimaVariable)))->offset + 4;
	log_info(log,"Entre a proxima direccion Argumento\n");
	log_info(log,"Offset siguiente es %d\n", offset);
		if(offset>=tamPagina){
			direccion->pagina = ((retVar*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->args, posicionUltimaVariable)))->pagina + 1;
			direccion->offset = 0;
			direccion->tam=4;
			memcpy(direccionOriginal, direccion ,sizeT_direccion);
			free(direccion);
		}
		else
			{
				direccion->pagina = ((retVar*)(list_get(((t_contexto*)(list_get(pcb->contextoActual, posicionStack)))->args, posicionUltimaVariable)))->pagina;
				direccion->offset = offset;
				direccion->tam=4;
				memcpy(direccionOriginal, direccion , sizeT_direccion);
				free(direccion);
		}

		return;
}

void armarDireccionDeFuncion(retVar* direccionOriginal)
{
	if(((t_contexto*)list_get(pcb->contextoActual, pcb->tamContextoActual-1))->tamArgs == 0 && ((t_contexto*)list_get(pcb->contextoActual, pcb->tamContextoActual-1))->tamVars == 0)
		{
			log_info(log,"Entrando a definir variable en contexto sin argumentos y sin vars\n");
			int posicionStackAnterior = pcb->tamContextoActual-2;
			int posicionUltimaVariable = ((t_contexto*)(list_get(pcb->contextoActual,pcb->tamContextoActual-2)))->tamVars-1;
			proximaDireccion(direccionOriginal,posicionStackAnterior, posicionUltimaVariable);
		}
	else
		{
			if(((t_contexto*)list_get(pcb->contextoActual, pcb->tamContextoActual-1))->tamArgs != 0 && ((t_contexto*)list_get(pcb->contextoActual, pcb->tamContextoActual-1))->tamVars == 0)
				{

			log_info(log,"Entrando a definir variable a partir del ultimo argumento\n");
			int posicionStackActual = pcb->tamContextoActual-1;
			int posicionUltimoArgumento = ((t_contexto*)(list_get(pcb->contextoActual, pcb->tamContextoActual-1)))->tamArgs-1;
			proximaDireccionArgumento(direccionOriginal,posicionStackActual, posicionUltimoArgumento);
			}
			else
			{
				if(((t_contexto*)list_get(pcb->contextoActual, pcb->tamContextoActual-1))->tamVars != 0){

				log_info(log,"Entrando a definir variable a partir de la ultima variable\n");
				int posicionStackActual = pcb->tamContextoActual-1;
				int posicionUltimaVariable = ((t_contexto*)(list_get(pcb->contextoActual, pcb->tamContextoActual-1)))->tamVars-1;
				proximaDireccion(direccionOriginal,posicionStackActual, posicionUltimaVariable);
			}
			}
	}
}

void enviarDirAMemoria(retVar* direccion, long valor)
{
		char* escritura= malloc(sizeof(char)*16);
		paquete paqueteAEnviar,auxiliar;
		memcpy(escritura, &direccion , 4);
		memcpy(escritura+4, &valor , 4);
		auxiliar=serializar(escritura,MENSAJE_DIRECCION);
		memcpy(&paqueteAEnviar, &auxiliar, sizeof(auxiliar)); /*no se si es sizeof(paquete)*/
		realizarHandshake(socketMemoria,HANDSHAKE_MEMORIA);
		destruirPaquete(&paqueteAEnviar);
		destruirPaquete(&auxiliar);
		free(escritura);
}

void enviarDirALeerMemoria(char* datosMemoria, retVar* dir)
{
	memcpy(datosMemoria, &dir->pagina , 4);
	memcpy(datosMemoria+4, &dir->offset , 4);
	memcpy(datosMemoria+8, &dir->tam , 4);
	paquete paqueteAEnviar,auxiliar;
	auxiliar=serializar(datosMemoria,PETICION_LECTURA);
	memcpy(&paqueteAEnviar, &auxiliar, sizeof(auxiliar));
	realizarHandshake(socketMemoria,HANDSHAKE_MEMORIA);
	destruirPaquete(&paqueteAEnviar);
	destruirPaquete(&auxiliar);
	free(datosMemoria);
}
void enviarDireccionALeerKernel(retVar* direccion, int valor)
{
		char* leer= malloc(sizeof(char)*16);
		paquete paqueteAEnviar,auxiliar;
		memcpy(leer, &direccion,4);
		memcpy(leer+4, &valor , 4);
		auxiliar=serializar(leer,MENSAJE_DIRECCION);
		memcpy(&paqueteAEnviar, &auxiliar, sizeof(auxiliar));
		realizarHandshake(socketKernel,HANDSHAKE_MEMORIA);
		destruirPaquete(&paqueteAEnviar);
		destruirPaquete(&auxiliar);
		free(leer);
}
void punteroADir(int puntero, retVar* dir)
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
void deserealizarConRetorno(int socket, paquete* aRetornar)
{
	paquete * auxiliar=malloc(sizeof(paquete));
			if(recv(socket,auxiliar,16,0)==-1)
				{
					perror("Error de receive");
				}
	auxiliar=aRetornar;
	destruirPaquete(auxiliar);
}

char* loQueTengoQueLeer (int pagina,int desplazamiento, int tam)
{
	if((tam+desplazamiento)<=tamPagina){

					char *datos= malloc(sizeof(char*)*16);
					retVar *datosMemoria=malloc(sizeof(retVar));
					datosMemoria->offset=desplazamiento;
					datosMemoria->pagina=pagina;
					datosMemoria->tam=tam;
					enviarDirALeerMemoria(datos,datosMemoria);

					paquete* instruccion;

					instruccion = deserealizarMensaje(socketMemoria);

					char* sentencia=malloc(datosMemoria->tam);
					memcpy(sentencia, instruccion->mensaje, datosMemoria->tam);
					free(datos);
					free(datosMemoria);
					destruirPaquete(instruccion);
					return sentencia;}
	else{
		char* datos ;
		//datos=leer(pagina,desplazamiento,(tamPagina-desplazamiento));
		if(datos==NULL) return NULL;
		char* datos2;
		//datos2= leer(pagina+1,0,tam-(tamPagina-desplazamiento));
		if(datos2==NULL) return NULL;

		char* nuevo =malloc((tamPagina-desplazamiento)+tam-(tamPagina-desplazamiento));
		memcpy(nuevo,datos,(tamPagina-desplazamiento));
		memcpy(nuevo+(tamPagina-desplazamiento),datos2,tam-(tamPagina-desplazamiento));
		free(datos);
		free(datos2);
		return nuevo;
	}
}
