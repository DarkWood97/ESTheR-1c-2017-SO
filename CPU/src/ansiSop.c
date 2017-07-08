
#include "funcionesCpu.h"
#include "ansiSop.h"






//-----------------ESTRUCTURAS---------------------------------------
typedef struct{
  int numPagina;
  int offset;
  int tamanioPuntero;
}direccion;

typedef struct __attribute__((packed))t_variable
{
	char etiqueta;
	direccion *direccion;
} t_variable;

//typedef struct __attribute__((packed))t_contexto
//{
//	int posicion;
//	t_list *args;
//	t_list *vars;
//	int retPos;
//	direccion retVar;
//	int tamArgs;
//	int tamVars;
//}contexto;

//-----------------------VARIABLES GLOBALES------------------------------
//int sizeT_direccion= sizeof(t_direccion);
//----------------------------FUNCIONES AUXILIARES----------------------

direccion *obtenerDireccionVirtual(t_puntero puntero){
	direccion* direccionDelDato = malloc(sizeof(direccion));
	direccionDelDato->numPagina = puntero/tamPaginasMemoria;
	direccionDelDato->offset = puntero % tamPaginasMemoria;
	direccionDelDato->tamanioPuntero = 4;
	return direccionDelDato;
}

void realizarPeticionDeLecturaAMemoria(direccion* direccionVirtual, t_valor_variable sirveParaTamanio){
	sendRemasterizado(socketMemoria, PETICION_LECTURA_MEMORIA, sizeof(direccion), direccionVirtual);
}

t_valor_variable recibirLecturaDeMemoria(){
	t_valor_variable valorDeVariable;
	paquete *paqueteConDatosDeLectura = recvRemasterizado(socketMemoria);
	if(paqueteConDatosDeLectura->tipoMsj == DATOS_VARIABLE_MEMORIA){
		valorDeVariable = *(int*)paqueteConDatosDeLectura->mensaje;
	}else{
		log_info(loggerCPU, "No se recibio correctamente el valor de la variable de memoria.");
		exit(-1);
	}
	free(paqueteConDatosDeLectura);
	return valorDeVariable;
}

void realizarPeticionDeEscrituraEnMemoria(direccion* direcVirtual, t_valor_variable valorAAsignar){
	void *buffer = malloc(sizeof(direccion)+sizeof(t_valor_variable));
	memcpy(buffer,direcVirtual,sizeof(direccion));
	memcpy(buffer + sizeof(direccion), &valorAAsignar, sizeof(t_valor_variable));
	sendRemasterizado(socketMemoria, PETICION_ESCRITURA_MEMORIA , sizeof(direccion)+sizeof(t_valor_variable), buffer);
	free(buffer);
}

t_valor_variable recibirValorCompartida(){
	t_valor_variable valorVariableCompartida;
	paquete *paqueteDeVariableCompartida = recvRemasterizado(socketKernel);
	if(paqueteDeVariableCompartida->tipoMsj == DATOS_VARIABLE_COMPARTIDA_KERNEL){
		valorVariableCompartida = *(t_valor_variable*)paqueteDeVariableCompartida->mensaje;
	}else{
		log_info(loggerCPU, "No se recibio correctamente el valor de la variable compartida de kernel.");
		exit(-1);
	}
	free(paqueteDeVariableCompartida);
	return valorVariableCompartida;
}

t_valor_variable consultarVariableCompartida(char* nombreDeLaVariable){
	sendRemasterizado(socketKernel, PETICION_VARIABLE_COMPARTIDA_KERNEL, string_length(nombreDeLaVariable), nombreDeLaVariable);
	return recibirValorCompartida();
}

void enviarModificacionDeValor(char* nombreDeVariableAModificar, t_valor_variable valor){
	int tamanioDelMensaje = string_length(nombreDeVariableAModificar)+sizeof(t_valor_variable);
	void *buffer = malloc(tamanioDelMensaje);
	memcpy(buffer, nombreDeVariableAModificar, string_length(nombreDeVariableAModificar));
	memcpy(buffer+string_length(nombreDeVariableAModificar), &valor, sizeof(t_valor_variable));
	sendRemasterizado(socketKernel, PETICION_CAMBIO_VALOR_KERNEL, tamanioDelMensaje, buffer);
	free(buffer);
}


void finalizarProceso(PCB *pcb){
	programaEnEjecucionFinalizado = 1;
	void* pcbSerializada;
	pcbSerializada = serializarPCB(pcb);
	sendRemasterizado(socketKernel, FINALIZO_PROCESO, sacarTamanioPCB(pcb), pcbSerializada);
	if(sigusr1EstaActivo){
		yaMePuedoDesconectar = true;
	}
	free(pcbSerializada);
}

int obtenerPCAnterior(PCB *pcb){
	t_contexto* contextoAEvaluar;
	contextoAEvaluar = list_get(pcb->contextoActual, list_size(pcb->contextoActual)-1);
	return contextoAEvaluar->retPos;
}

 void liberarElemento(void* elemento){
   free(elemento);
 }
