
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

void realizarPeticionDeLecturaAMemoria(direccion* direccionVirtual, t_valor_variable sirveParaTamanio){
	void *mensajeDePeticionLectura = malloc(sizeof(int)+sizeof(direccion));
	memcpy(mensajeDePeticionLectura, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDePeticionLectura+sizeof(int), direccionVirtual, sizeof(direccion));
	sendRemasterizado(socketMemoria, PETICION_LECTURA_MEMORIA, sizeof(direccion)+sizeof(int), mensajeDePeticionLectura);
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
	}else{
		log_info(loggerCPU, "No se recibio correctamente el valor de la variable compartida de kernel...");
		exit(-1);
	}
	free(paqueteDeVariableCompartida);
	return valorVariableCompartida;
}

t_valor_variable consultarVariableCompartida(char* nombreDeLaVariable){
	void *mensajeDeVarCompartida = malloc(sizeof(int)+string_length(nombreDeLaVariable));
	memcpy(mensajeDeVarCompartida, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeVarCompartida+sizeof(int), nombreDeLaVariable, string_length(nombreDeLaVariable));
	sendRemasterizado(socketKernel, PETICION_VARIABLE_COMPARTIDA_KERNEL, string_length(nombreDeLaVariable)+sizeof(int), mensajeDeVarCompartida);
	free(mensajeDeVarCompartida);
	return recibirValorCompartida();
}

void enviarModificacionDeValor(char* nombreDeVariableAModificar, t_valor_variable valor){
	int tamanioDelMensaje = string_length(nombreDeVariableAModificar)+sizeof(t_valor_variable)+sizeof(int);
	void *buffer = malloc(tamanioDelMensaje);
	memcpy(buffer, &pcbEnProceso->pid, sizeof(int));
	memcpy(buffer+sizeof(int), nombreDeVariableAModificar, string_length(nombreDeVariableAModificar));
	memcpy(buffer+sizeof(int)+string_length(nombreDeVariableAModificar), &valor, sizeof(t_valor_variable));
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

 int obtenerCantidadDeVars(stack *contextoACalcular){
   return list_size(contextoACalcular->vars);
 }

 int obtenerCantidadDeArgs(stack *contextoACalcular){
   return list_size(contextoACalcular->args);
 }

 void generarDireccionDePrimeraPagina(direccion *direccionAnterior){
 	direccionAnterior->numPagina = 0;
 	direccionAnterior->offset = pcbEnProceso->cantidadPaginasCodigo; //LA ULTIMA PAGINA DE CODIGO ES CANTIDAD-1
 	direccionAnterior->tamanioPuntero = 4;
 }

 direccion* generarDireccionParaArgumento(int cantidadDeContextos){
   direccion *direccionAnterior;
   direccionAnterior = malloc(sizeof(direccion));
   stack *contextoActual = list_get(pcbEnProceso->indiceStack, cantidadDeContextos);
   int cantidadDeArgumentos = list_size(contextoActual->args);
   if(cantidadDeContextos == 1 && cantidadDeArgumentos == 0){
	   generarDireccionDePrimeraPagina(direccionAnterior);
   }else if(obtenerCantidadDeArgs(contextoActual) == 0){
     stack *contextoAnterior = obtenerContextoAnterior();
     direccionAnterior = obtenerDireccionDeVariable(list_get(contextoAnterior->args, list_size(contextoAnterior->args)-1));
     direccionAnterior->offset +=4;
   }else{
     direccionAnterior = obtenerDireccionDeVariable(list_get(contextoActual->args, list_size(contextoActual->args)-1));
     direccionAnterior->offset +=4;
   }
   return direccionAnterior;
 }

 direccion* generarDireccionParaVariable(int cantidadDeContextos){
   direccion *direccionAnterior;
   direccionAnterior = malloc(sizeof(direccion));
   stack *contextoActual = list_get(pcbEnProceso->indiceStack, cantidadDeContextos);
   int cantidadDeVars = list_size(contextoActual->vars);
   if(cantidadDeContextos == 1 && cantidadDeVars == 0){
     //generarDireccionDePrimeraPagina(direccionAnterior);
   }else if(obtenerCantidadDeVars(contextoActual) == 0){
     stack *contextoAnterior = obtenerContextoAnterior();
     direccionAnterior = obtenerDireccionDeVariable(list_get(contextoAnterior->vars, list_size(contextoAnterior->vars)-1));
     direccionAnterior->offset += 4;
   }else{
     direccionAnterior = obtenerDireccionDeVariable(list_get(contextoActual->vars, list_size(contextoActual->vars)-1));
     direccionAnterior->offset += 4;
   }
   return direccionAnterior;
 }

void agregarVariableAVars(direccion *direccionDeVariable, t_nombre_variable identificador_variable){
   variable* variable;
   variable = malloc(sizeof(variable));
   variable->direccionDeVariable = direccionDeVariable;
   variable->nombreVariable = identificador_variable;
   stack *contextoParaAgregar = list_get(pcbEnProceso->indiceStack, pcbEnProceso->posicionStackActual);
   list_add(contextoParaAgregar->vars, variable);
   /*
    * La otra opcion seria:
    * t_contexto *contextoParaAgregar = list_remove(pcbEnProceso->contextoActual, list_size(pcbEnProceso->contextoActual)-1);
    * list_add(contextoParaAgregar->vars, variable)
    * list_add(pcbEnProceso->contextoActual, contextoParaAgregar);
    */
 }

 void agregarVariableAArgs(direccion *direccionDeVariable, t_nombre_variable identificador_variable){
 	variable* variable;
 	variable = malloc(sizeof(variable));
 	variable->direccionDeVariable = direccionDeVariable;
 	variable->nombreVariable = identificador_variable;
 	stack* contextoParaAgregar = list_get(pcbEnProceso->indiceStack, pcbEnProceso->posicionStackActual);
 	list_add(contextoParaAgregar->args, variable);

 	//list_add((list_get(pcbEnProceso->contextoActual, list_size(pcbEnProceso->contextoActual)-1))->args, variable);
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
