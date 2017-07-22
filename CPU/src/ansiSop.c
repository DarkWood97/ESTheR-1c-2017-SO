
#include "funcionesCpu.h"
#include "ansiSop.h"






//-----------------ESTRUCTURAS---------------------------------------
//typedef struct{
//  int numPagina;
//  int offset;
//  int tamanioPuntero;
//}direccion;

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

direccion *obtenerDireccionDePuntero(t_puntero puntero){
	direccion* direccionDelDato = malloc(sizeof(direccion));
	direccionDelDato->numPagina = puntero/tamPaginasMemoria;
	direccionDelDato->offset = puntero % tamPaginasMemoria;
	direccionDelDato->tamanioPuntero = 4;
	return direccionDelDato;
}

void realizarPeticionDeLecturaAMemoria(direccion* direccionVirtual){
	void *mensajeDePeticionLectura = malloc(sizeof(int)+sizeof(direccion));
	memcpy(mensajeDePeticionLectura, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDePeticionLectura+sizeof(int), direccionVirtual, sizeof(direccion));
	sendRemasterizado(socketMemoria, LEER_DATOS, sizeof(direccion)+sizeof(int), mensajeDePeticionLectura);
}

t_valor_variable recibirLecturaDeMemoria(){
	t_valor_variable valorDeVariable;
	paquete *paqueteConDatosDeLectura = recvRemasterizado(socketMemoria);
	if(paqueteConDatosDeLectura->tipoMsj == DATOS_DE_PAGINA){
		valorDeVariable = *(int*)paqueteConDatosDeLectura->mensaje;
	}else{
		log_info(loggerCPU, "No se recibio correctamente el valor de la variable de memoria.");
		exit(-1);
	}
	free(paqueteConDatosDeLectura);
	return valorDeVariable;
}

void realizarPeticionDeEscrituraEnMemoria(direccion* direcVirtual, t_valor_variable valorAAsignar){
	void *buffer = malloc(sizeof(direccion)+sizeof(t_valor_variable)+sizeof(int));
	memcpy(buffer,&pcbEnProceso->pid, sizeof(int));
	memcpy(buffer+sizeof(int),direcVirtual,sizeof(direccion));
	memcpy(buffer + sizeof(direccion) +sizeof(int), &valorAAsignar, sizeof(t_valor_variable));
	sendRemasterizado(socketMemoria, PETICION_ESCRITURA_MEMORIA , sizeof(int)+sizeof(direccion)+sizeof(t_valor_variable), buffer);
	free(buffer);
}

t_valor_variable recibirValorCompartida(){
	t_valor_variable valorVariableCompartida;
	paquete *paqueteDeVariableCompartida = recvRemasterizado(socketKernel);
	if(paqueteDeVariableCompartida->tipoMsj == DATOS_VARIABLE_COMPARTIDA_KERNEL){
		valorVariableCompartida = *(t_valor_variable*)paqueteDeVariableCompartida->mensaje;
		log_info(loggerCPU, "Se recibio el valor de la variable compartida %d...", valorVariableCompartida);
	}else if(paqueteDeVariableCompartida->tipoMsj == OPERACION_VARIABLE_COMPARTIDA_FALLIDA){
		log_info(loggerCPU, "No se recibio correctamente el valor de la variable compartida de kernel...");
		exit(-1);
	}
	free(paqueteDeVariableCompartida);
	return valorVariableCompartida;
}

t_valor_variable consultarVariableCompartida(char* nombreDeLaVariable){
	int tamanioNombreVariable = string_length(nombreDeLaVariable);
	void *mensajeDeVarCompartida = malloc(sizeof(int)*2+tamanioNombreVariable);
	memcpy(mensajeDeVarCompartida, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeVarCompartida+sizeof(int), &tamanioNombreVariable, sizeof(int));
	memcpy(mensajeDeVarCompartida+sizeof(int)*2, nombreDeLaVariable, tamanioNombreVariable);
	sendRemasterizado(socketKernel, PETICION_VARIABLE_COMPARTIDA_KERNEL, sizeof(int)*2+tamanioNombreVariable, mensajeDeVarCompartida);
	free(mensajeDeVarCompartida);
	free(nombreDeLaVariable);
	return recibirValorCompartida();
}

void enviarModificacionDeValor(char* nombreDeVariableAModificar, t_valor_variable valor){
	int tamanioNombreVariable = string_length(nombreDeVariableAModificar);
	int tamanioDelMensaje = tamanioNombreVariable+sizeof(t_valor_variable)+sizeof(int)*2;
	void *buffer = malloc(tamanioDelMensaje);
	memcpy(buffer, &pcbEnProceso->pid, sizeof(int));
	memcpy(buffer+sizeof(int),&tamanioNombreVariable,sizeof(int));
	memcpy(buffer+sizeof(int)*2, nombreDeVariableAModificar, tamanioNombreVariable);
	memcpy(buffer+sizeof(int)*2+tamanioNombreVariable, &valor, sizeof(t_valor_variable));
	sendRemasterizado(socketKernel, PETICION_CAMBIO_VALOR_KERNEL, tamanioDelMensaje, buffer);
	free(buffer);
}

//void finalizarProceso(PCB *pcb){
////	programaEnEjecucionFinalizado = 1;
////	void* pcbSerializada;
////	pcbSerializada = serializarPCB(pcb);
////	sendRemasterizado(socketKernel, FINALIZO_PROCESO, sacarTamanioPCB(pcb), pcbSerializada);
////	if(sigusr1EstaActivo){
////		yaMePuedoDesconectar = true;
////	}
////	free(pcbSerializada);
//}

int obtenerPCAnterior(PCB *pcb){
	stack* contextoAEvaluar;
	contextoAEvaluar = list_get(pcb->indiceStack, list_size(pcb->indiceStack)-1);
	return contextoAEvaluar->retPos;
}

 void liberarElemento(void* elemento){
   free(elemento);
 }

 int obtenerPosicionDeStackAnterior(){
 	stack *contextoQueNecesito;
 	contextoQueNecesito = list_get(pcbEnProceso->indiceStack, list_size(pcbEnProceso->indiceStack)-1);
 	int posicionARetornar = contextoQueNecesito->pos;
 	//free(contextoQueNecesito);
 	return posicionARetornar;
 }

 void generarContexto(stack *contextoAGenerar){
   contextoAGenerar->args = list_create();
   contextoAGenerar->vars = list_create();
   contextoAGenerar->retPos = pcbEnProceso->programCounter;
   contextoAGenerar->pos = obtenerPosicionDeStackAnterior()+1;
 }

 stack* obtenerContextoAnterior(){
   return list_get(pcbEnProceso->indiceStack, pcbEnProceso->posicionStackActual-1);
 }

 t_puntero convertirDeDireccionAPuntero(direccion *direccionAConvertir){
   t_puntero punteroDeVariable;
   punteroDeVariable = direccionAConvertir->numPagina*tamPaginasMemoria +direccionAConvertir->offset;
   return punteroDeVariable;
 }

 direccion *obtenerDireccionDeVariable(variable* variableAObtenerDirec){
   direccion* direccionDeVariable;
   direccionDeVariable = variableAObtenerDirec->direccionDeVariable;
   return direccionDeVariable;
 }

 int cantidadDePaginasDeStack(int espacioUsado){
     //STACK_SIZE = A CANTIDAD DE ESPACIO USADO?
	 return 1;
 }

 void agregarArgumentoAStack(variable * variableAAgregar){
     stack* stackParaAgregarArgumento = list_get(pcbEnProceso->indiceStack, pcbEnProceso->posicionStackActual);
     log_info(loggerProgramas, "Obteniendo posicion del stack donde debe agregarse el argumento...");
     if(stackParaAgregarArgumento == NULL){
         log_info(loggerProgramas, "La poscicion del stack no existe...");
         log_info(loggerProgramas, "Creando posicion de stack...");
         stackParaAgregarArgumento = malloc(sizeof(stack));
         stackParaAgregarArgumento->args = list_create();
         stackParaAgregarArgumento->vars = list_create();
         stackParaAgregarArgumento->pos = pcbEnProceso->posicionStackActual;
         log_info(loggerProgramas, "Nueva posicion de stack creada correctamente...");
     }
     log_info(loggerProgramas, "Agregando argumento a args...");
     list_add(stackParaAgregarArgumento->args, variableAAgregar);
 }

 void agregarVariableAStack(variable* variableAAgregar){
     stack *stackParaAgregarVariable = list_get(pcbEnProceso->indiceStack, pcbEnProceso->posicionStackActual);
     log_info(loggerProgramas, "Obteniendo posicion del stack donde debe agregarse la variable...");
     if(stackParaAgregarVariable == NULL){
         log_info(loggerProgramas, "La poscicion del stack no existe...");
         log_info(loggerProgramas, "Creando posicion de stack...");
         stackParaAgregarVariable = malloc(sizeof(stack));
         stackParaAgregarVariable->args = list_create();
         stackParaAgregarVariable->vars = list_create();
         stackParaAgregarVariable->pos = pcbEnProceso->posicionStackActual;
         log_info(loggerProgramas, "Nueva posicion de stack creada correctamente...");
     }else{
         log_info(loggerProgramas, "La posicion de stack %d existe...", pcbEnProceso->posicionStackActual);
     }
     log_info(loggerProgramas, "Agregando variable a vars...");
     list_add(stackParaAgregarVariable->vars, variableAAgregar);
}

 void mandarAGuardarDatosDeArchivo(paquete *paqueteConDatos, t_puntero punteroDeDatos){
 	if(paqueteConDatos->tipoMsj == DATOS_DE_ARCHIVO){
 		log_info(loggerProgramas, "El proceso %d leyo exitosamente del archivo...", pcbEnProceso->pid);
 		void *bufferParaKernel = malloc(sizeof(t_puntero)+paqueteConDatos->tamMsj);
 		memcpy(bufferParaKernel, &punteroDeDatos, sizeof(t_puntero));
 		memcpy(bufferParaKernel+sizeof(t_puntero), paqueteConDatos->mensaje, paqueteConDatos->tamMsj);
 		sendRemasterizado(socketKernel, GUARDAR_DATOS_ARCHIVO, sizeof(t_puntero)+paqueteConDatos->tamMsj, bufferParaKernel);
 		free(paqueteConDatos);
 		free(bufferParaKernel);
 		int respuestaDeKernel = recvDeNotificacion(socketKernel);
 		if(respuestaDeKernel != OPERACION_CON_ARCHIVO_EXITOSA){
 			log_info(loggerProgramas, "El proceso %d no pudo guardar los datos leidos del archivo...", pcbEnProceso->pid);
 			log_info(loggerProgramas, "Se prosigue a abortar el proceso %d...", pcbEnProceso->pid);
 			programaEnEjecucionAbortado = true;
 		}
 	}else{
 		log_info(loggerProgramas, "El proceso %d no pudo leer del archivo...", pcbEnProceso->pid);
 		log_info(loggerProgramas, "Se prosigue a abortar el proceso %d...", pcbEnProceso->pid);
 		programaEnEjecucionAbortado = true;
 	}
 }
