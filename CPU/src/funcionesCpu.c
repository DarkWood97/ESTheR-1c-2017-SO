#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include "funcionesCpu.h"

t_config *generarT_ConfigParaCargar(char *path) {
	t_config* configuracionDelComponente;
	if((configuracionDelComponente = config_create(path)) == NULL){
		perror("Error de ruta de archivo de configuracion");
		exit(-1);
	}
	return configuracionDelComponente;
}

void obtenerUltimoPunteroStack(){
    //OBTENGO LA POSICION A PARTIR DE DONDE TENGO QUE EMPEZAR A GUARDAR MIS VARIABLES Y ARGUMENTOS EN EL STACK.
    punteroAPosicionEnStack = pcbEnProceso->cantidadPaginasCodigo * tamPaginasMemoria;
    int posicionDeStack;
    for(posicionDeStack = 0; posicionDeStack<list_size(pcbEnProceso->indiceStack); posicionDeStack++){
        stack* stackChequeado = list_get(pcbEnProceso->indiceStack, posicionDeStack);
        punteroAPosicionEnStack += list_size(stackChequeado->args)*4;
        punteroAPosicionEnStack += list_size(stackChequeado->vars)*4;
    }
}

void crearCPU(t_config *configuracionCPU){
	IP_KERNEL = string_new();
	IP_MEMORIA = string_new();
	PUERTO_KERNEL = config_get_int_value(configuracionCPU, "PUERTO_KERNEL");
	PUERTO_MEMORIA = config_get_int_value(configuracionCPU, "PUERTO_MEMORIA");
	string_append(&IP_KERNEL, config_get_string_value(configuracionCPU, "IP_KERNEL"));
	string_append(&IP_MEMORIA, config_get_string_value(configuracionCPU, "IP_MEMORIA"));
}

void inicializarCPU(char* path){
	t_config *configuracionCPU = generarT_ConfigParaCargar(path);
	crearCPU(configuracionCPU);
	config_destroy(configuracionCPU);
}

/*void recibirMensajeDeKernel(int socketKernel){
	char *buff = malloc(sizeof(char)*16);
//	int tamanioBuff = strlen(buff)+1;
	if(recv(socketKernel,buff,16,0) == -1){
		perror("Error de receive");
		exit(-1);
	}
	printf("%s\n",buff);
	free(buff);
}*/
//PCB pcb;
void verificarParametrosInicio(int argc)
{
	if(argc!=2){
			perror("Faltan parametros");
			exit(-1);
		}
}
void verificarParametrosCrear(t_config* configuracion, int sizeStruct)
{
	if (dictionary_size(configuracion->properties) != sizeStruct) {
			perror("Faltan parametros para inicializar el fileSystem");
			exit(-1);
		}
}
//paquete serializar(void* bufferDeData, int tipoDeMensaje)
//{
//  paquete paqueteAEnviar;
//  int longitud= string_length(bufferDeData);
//  paqueteAEnviar.mensaje=malloc(sizeof(char)*16);
//  paqueteAEnviar.tamMsj = longitud;
//  paqueteAEnviar.tipoMsj=tipoDeMensaje;
//  paqueteAEnviar.mensaje = bufferDeData;
//  return paqueteAEnviar;
//  free(paqueteAEnviar.mensaje);
//}
//
//paquete serializarCod(codeIndex* cod, int tipoDeMensaje)
//{
//  paquete paqueteAEnviar;
//  int longitud= sizeof(codeIndex);
//  paqueteAEnviar.mensaje=malloc(longitud);
//  paqueteAEnviar.tamMsj = longitud;
//  paqueteAEnviar.tipoMsj= tipoDeMensaje;
//  paqueteAEnviar.mensaje = cod;
//  return paqueteAEnviar;
//}
//
//void enviar(int socket, paquete mensaje) { //Socket que envia mensaje
//
//	int longitud = sizeof(mensaje); //sino no lee \0
//		if (send(socket, &mensaje, longitud, 0) == -1) {
//			perror("Error de send");
//			close(socket);
//			exit(-1);
//	}
//
//}
//void realizarHandshake(int socket, int handshake)
//{ //Socket que envia mensaje
//	paquete paqueteAEnviar,auxiliar;
//	paqueteAEnviar.mensaje=malloc(sizeof(char)*16);
//	auxiliar=serializar(NULL,handshake);
//	memcpy(&paqueteAEnviar, &auxiliar, sizeof(auxiliar)); /*no se si es sizeof(paquete)*/
//	enviar(socket,paqueteAEnviar);
//	free(paqueteAEnviar.mensaje);
//}
//void destruirPaquete(paquete * paquete)
//{
//	free(paquete->mensaje);
//	free(paquete);
//}
//
////void* serializarPCB(PCB  pcb){
////	void* mensaje = malloc(sizeof(pcb));
////	memcpy(mensaje, &pcb.PID, sizeof(int));
////	memcpy(mensaje+sizeof(int), &pcb.ProgramCounter, sizeof(int));
////	memcpy(mensaje+(sizeof(int)*2), &pcb.paginas_Codigo, sizeof(int));
//////	memcpy(mensaje+(sizeof(int)*3), &pcb.cod.comienzo, sizeof(int));
//////	memcpy(mensaje+(sizeof(int)*4), &pcb.cod.offset, sizeof(int));
////	memcpy(mensaje+(sizeof(int)*5), pcb.etiquetas, sizeof(char)*16);
////	memcpy(mensaje+(sizeof(int)*5)+(sizeof(char)*16), &pcb.exitCode, sizeof(int));
////	memcpy(mensaje+(sizeof(int)*6)+(sizeof(char)*16), &pcb.contextoActual, sizeof(t_list*)*16);
////	memcpy(mensaje+(sizeof(int)*6)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tamContextoActual, sizeof(int));
////	memcpy(mensaje+(sizeof(int)*7)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tamEtiquetas, sizeof(int));
////	memcpy(mensaje+(sizeof(int)*8)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tablaKernel.paginas, sizeof(int));
////	memcpy(mensaje+(sizeof(int)*9)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tablaKernel.pid, sizeof(int));
////	memcpy(mensaje+(sizeof(int)*10)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tablaKernel.tamaniosPaginas, sizeof(int));
////	return mensaje;
////}
//int _obtenerSizeActual_(int b)
//	{
//		int auxiliar=sizeof(int)*b+sizeof(char)*16+sizeof(t_list)*16;
//		b++;
//		return auxiliar;
//	}
//
//
//void lineaParaElAnalizador(char* sentencia, char * aDevolver){
//
//		int i = string_length(sentencia);
//		while (string_ends_with(sentencia, "\n")) {
//			i--;
//			sentencia = string_substring_until(sentencia, i);
//		}
//		aDevolver=sentencia;
//}

//----------------------SERIALIZAR PCB
void *serializarT_Intructions(PCB *pcbConT_intruction){
  void *serializacion = malloc(sizeof(t_intructions)*pcbConT_intruction->cantidadTIntructions);
  int i, auxiliar1 = 0;
  for(i = 0; i<pcbConT_intruction->cantidadTIntructions; i++){
    memcpy(serializacion+sizeof(int)*auxiliar1, &pcbConT_intruction->indiceCodigo[i].start, sizeof(int));
    auxiliar1++;
    memcpy(serializacion+sizeof(int)*auxiliar1,&pcbConT_intruction->indiceCodigo[i].offset, sizeof(int));
    auxiliar1++;
  }
  return serializacion;
}

void* serializarVariable(t_list* variables){
    void* variableSerializada = malloc(list_size(variables)*sizeof(variable)+sizeof(int));
    int i;
    int cantidadDeVariables = list_size(variables);
    memcpy(variableSerializada, &cantidadDeVariables, sizeof(int));
    for(i = 0; i<cantidadDeVariables; i++){ //Antes i=1
        variable *variableObtenida = list_get(variables, i);
        memcpy(variableSerializada+sizeof(variable)*i, variableObtenida, sizeof(variable));
    }
   // free(variableSerializada);
    return variableSerializada;
}

int sacarTamanioDeLista(t_list* contexto){
    int i, tamanioDeContexto = 0;
    for(i = 0; i<list_size(contexto); i++){
        stack *contextoAObtener = list_get(contexto, i);
        tamanioDeContexto += sizeof(int)*4+sizeof(direccion)+(list_size(contextoAObtener->args)+list_size(contextoAObtener->vars))*(sizeof(char)+sizeof(direccion));
        //free(contextoAObtener);
    }
    return tamanioDeContexto;
}

void *serializarStack(PCB* pcbConContextos){
    void* contextoSerializado = malloc(sacarTamanioDeLista(pcbConContextos->indiceStack));
  int i;
  for(i = 0; i<pcbConContextos->tamanioContexto; i++){
    stack* contexto = list_get(pcbConContextos->indiceStack, i);
    memcpy(contextoSerializado, &contexto->pos, sizeof(int));
    void *argsSerializadas = serializarVariable(contexto->args);
    void *varsSerializadas = serializarVariable(contexto->vars);
    memcpy(contextoSerializado+sizeof(int), argsSerializadas, list_size(contexto->args)*sizeof(variable));
    memcpy(contextoSerializado+sizeof(int)+list_size(contexto->args)*sizeof(variable), varsSerializadas, list_size(contexto->vars)*sizeof(variable));
    memcpy(contextoSerializado+list_size(contexto->args)*sizeof(variable)+list_size(contexto->vars)*sizeof(variable), &contexto->retPos, sizeof(int));
    free(argsSerializadas);
    free(varsSerializadas);
  }
  return contextoSerializado;
}

int sacarTamanioPCB(PCB* pcbASerializar){
    int tamaniosDeContexto = sacarTamanioDeLista(pcbASerializar->indiceStack);
    int tamanio= sizeof(int)*8+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+tamaniosDeContexto+pcbASerializar->tamanioEtiquetas;
    return tamanio;
}

void* serializarPCB(PCB* pcbASerializar){
    int tamanioPCB = sacarTamanioPCB(pcbASerializar);
    void* pcbSerializada = malloc(tamanioPCB);
    memcpy(pcbSerializada, &pcbASerializar->pid, sizeof(int));
    memcpy(pcbSerializada+sizeof(int), &pcbASerializar->programCounter, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*2, &pcbASerializar->cantidadPaginasCodigo, sizeof(int));
    void* indiceDeCodigoSerializado = serializarT_Intructions(pcbASerializar);
    memcpy(pcbSerializada+sizeof(int)*3, &pcbASerializar->cantidadTIntructions, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*4, indiceDeCodigoSerializado, sizeof(t_intructions)*pcbASerializar->cantidadTIntructions);
    memcpy(pcbSerializada+sizeof(int)*4+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions, &pcbASerializar->tamanioEtiquetas, sizeof(int));
    if(pcbASerializar->tamanioEtiquetas!=0){
    	memcpy(pcbSerializada+ sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions, pcbASerializar->indiceEtiquetas, pcbASerializar->tamanioEtiquetas); //CHEQUEAR QUE ES TAMETIQUETAS
    }
    memcpy(pcbSerializada+sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->rafagas, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*6+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->posicionStackActual, sizeof(int));
    void* stackSerializado = serializarStack(pcbASerializar);
    memcpy(pcbSerializada+sizeof(int)*7+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->tamanioContexto, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*8+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, stackSerializado, sacarTamanioDeLista(pcbASerializar->indiceStack));
    free(indiceDeCodigoSerializado);
    free(stackSerializado);
    return pcbSerializada;
}

//---------------DESERIALIZAR PCB
void desserializarVariables(t_list *variables, paquete *paqueteConVariables, int* dondeEstoy){
    int cantidadDeVariables;
    memcpy(&cantidadDeVariables, paqueteConVariables->mensaje+*dondeEstoy, sizeof(int));
    *dondeEstoy += sizeof(int);
    int i;
    for(i = 0; i<cantidadDeVariables; i++){
        variable *variable = malloc(sizeof(variable));
        memcpy(&variable->nombreVariable, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(char));
        *dondeEstoy += sizeof(char);
        variable->direccionDeVariable = malloc(sizeof(direccion));
        memcpy(&variable->direccionDeVariable->numPagina, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        memcpy(&variable->direccionDeVariable->offset, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        memcpy(&variable->direccionDeVariable->tamanioPuntero, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        list_add(variables, variable);
    }
}

void desserializarContexto(PCB* pcbConContexto, paquete* paqueteConContexto, int dondeEstoy){
    int nivelDeContexto;
    for(nivelDeContexto = 0; nivelDeContexto<pcbConContexto->tamanioContexto; nivelDeContexto++){
        stack *contextoAAgregarAPCB = malloc(sizeof(stack));
        contextoAAgregarAPCB->args = list_create();
        contextoAAgregarAPCB->vars = list_create();
        memcpy(&contextoAAgregarAPCB->pos, paqueteConContexto->mensaje+dondeEstoy, sizeof(int));
        dondeEstoy +=sizeof(int);
        desserializarVariables(contextoAAgregarAPCB->args, paqueteConContexto, &dondeEstoy);
        desserializarVariables(contextoAAgregarAPCB->vars, paqueteConContexto, &dondeEstoy);
        memcpy(&contextoAAgregarAPCB->retPos, paqueteConContexto->mensaje + dondeEstoy, sizeof(int));
        memcpy(&contextoAAgregarAPCB->retVar, paqueteConContexto->mensaje + dondeEstoy + sizeof(int), sizeof(direccion));
        list_add(pcbConContexto->indiceStack, contextoAAgregarAPCB);
        dondeEstoy += sizeof(int);
    }
}

int desserializarTIntructions(PCB* pcbDesserializada, paquete *paqueteConPCB){
    int i, auxiliar = 0;
    memcpy(&pcbDesserializada->cantidadTIntructions, paqueteConPCB->mensaje+(sizeof(int)*3), sizeof(int));
    pcbDesserializada->indiceCodigo = malloc(pcbDesserializada->cantidadTIntructions*sizeof(t_intructions));
    for(i = 0; i<pcbDesserializada->cantidadTIntructions; i++){
        memcpy(&pcbDesserializada->indiceCodigo[i].start, paqueteConPCB->mensaje+sizeof(int)*(4+auxiliar), sizeof(int));
        auxiliar++;
        memcpy(&pcbDesserializada->indiceCodigo[i].offset, paqueteConPCB->mensaje+sizeof(int)*(4+auxiliar), sizeof(int));
        auxiliar++;
    }
    int retorno = sizeof(int)*(4+auxiliar);
    return retorno;
}

PCB* deserializarPCB(paquete* paqueteConPCB){
    PCB* pcbDesserializada = malloc(paqueteConPCB->tamMsj);
    memcpy(&pcbDesserializada->pid, paqueteConPCB->mensaje, sizeof(int));
    memcpy(&pcbDesserializada->programCounter, paqueteConPCB->mensaje+sizeof(int), sizeof(int));
    memcpy(&pcbDesserializada->cantidadPaginasCodigo, paqueteConPCB->mensaje+sizeof(int)*2, sizeof(int));
    int dondeEstoy = desserializarTIntructions(pcbDesserializada, paqueteConPCB);
    memcpy(&pcbDesserializada->tamanioEtiquetas, paqueteConPCB->mensaje+dondeEstoy, sizeof(int));
    pcbDesserializada->indiceEtiquetas = string_new();//etiquetas
    if(pcbDesserializada->tamanioEtiquetas!=0){
    	 memcpy(pcbDesserializada->indiceEtiquetas, paqueteConPCB->mensaje+sizeof(int)+dondeEstoy, pcbDesserializada->tamanioEtiquetas);
    }
    memcpy(&pcbDesserializada->rafagas,paqueteConPCB->mensaje+sizeof(int)+dondeEstoy+pcbDesserializada->tamanioEtiquetas,sizeof(int));
    memcpy(&pcbDesserializada->posicionStackActual, paqueteConPCB->mensaje+dondeEstoy+sizeof(int)*2+pcbDesserializada->tamanioEtiquetas, sizeof(int));
    memcpy(&pcbDesserializada->tamanioContexto, paqueteConPCB->mensaje+sizeof(int)*3+dondeEstoy+pcbDesserializada->tamanioEtiquetas, sizeof(int));

    dondeEstoy +=sizeof(int)*4+pcbDesserializada->tamanioEtiquetas;
    pcbDesserializada->indiceStack = list_create();
    desserializarContexto(pcbDesserializada, paqueteConPCB, dondeEstoy);
    return pcbDesserializada;
}
