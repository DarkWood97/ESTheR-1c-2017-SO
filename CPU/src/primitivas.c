#include "primitivas.h"
#include "ansiSop.h"
#include "funcionesCpu.h"
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include <parser/sintax.h>

//---------------------DEREFERENCIAR----------------//
t_valor_variable dereferenciar(t_puntero direccion_variable){
  t_valor_variable valorDeRetorno;
  direccion *direccionVirtual = obtenerDireccionDePuntero(direccion_variable);
  realizarPeticionDeLecturaAMemoria(direccionVirtual, valorDeRetorno);
  valorDeRetorno = recibirLecturaDeMemoria();
  return valorDeRetorno;
}

//---------------ASIGNAR----------------//

void asignar(t_puntero direccion_variable, t_valor_variable valor){
  direccion *direccionVirtual = obtenerDireccionDePuntero(direccion_variable);
  realizarPeticionDeEscrituraEnMemoria(direccionVirtual, valor);
}


//----------------OBTENER VALOR COMPARTIDA------------//

t_valor_variable obtenerValorCompartida(t_nombre_compartida variable){
  char* nombreDeLaVariable = malloc(string_length(variable)+1);
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
  int cantidadDeContextos = list_size(pcbEnProceso->contextoActual);
  direccion *direccionDeVariable;
  direccionDeVariable = malloc(sizeof(direccion));
  if((identificador_variable>='0')&&(identificador_variable<='9')){
    direccionDeVariable = generarDireccionParaArgumento(cantidadDeContextos);
    agregarVariableAArgs(direccionDeVariable, identificador_variable);
  }else{
    direccionDeVariable = generarDireccionParaVariable(cantidadDeContextos);
    agregarVariableAVars(direccionDeVariable, identificador_variable);
  }
  return convertirDeDireccionAPuntero(direccionDeVariable);
}



//---------------------OBTENER POSICION DE VARIABLE--------------------//
t_puntero obtenerPosicionVariable(t_nombre_variable identificador_variable){
	bool esVariableBuscada(t_nombre_variable identificadorDevariableChequeado){
		return identificador_variable == identificadorDevariableChequeado;
	}
  int posicionDeRetorno;
  direccion *direccionAConvertir;
  t_contexto *ultimoContexto;
  ultimoContexto = list_get(pcbEnProceso->contextoActual, list_size(pcbEnProceso->contextoActual)-1); //La lista empieza de 0?
  if(identificador_variable>='0' && identificador_variable<='9'){ //ME PUEDE LLEGAR UNA VARIABLE QUE NO EXISTE
    direccionAConvertir = obtenerDireccionDeVariable(list_get(ultimoContexto->args, atoi(&identificador_variable)-1));
    posicionDeRetorno = convertirDeDireccionAPuntero(direccionAConvertir);
    return posicionDeRetorno;
  }else{
	  if(list_any_satisfy(ultimoContexto->vars, (void*)esVariableBuscada)){
		  direccionAConvertir = obtenerDireccionDeVariable((list_find(ultimoContexto->vars, (void*)esVariableBuscada)));
		  posicionDeRetorno = convertirDeDireccionAPuntero(direccionAConvertir);
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
  log_info(loggerCPU, "Se pasa a modificar instruccion de proceso %d", pcbEnProceso->PID);
  instruccionParaPCB = metadata_buscar_etiqueta(nombreDeLaEtiqueta, pcbEnProceso->etiquetas, pcbEnProceso->tamEtiquetas); //La funcion devuelve la instruccion perteneciente a la etiqueta
  pcbEnProceso->ProgramCounter = instruccionParaPCB; //Puede que lleve un -1
  log_info(loggerCPU,"Instruccion de program counter modificada.");
}

//---------------------LLAMAR SIN RETORNO----------//
void llamarSinRetorno(t_nombre_etiqueta etiqueta){
  t_contexto *contextoAAgregar;
  contextoAAgregar = malloc(sizeof(t_contexto));
  generarContexto(contextoAAgregar);
  list_add(pcbEnProceso->contextoActual, contextoAAgregar);
  irAlLabel(etiqueta);
}

//--------------------LLAMAR CON RETORNO-------------//
void llamarConRetorno(t_nombre_etiqueta etiqueta, t_puntero donde_retornar){
  t_contexto *contextoAAgregar;
  direccion* direccionParaNuevoContexto;
  contextoAAgregar = malloc(sizeof(t_contexto));
  generarContexto(contextoAAgregar);
  direccionParaNuevoContexto = obtenerDireccionDePuntero(donde_retornar);
  contextoAAgregar->retVar = *direccionParaNuevoContexto;
  list_add(pcbEnProceso->contextoActual, contextoAAgregar);
  irAlLabel(etiqueta);
}





//--------------------FINALIZAR-------------//
void finalizar(){
	t_contexto *contextoAFinalizar;
	contextoAFinalizar = malloc(sizeof(t_contexto)); //Es necesario hacer el malloc aca
	contextoAFinalizar = list_get(pcbEnProceso->contextoActual, list_size(pcbEnProceso->contextoActual)-1);
	if(list_size(pcbEnProceso->contextoActual)-1 == 0){
		finalizarProceso(pcbEnProceso);
	}else{
		list_clean_and_destroy_elements(contextoAFinalizar->args, liberarElemento);
		list_clean_and_destroy_elements(contextoAFinalizar->vars, liberarElemento);
		pcbEnProceso->ProgramCounter = contextoAFinalizar->retPos;
	}
	free(contextoAFinalizar);
}

//---------------------RETORNAR------------------//
void retornar(t_valor_variable retorno){
	t_contexto *contextoAFinalizar;
	contextoAFinalizar = malloc(sizeof(t_contexto));
	contextoAFinalizar = list_get(pcbEnProceso->contextoActual, list_size(pcbEnProceso->contextoActual)-1);
	pcbEnProceso->ProgramCounter = contextoAFinalizar->retPos;
	t_puntero punteroARetVar = convertirDeDireccionAPuntero(&contextoAFinalizar->retVar);
	asignar(punteroARetVar, retorno);
	list_clean_and_destroy_elements(contextoAFinalizar->vars, liberarElemento);
	list_clean_and_destroy_elements(contextoAFinalizar->args, liberarElemento);
	free(contextoAFinalizar);
}

//----------------------PRIMITIVAS DE KERNEL-----------------------//

//--------------------WAIT------------//
void pedirWait(t_nombre_semaforo identificador_semaforo){
	int tamanio = string_length(identificador_semaforo); // Tengo que agregar el barra cero?
	sendRemasterizado(socketKernel, PIDO_SEMAFORO, tamanio, identificador_semaforo);
	paquete *paqueteParaVerSiMeBloqueo;
	paqueteParaVerSiMeBloqueo = recvRemasterizado(socketKernel);
	if(paqueteParaVerSiMeBloqueo->tipoMsj == 1){
		programaEnEjecucionBloqueado = true;
	}
}
//---------------SIGNAL------------//

void pedirSignal(t_nombre_semaforo identificador_semaforo){
	int tamanio = string_length(identificador_semaforo); // Tengo que agregar el barra cero?
	sendRemasterizado(socketKernel, DOY_SEMAFORO, tamanio, identificador_semaforo);
}

//---------------RESERVAR--------------//

t_puntero reservarMemoria(t_valor_variable espacio){
	sendRemasterizado(socketKernel, PIDO_RESERVA, sizeof(int), &espacio);
	paquete *paqueteConT_Puntero;
	paqueteConT_Puntero = recvRemasterizado(socketKernel);
	return (t_puntero)paqueteConT_Puntero->mensaje;
}

//----------------LIBERAR------------//
void liberarMemoria(t_puntero puntero){ // ARREGLAR
	sendRemasterizado(socketKernel, PIDO_LIBERACION, sizeof(t_puntero), &puntero);
}

//----------------ABRIR------------//

t_descriptor_archivo abrimeArchivo(t_direccion_archivo direccion, t_banderas flags){
	int tamanioMensaje = string_length(direccion) + sizeof(t_banderas);
	void* mensajeAEnviar = malloc(tamanioMensaje);
	memcpy(mensajeAEnviar,direccion,string_length(direccion));
	memcpy(mensajeAEnviar + string_length(direccion), &flags, sizeof(t_banderas));
	sendRemasterizado(socketKernel, ABRIR_ARCHIVO, tamanioMensaje, mensajeAEnviar);
	free(mensajeAEnviar);
	paquete *paqueteConDescriptorDeArchivo;
	paqueteConDescriptorDeArchivo = recvRemasterizado(socketKernel); // No se si es necesario chequear lo que se recibe
	if(paqueteConDescriptorDeArchivo->tipoMsj == NO_SE_PUDO_ABRIR){
		programaEnEjecucionAbortado = true;
	}
	return (t_descriptor_archivo)paqueteConDescriptorDeArchivo->mensaje;
}

//-------------BORRAR-----------//

void borrameArchivo(t_descriptor_archivo descriptor_archivo){
	sendRemasterizado(socketKernel, BORRAR_ARCHIVO, sizeof(t_descriptor_archivo), &descriptor_archivo);
}

//------------CERRAR---------//

void cerrameArchivo(t_descriptor_archivo descriptor_archivo){
	sendRemasterizado(socketKernel, CERRAR_ARCHIVO, sizeof(t_descriptor_archivo), &descriptor_archivo);
}

//------------MOVER CURSOR---------//

void movemeElCursor(t_descriptor_archivo descriptor_archivo, t_valor_variable posicion){
	int tamanioMensaje = sizeof(descriptor_archivo) + sizeof(t_valor_variable);
	void* mensajeDeCursor = malloc(tamanioMensaje);
	memcpy(mensajeDeCursor, &descriptor_archivo, sizeof(descriptor_archivo));
	memcpy(mensajeDeCursor+sizeof(descriptor_archivo), &posicion, sizeof(t_valor_variable));
	sendRemasterizado(socketKernel, MOVEME_CURSOR, tamanioMensaje, mensajeDeCursor);
	free(mensajeDeCursor);
}

//---------------ESCRBIR---------//

void escribimeArchivo(t_descriptor_archivo descriptor_archivo, void* informacion, t_valor_variable tamanio){
	int tamanioMensaje = sizeof(t_descriptor_archivo)+tamanio+sizeof(t_valor_variable);
	void* mensajeDeEscritura = malloc(tamanioMensaje);
	memcpy(mensajeDeEscritura, &descriptor_archivo, sizeof(t_descriptor_archivo));
	memcpy(mensajeDeEscritura+sizeof(t_descriptor_archivo), informacion, tamanio);
	memcpy(mensajeDeEscritura+sizeof(t_descriptor_archivo)+tamanio, &tamanio, sizeof(t_valor_variable));
	sendRemasterizado(socketKernel, ESCRIBIR_ARCHIVO, tamanioMensaje, mensajeDeEscritura);
	free(mensajeDeEscritura);
}

//-----------LEER----------//

void leemeDeArchivo(t_descriptor_archivo descriptor_archivo, t_puntero informacion, t_valor_variable tamanio){
	int tamanioMensaje = sizeof(t_descriptor_archivo)+sizeof(t_puntero)+sizeof(t_valor_variable); //Chequear si es necesario sumar el barra cero
	void *mensajeDeLectura = malloc(tamanioMensaje);
	memcpy(mensajeDeLectura, &descriptor_archivo, sizeof(t_descriptor_archivo));
	memcpy(mensajeDeLectura+sizeof(t_descriptor_archivo), &informacion, tamanio);
	memcpy(mensajeDeLectura+sizeof(t_descriptor_archivo)+tamanio, &tamanio, sizeof(t_valor_variable));
	sendRemasterizado(socketKernel, LEER_DE_ARCHIVO, tamanioMensaje, mensajeDeLectura);
	free(mensajeDeLectura);
}
