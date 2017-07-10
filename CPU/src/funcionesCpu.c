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

/*
 * funcionesGenericas.c
 *
 *  Created on: 6/4/2017
 *      Author: utnso
 */

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/types.h>
//#include <sys/socket.h>
//#include <sys/time.h>
//#include <sys/types.h>
//#include <sys/wait.h>
//#include <netinet/in.h>
//#include <unistd.h>
//#include <fcntl.h>
//#include <errno.h>
//#include <pthread.h>
//#include <arpa/inet.h>
//#include <signal.h>
//#include <netdb.h>
//#include <commons/log.h>
//#include <commons/string.h>
//#include <commons/config.h>
//#include <commons/collections/dictionary.h>
//#include <commons/collections/list.h>
//#include "funcionesCpu.h"

//t_config *generarT_ConfigParaCargar(char *path) {
//	t_config *configuracionDelComponente;
//	if((configuracionDelComponente = config_create(path)) == NULL){
//		perror("Error de ruta de archivo de configuracion");
//		exit(-1);
//	}
//	return configuracionDelComponente;
//}

//void recibirMensajeDeKernel(int socketKernel){
//	char *buff = malloc(sizeof(char)*16);
//	int tamanioBuff = strlen(buff)+1;
//	if(recv(socketKernel,buff,tamanioBuff,0) == -1){
//		perror("Error de receive");
//		exit(-1);
//	}
//	printf("%s",buff);
//	free(buff);
//}
//void verificarParametrosInicio(int argc)
//{
//	if(argc!=2){
//			perror("Faltan parametros");
//			exit(-1);
//		}
//}
//void verificarParametrosCrear(t_config* configuracion, int sizeStruct)
//{
//	if (dictionary_size(configuracion->properties) != sizeStruct) {
//			perror("Faltan parametros para inicializar el fileSystem");
//			exit(-1);
//		}
//}


void *serializarT_Intructions(PCB *pcbConT_intruction){
  void *serializacion = malloc(sizeof(t_intructions)*pcbConT_intruction->cantidadDeT_Intructions);
  int i, auxiliar1 = 0;
  for(i = 0; i<pcbConT_intruction->cantidadDeT_Intructions; i++){
    memcpy(serializacion+sizeof(int)*auxiliar1, &pcbConT_intruction->cod[i].start, sizeof(int));
    auxiliar1++;
    memcpy(serializacion+sizeof(int)*auxiliar1,&pcbConT_intruction->cod[i].offset, sizeof(int));
    auxiliar1++;
  }
  return serializacion;
}

void* serializarVariable(t_list* variables){
	void* variableSerializada = malloc(list_size(variables)*sizeof(variable));
	int i;
	int cantidadDeVariables = list_size(variables);
	memcpy(variableSerializada, &cantidadDeVariables, sizeof(int));
	for(i = 1; i<cantidadDeVariables; i++){
		variable *variableObtenida = list_get(variables, i);
    	memcpy(variableSerializada+sizeof(variable)*i, variableObtenida, sizeof(variable));
	}
	return variableSerializada;
}

int sacarTamanioDeLista(t_list* contexto){
	int i, tamanioDeContexto;
	for(i = 0; i<list_size(contexto); i++){
		Stack *contextoAObtener = list_get(contexto, i);
		tamanioDeContexto += sizeof(int)*4+sizeof(direccion)+list_size(contextoAObtener->args)*sizeof(variable)+list_size(contextoAObtener->vars)*sizeof(variable);
		//free(contextoAObtener);
	}
	return tamanioDeContexto;
}

void *serializarContextoActual(PCB* pcbConContextos){
	void* contextoSerializado = malloc(sacarTamanioDeLista(pcbConContextos->contextos));
  int i;
  for(i = 0; i<pcbConContextos->tamContextos; i++){
    Stack* contexto = list_get(pcbConContextos->contextos, i);
    memcpy(contextoSerializado, &contexto->pos, sizeof(int));
    void *argsSerializadas = serializarVariable(contexto->args);
    void *varsSerializadas = serializarVariable(contexto->vars);
    memcpy(contextoSerializado+sizeof(int), argsSerializadas, list_size(contexto->args)*sizeof(variable));
    memcpy(contextoSerializado+sizeof(int)+list_size(contexto->args)*sizeof(variable), varsSerializadas, list_size(contexto->vars)*sizeof(variable));
    memcpy(contextoSerializado+list_size(contexto->args)*sizeof(variable)+list_size(contexto->vars)*sizeof(variable), &contexto->retPos, sizeof(int));
  }
  return contextoSerializado;
}

int sacarTamanioPCB(PCB* pcbASerializar){
 int tamanio= sizeof(int)*7+sizeof(t_intructions)*pcbASerializar->cantidadDeT_Intructions+sacarTamanioDeLista(pcbASerializar->contextos)+string_length(pcbASerializar->etiquetas);
 return tamanio;
}

void *serializarPCB(PCB* pcbASerializar){
	int tamanioPCB = sacarTamanioPCB(pcbASerializar);
	void* pcbSerializada = malloc(tamanioPCB);
	memcpy(pcbSerializada, &pcbASerializar->pid, sizeof(int));
	memcpy(pcbSerializada+sizeof(int), &pcbASerializar->programCounter, sizeof(int));
	memcpy(pcbSerializada+sizeof(int)*2, &pcbASerializar->cantidadPaginasCodigo, sizeof(int));
	void* indiceDeCodigoSerializado = serializarT_Intructions(pcbASerializar);
	memcpy(pcbSerializada+sizeof(int)*3, &pcbASerializar->cantidadDeT_Intructions, sizeof(int));
	memcpy(pcbSerializada+sizeof(int)*4, indiceDeCodigoSerializado, sizeof(t_intructions)*pcbASerializar->cantidadDeT_Intructions);
	memcpy(pcbSerializada+sizeof(int)*4+sizeof(t_intructions)*pcbASerializar->cantidadDeT_Intructions, &pcbASerializar->tamEtiquetas, sizeof(int));
	memcpy(pcbSerializada+ sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadDeT_Intructions, pcbASerializar->etiquetas, pcbASerializar->tamEtiquetas); //CHEQUEAR QUE ES TAMETIQUETAS
	memcpy(pcbSerializada+sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadDeT_Intructions+pcbASerializar->tamEtiquetas, &pcbASerializar->exitCode, sizeof(int));
	void* contextoActualSerializado = serializarContextoActual(pcbASerializar);
	memcpy(pcbSerializada+sizeof(int)*6+sizeof(t_intructions)*pcbASerializar->cantidadDeT_Intructions+pcbASerializar->tamEtiquetas, &pcbASerializar->tamContextos, sizeof(int));
	memcpy(pcbSerializada+sizeof(int)*7+sizeof(t_intructions)*pcbASerializar->cantidadDeT_Intructions+pcbASerializar->tamEtiquetas+sacarTamanioDeLista(pcbASerializar->contextos), contextoActualSerializado, sacarTamanioDeLista(pcbASerializar->contextos));
	return pcbSerializada;
}
