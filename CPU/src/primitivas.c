#include "primitivas.h"
#include "ansiSop.h"
#include "funcionesCpu.h"
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <parser/sintax.h>

//---------------------DEREFERENCIAR----------------//
t_valor_variable dereferenciar(t_puntero direccion_variable){
	log_info(loggerProgramas, "El proceso %d pidio que se haga una dereferenciacion de la direccion %d...", pcbEnProceso->pid, direccion_variable);
  t_valor_variable valorDeRetorno;
  direccion *direccionVirtual = obtenerDireccionDePuntero(direccion_variable);
  log_info(loggerProgramas, "El proceso %d pidio una lectura a memoria en la pagina %d offset %d...", pcbEnProceso->pid, direccionVirtual->numPagina, direccionVirtual->offset);
  realizarPeticionDeLecturaAMemoria(direccionVirtual);
  valorDeRetorno = recibirLecturaDeMemoria();
  log_info(loggerProgramas, "El proceso %d recibio la peticion de lectura en memoria con los datos %d...", pcbEnProceso->pid, valorDeRetorno);
  free(direccionVirtual);
  return valorDeRetorno;
}

//---------------ASIGNAR----------------//

void asignar(t_puntero direccion_variable, t_valor_variable valor){
  direccion *direccionVirtual = obtenerDireccionDePuntero(direccion_variable);
  log_info(loggerProgramas, "El proceso %d pidio que se asigne el valor %d a la variable que se encuentra en el puntero %d...", pcbEnProceso->pid, valor, direccion_variable);
  realizarPeticionDeEscrituraEnMemoria(direccionVirtual, valor);
  if(recvDeNotificacion(socketMemoria) != OPERACION_CON_MEMORIA_EXITOSA){
	  programaEnEjecucionAbortado = true;
  }
  free(direccionVirtual);
}


//----------------OBTENER VALOR COMPARTIDA------------//

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
  char* nombreDeLaVariable = string_new();//malloc(string_length(variable)+1);
  log_info(loggerProgramas, "El proceso %d pidio el valor de la variable compartida %s...", pcbEnProceso->pid, variable);
  string_append(&nombreDeLaVariable,"\0");
  free(nombreDeLaVariable);
  return consultarVariableCompartida(nombreDeLaVariable);
}

//-------------------------ASIGNAR VALOR COMPARTIDA----------------//

t_valor_variable asignarValorCompartida(t_nombre_compartida variable, t_valor_variable valor){
  char* nombreDeVariableAModificar = string_new();
  string_append(&nombreDeVariableAModificar, variable);
  string_append(&nombreDeVariableAModificar,"\0");
  enviarModificacionDeValor(nombreDeVariableAModificar, valor);
  free(nombreDeVariableAModificar);
  return recibirValorCompartida();
}


//----------------------DEFINIR VARIABLE----------------//-------------------SE PUEDE MEJORAR (MUCHO)



t_puntero definirVariable(t_nombre_variable identificador_variable){
    log_info(loggerProgramas, "Se recibio una peticion para definir la variable %c...", identificador_variable);
    if((punteroAPosicionEnStack+4)%tamPaginasMemoria>tamPaginasMemoria && cantidadDePaginasDeStack(punteroAPosicionEnStack)>datosParaEjecucion->tamanioStack){
        log_info(loggerProgramas, "No se puede declarar la variable, el proceso a generado un stackoverflow...");
        programaEnEjecucionAbortado = true;
        return -1;
    }else{
        variable* variableAAgregar = malloc(sizeof(variable));
        variableAAgregar->nombreVariable = identificador_variable;
        if((identificador_variable>='0')&&(identificador_variable<='9')){
            log_info(loggerProgramas, "La variable %c es un argumento de funcion...", identificador_variable);
            variableAAgregar->direccionDeVariable = obtenerDireccionDePuntero(punteroAPosicionEnStack);
            agregarArgumentoAStack(variableAAgregar);
            log_info(loggerProgramas, "Se agrego el argumento %c al stack...", identificador_variable);
        }else{
            log_info(loggerProgramas, "La variable %c es una variable...", identificador_variable);
            variableAAgregar->direccionDeVariable = obtenerDireccionDePuntero(punteroAPosicionEnStack);
            agregarVariableAStack(variableAAgregar);
            log_info(loggerProgramas, "Se agrego la variable %c al stack...", identificador_variable);
        }
        punteroAPosicionEnStack +=4;
        return convertirDeDireccionAPuntero(variableAAgregar->direccionDeVariable);
    }
}



//---------------------OBTENER POSICION DE VARIABLE--------------------//
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	bool esVariableBuscada(variable* variableBuscada){
		return identificador_variable == variableBuscada->nombreVariable;
	}
  int posicionDeRetorno;
  direccion *direccionAConvertir;
  stack *ultimoContexto;
  ultimoContexto = list_get(pcbEnProceso->indiceStack, pcbEnProceso->posicionStackActual);
  if(identificador_variable>='0' && identificador_variable<='9'){ //ME PUEDE LLEGAR UNA VARIABLE QUE NO EXISTE?
    direccionAConvertir = obtenerDireccionDeVariable(list_get(ultimoContexto->args, atoi(&identificador_variable)-1));
    posicionDeRetorno = convertirDeDireccionAPuntero(direccionAConvertir);
    free(direccionAConvertir);
    return posicionDeRetorno;
  }else{
	  if(list_any_satisfy(ultimoContexto->vars, (void*)esVariableBuscada)){
		  direccionAConvertir = obtenerDireccionDeVariable((list_find(ultimoContexto->vars, (void*)esVariableBuscada)));
		  posicionDeRetorno = convertirDeDireccionAPuntero(direccionAConvertir);
		  free(direccionAConvertir);
		  return posicionDeRetorno;
    /*int cantMaximaDeVars = obtenerCantidadDeArgs(ultimoContexto);
    for(; cantMaximaDeVars>=0; cantMaximaDeVars--){
      if(true){ //identificador_variable == obtenerIDVariable()
        direccionAConvertir = obtenerDireccionDeVariable(list_get(ultimoContexto->vars, list_size(ultimoContexto->vars)-1));
        posicionDeRetorno = convertirDeDireccionAPuntero(direccionAConvertir);
        return posicionDeRetorno;
      }*/
    }
  }
  programaEnEjecucionAbortado = true;
  return -1;
}


//---------------------IR A LABEL-------------------//
void irAlLabel(t_nombre_etiqueta nombreDeLaEtiqueta){
  t_puntero_instruccion instruccionParaPCB;
  log_info(loggerCPU, "Se pasa a modificar instruccion de proceso %d", pcbEnProceso->pid);
  instruccionParaPCB = metadata_buscar_etiqueta(nombreDeLaEtiqueta, pcbEnProceso->indiceEtiquetas, pcbEnProceso->tamanioEtiquetas); //La funcion devuelve la instruccion perteneciente a la etiqueta
  pcbEnProceso->programCounter = instruccionParaPCB; //Puede que lleve un -1
  log_info(loggerCPU,"Instruccion de program counter modificada.");
}

//---------------------LLAMAR SIN RETORNO----------//
void llamarSinRetorno(t_nombre_etiqueta etiqueta){
  stack *contextoAAgregar;
  contextoAAgregar = malloc(sizeof(stack));
  generarContexto(contextoAAgregar);
  list_add(pcbEnProceso->indiceStack, contextoAAgregar);
  irAlLabel(etiqueta);
}

//--------------------LLAMAR CON RETORNO-------------//
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
  stack *contextoAAgregar;
  direccion* direccionParaNuevoContexto;
  contextoAAgregar = malloc(sizeof(stack));
  generarContexto(contextoAAgregar);
  direccionParaNuevoContexto = obtenerDireccionDePuntero(donde_retornar);
  contextoAAgregar->retVar = *direccionParaNuevoContexto;
  list_add(pcbEnProceso->indiceStack, contextoAAgregar);
  irAlLabel(etiqueta);
}





//--------------------FINALIZAR-------------//
void finalizar(){
	stack *contextoAFinalizar;
	//contextoAFinalizar = malloc(sizeof(t_contexto)); //Es necesario hacer el malloc aca
	contextoAFinalizar = list_get(pcbEnProceso->indiceStack, pcbEnProceso->posicionStackActual);
	if(list_size(pcbEnProceso->indiceStack)-1 == 0){
		//finalizarProceso(pcbEnProceso);
		programaEnEjecucionFinalizado = true;
	}else{
		list_clean_and_destroy_elements(contextoAFinalizar->args, liberarElemento);
		list_clean_and_destroy_elements(contextoAFinalizar->vars, liberarElemento);
		pcbEnProceso->programCounter = contextoAFinalizar->retPos;
	}
	free(contextoAFinalizar);
}

//---------------------RETORNAR------------------//
void retornar(t_valor_variable retorno){
	stack *contextoAFinalizar;
	//contextoAFinalizar = malloc(sizeof(stack));
	contextoAFinalizar = list_get(pcbEnProceso->indiceStack, pcbEnProceso->posicionStackActual);
	pcbEnProceso->programCounter = contextoAFinalizar->retPos;
	t_puntero punteroARetVar = convertirDeDireccionAPuntero(&contextoAFinalizar->retVar);
	asignar(punteroARetVar, retorno);
	list_clean_and_destroy_elements(contextoAFinalizar->vars, liberarElemento);
	list_clean_and_destroy_elements(contextoAFinalizar->args, liberarElemento);
	free(contextoAFinalizar);
}

//----------------------PRIMITIVAS DE KERNEL-----------------------//

//--------------------WAIT------------//
void pedirWait(t_nombre_semaforo identificador_semaforo){
	log_info(loggerProgramas, "El proceso %d pidio wait al semaforo %s...", pcbEnProceso->pid, identificador_semaforo);
	int tamanio = string_length(identificador_semaforo)+sizeof(int); // Tengo que agregar el barra cero?
	void *paqueteDeWait = malloc(tamanio);
	memcpy(paqueteDeWait, &pcbEnProceso->pid, sizeof(int));
	memcpy(paqueteDeWait+sizeof(int), identificador_semaforo, string_length(identificador_semaforo));
	sendRemasterizado(socketKernel, PIDO_SEMAFORO, tamanio, paqueteDeWait);
	paquete *paqueteParaVerSiMeBloqueo;
	paqueteParaVerSiMeBloqueo = recvRemasterizado(socketKernel);
	if(paqueteParaVerSiMeBloqueo->tipoMsj == BLOQUEO_POR_SEMAFORO){
		programaEnEjecucionBloqueado = true;
		log_info(loggerProgramas, "El proceso %d quedo bloqueado por la peticion de semaforo...", pcbEnProceso->pid);
	}
}
//---------------SIGNAL------------//

void pedirSignal(t_nombre_semaforo identificador_semaforo){
	log_info(loggerProgramas, "El proceso %d dio signal del semaforo %c...", pcbEnProceso->pid, identificador_semaforo);
	int tamanio = string_length(identificador_semaforo)+sizeof(int); // Tengo que agregar el barra cero?
	void *mensajeDeSignal = malloc(tamanio);
	memcpy(mensajeDeSignal, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeSignal+sizeof(int), identificador_semaforo, string_length(identificador_semaforo));
	sendRemasterizado(socketKernel, DOY_SEMAFORO, tamanio, mensajeDeSignal);
	free(mensajeDeSignal);
}

//---------------RESERVAR--------------//

t_puntero reservarMemoria(t_valor_variable espacio){
	log_info(loggerProgramas, "El proceso %d pidio memoria por un espacio de %d...", pcbEnProceso->pid, espacio);
	void *mensajeDeAlocamiento = malloc(sizeof(int)+sizeof(t_valor_variable));
	memcpy(mensajeDeAlocamiento, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeAlocamiento+sizeof(int), &espacio, sizeof(t_valor_variable));
	sendRemasterizado(socketKernel, PIDO_RESERVA, sizeof(int)+sizeof(t_valor_variable), mensajeDeAlocamiento);
	free(mensajeDeAlocamiento);
	paquete *paqueteConT_Puntero;
	paqueteConT_Puntero = recvRemasterizado(socketKernel);
	if(paqueteConT_Puntero->tipoMsj != OPERACION_CON_KERNEL_EXITOSA){
		log_info(loggerProgramas, "No se pudo alocar memoria dinamica para el proceso %d...", pcbEnProceso->pid);
		log_info(loggerProgramas, "Se prosigue a abortar el proceso %d...", pcbEnProceso->pid);
		programaEnEjecucionAbortado = true;
	}
	log_info(loggerProgramas, "Se recibio puntero al espacio reservado %d...", *(t_puntero*)paqueteConT_Puntero->mensaje);
	t_puntero punteroARetornar = *(t_puntero*)paqueteConT_Puntero->mensaje;
	free(paqueteConT_Puntero);
	return punteroARetornar;
}

//----------------LIBERAR------------//
void liberarMemoria(t_puntero puntero){
	log_info(loggerProgramas, "El proceso %d pidio liberacion de memoria en el puntero %d...", pcbEnProceso->pid, puntero);
	void *mensajeDeLiberacion = malloc(sizeof(int)+sizeof(t_puntero));
	memcpy(mensajeDeLiberacion, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeLiberacion+sizeof(int), &puntero, sizeof(t_puntero));
	sendRemasterizado(socketKernel, PIDO_LIBERACION, sizeof(t_puntero)+sizeof(int), mensajeDeLiberacion);
	free(mensajeDeLiberacion);
}

//----------------ABRIR------------//

t_descriptor_archivo abrimeArchivo(t_direccion_archivo direccion, t_banderas flags){
	log_info(loggerProgramas, "El proceso %d pidio que se abra el archivo con la ruta %s...", pcbEnProceso->pid, direccion);
	int tamanioMensaje = string_length(direccion) + sizeof(t_banderas) + sizeof(int);
	void* mensajeAEnviar = malloc(tamanioMensaje);
	memcpy(mensajeAEnviar, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeAEnviar+sizeof(int),direccion,string_length(direccion));
	memcpy(mensajeAEnviar + string_length(direccion) + sizeof(int), &flags, sizeof(t_banderas));
	sendRemasterizado(socketKernel, ABRIR_ARCHIVO, tamanioMensaje, mensajeAEnviar);
	free(mensajeAEnviar);
	paquete *paqueteConDescriptorDeArchivo;
	paqueteConDescriptorDeArchivo = recvRemasterizado(socketKernel); // No se si es necesario chequear lo que se recibe
	if(paqueteConDescriptorDeArchivo->tipoMsj == NO_SE_PUDO_ABRIR){
		log_info(loggerProgramas, "No se pudo abrir el archivo %s...", direccion);
		log_info(loggerProgramas, "El proceso %d es abortado...", pcbEnProceso->pid);
		programaEnEjecucionAbortado = true;
	}
	t_descriptor_archivo descriptorARetornar = *(t_descriptor_archivo*)paqueteConDescriptorDeArchivo->mensaje;
	free(paqueteConDescriptorDeArchivo);
	return descriptorARetornar;
}

//-------------BORRAR-----------//

void borrameArchivo(t_descriptor_archivo descriptor_archivo){
	log_info(loggerProgramas, "El proceso %d pidio que se borre el archivo %d...", pcbEnProceso->pid, descriptor_archivo);
	void *mensajeDeBorrado = malloc(sizeof(t_descriptor_archivo)+sizeof(int));
	memcpy(mensajeDeBorrado, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeBorrado + sizeof(int), &descriptor_archivo, sizeof(t_descriptor_archivo));
	sendRemasterizado(socketKernel, BORRAR_ARCHIVO, sizeof(t_descriptor_archivo) + sizeof(int), mensajeDeBorrado);
	free(mensajeDeBorrado);
	int notificacion = recvDeNotificacion(socketKernel);
	if(notificacion != OPERACION_CON_ARCHIVO_EXITOSA){
		log_info(loggerProgramas, "El proceso %d no pudo borrar el archivo %d...", pcbEnProceso->pid, descriptor_archivo);
		log_info(loggerProgramas, "Se prosigue a abortar el proceso %d...", pcbEnProceso->pid);
		programaEnEjecucionAbortado = true;
	}
}

//------------CERRAR---------//

void cerrameArchivo(t_descriptor_archivo descriptor_archivo){
	log_info(loggerProgramas, "El proceso %d pidio que se cierre el archivo %d...", pcbEnProceso->pid, descriptor_archivo);
	void *mensajeDeCerrado = malloc(sizeof(int)+sizeof(t_descriptor_archivo));
	memcpy(mensajeDeCerrado, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeCerrado + sizeof(int), &descriptor_archivo, sizeof(t_descriptor_archivo));
	sendRemasterizado(socketKernel, CERRAR_ARCHIVO, sizeof(t_descriptor_archivo) + sizeof(int), mensajeDeCerrado);
	free(mensajeDeCerrado);
	int notificacion = recvDeNotificacion(socketKernel);
	if(notificacion != OPERACION_CON_ARCHIVO_EXITOSA){
		log_info(loggerProgramas, "El proceso %d no pudo cerrar el archivo %d...", pcbEnProceso->pid, descriptor_archivo);
		log_info(loggerProgramas, "Se prosigue a abortar el proceso %d...", pcbEnProceso->pid);
		programaEnEjecucionAbortado = true;
	}
}

//------------MOVER CURSOR---------//

void movemeElCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	log_info(loggerProgramas, "El proceso %d pidio que se mueva el cursor dentro del archivo %d a la posicion %d...", pcbEnProceso->pid, descriptor_archivo, posicion);
	int tamanioMensaje = sizeof(descriptor_archivo) + sizeof(t_valor_variable) + sizeof(int);
	void* mensajeDeCursor = malloc(tamanioMensaje);
	memcpy(mensajeDeCursor, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeCursor+sizeof(int), &descriptor_archivo, sizeof(descriptor_archivo));
	memcpy(mensajeDeCursor+sizeof(descriptor_archivo)+sizeof(int), &posicion, sizeof(t_valor_variable));
	sendRemasterizado(socketKernel, MOVEME_CURSOR, tamanioMensaje, mensajeDeCursor);
	int notificacion = recvDeNotificacion(socketKernel);
	if(notificacion != OPERACION_CON_ARCHIVO_EXITOSA){
		log_info(loggerProgramas, "El proceso %d no pudo mover el cursor dentro del archivo %d...", pcbEnProceso->pid, descriptor_archivo);
		log_info(loggerProgramas, "Se prosigue a abortar el proceso %d...", pcbEnProceso->pid);
		programaEnEjecucionAbortado = true;
	}
	free(mensajeDeCursor);
}

//---------------ESCRBIR---------//

void escribimeArchivo(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	log_info(loggerProgramas, "El proceso %d pidio que se escriba en el archivo %d, el contenido %p de tamanio %d", pcbEnProceso->pid, descriptor_archivo, informacion, tamanio);
	int tamanioMensaje = sizeof(t_descriptor_archivo)+tamanio+sizeof(t_valor_variable) + sizeof(int);
	void* mensajeDeEscritura = malloc(tamanioMensaje);
	memcpy(mensajeDeEscritura, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeEscritura+sizeof(int), &descriptor_archivo, sizeof(t_descriptor_archivo));
	memcpy(mensajeDeEscritura+sizeof(t_descriptor_archivo)+sizeof(int), &tamanio, sizeof(t_valor_variable));
	memcpy(mensajeDeEscritura+sizeof(t_descriptor_archivo)+sizeof(t_valor_variable)+sizeof(int), informacion, tamanio);
	sendRemasterizado(socketKernel, ESCRIBIR_ARCHIVO, tamanioMensaje, mensajeDeEscritura);
	if(recvDeNotificacion(socketKernel) != OPERACION_CON_ARCHIVO_EXITOSA){
		log_info(loggerProgramas, "El proceso %d no pudo escribir en el archivo %d...", pcbEnProceso->pid, descriptor_archivo);
		log_info(loggerProgramas, "Se prosigue a abortar el proceso %d...", pcbEnProceso->pid);
		programaEnEjecucionAbortado = true;
	}
	free(mensajeDeEscritura);
}

//-----------LEER----------//

void leemeDeArchivo(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){
	int tamanioMensaje = sizeof(t_descriptor_archivo)+sizeof(t_puntero)+sizeof(t_valor_variable)+sizeof(int); //Chequear si es necesario sumar el barra cero
	void *mensajeDeLectura = malloc(tamanioMensaje);
	memcpy(mensajeDeLectura, &pcbEnProceso->pid, sizeof(int));
	memcpy(mensajeDeLectura+sizeof(int), &descriptor_archivo, sizeof(t_descriptor_archivo));
	memcpy(mensajeDeLectura+sizeof(t_descriptor_archivo)+sizeof(int), &informacion, tamanio);
	memcpy(mensajeDeLectura+sizeof(t_descriptor_archivo)+tamanio+sizeof(int), &tamanio, sizeof(t_valor_variable));
	sendRemasterizado(socketKernel, LEER_DE_ARCHIVO, tamanioMensaje, mensajeDeLectura);
	free(mensajeDeLectura);
	paquete *paqueteConDatosDeLectura = recvRemasterizado(socketKernel);
	mandarAGuardarDatosDeArchivo(paqueteConDatosDeLectura, informacion);
}
