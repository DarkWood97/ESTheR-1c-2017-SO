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

//typedef struct __attribute__((packed)) {
//	int pid;
//	int tamaniosPaginas;
//	int paginas;
//} TablaKernel;
//typedef struct __attribute__((packed)) {
//	int comienzo;
//	int offset;
//} codeIndex;
//typedef struct __attribute__((packed)) {
//	int PID;
//	int ProgramCounter;
//	int paginas_Codigo;
//	codeIndex cod;
//	char* etiquetas;
//	int exitCode;
//	t_list *contextoActual;
//	int tamContextoActual;
//	int tamEtiquetas;
//	TablaKernel tablaKernel;
//} PCB;
//typedef struct __attribute__((__packed__)){
//	int tamMsj;
//	int tipoMsj;
//	void* mensaje;
//}paquete;

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
paquete serializar(void* bufferDeData, int tipoDeMensaje)
{
  paquete paqueteAEnviar;
  int longitud= string_length(bufferDeData);
  paqueteAEnviar.mensaje=malloc(sizeof(char)*16);
  paqueteAEnviar.tamMsj = longitud;
  paqueteAEnviar.tipoMsj=tipoDeMensaje;
  paqueteAEnviar.mensaje = bufferDeData;
  return paqueteAEnviar;
  free(paqueteAEnviar.mensaje);
}

paquete serializarCod(codeIndex* cod, int tipoDeMensaje)
{
  paquete paqueteAEnviar;
  int longitud= sizeof(codeIndex);
  paqueteAEnviar.mensaje=malloc(longitud);
  paqueteAEnviar.tamMsj = longitud;
  paqueteAEnviar.tipoMsj= tipoDeMensaje;
  paqueteAEnviar.mensaje = cod;
  return paqueteAEnviar;
}

void enviar(int socket, paquete mensaje) { //Socket que envia mensaje

	int longitud = sizeof(mensaje); //sino no lee \0
		if (send(socket, &mensaje, longitud, 0) == -1) {
			perror("Error de send");
			close(socket);
			exit(-1);
	}

}
void realizarHandshake(int socket, int handshake)
{ //Socket que envia mensaje
	paquete paqueteAEnviar,auxiliar;
	paqueteAEnviar.mensaje=malloc(sizeof(char)*16);
	auxiliar=serializar(NULL,handshake);
	memcpy(&paqueteAEnviar, &auxiliar, sizeof(auxiliar)); /*no se si es sizeof(paquete)*/
	enviar(socket,paqueteAEnviar);
	free(paqueteAEnviar.mensaje);
}
void destruirPaquete(paquete * paquete)
{
	free(paquete->mensaje);
	free(paquete);
}

void* serializarPCB(PCB  pcb){
	void* mensaje = malloc(sizeof(pcb));
	memcpy(mensaje, &pcb.PID, sizeof(int));
	memcpy(mensaje+sizeof(int), &pcb.ProgramCounter, sizeof(int));
	memcpy(mensaje+(sizeof(int)*2), &pcb.paginas_Codigo, sizeof(int));
	memcpy(mensaje+(sizeof(int)*3), &pcb.cod.comienzo, sizeof(int));
	memcpy(mensaje+(sizeof(int)*4), &pcb.cod.offset, sizeof(int));
	memcpy(mensaje+(sizeof(int)*5), pcb.etiquetas, sizeof(char)*16);
	memcpy(mensaje+(sizeof(int)*5)+(sizeof(char)*16), &pcb.exitCode, sizeof(int));
	memcpy(mensaje+(sizeof(int)*6)+(sizeof(char)*16), &pcb.contextoActual, sizeof(t_list*)*16);
	memcpy(mensaje+(sizeof(int)*6)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tamContextoActual, sizeof(int));
	memcpy(mensaje+(sizeof(int)*7)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tamEtiquetas, sizeof(int));
	memcpy(mensaje+(sizeof(int)*8)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tablaKernel.paginas, sizeof(int));
	memcpy(mensaje+(sizeof(int)*9)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tablaKernel.pid, sizeof(int));
	memcpy(mensaje+(sizeof(int)*10)+(sizeof(char)*16)+(sizeof(t_list*)*16), &pcb.tablaKernel.tamaniosPaginas, sizeof(int));
	return mensaje;
}
int _obtenerSizeActual_(int b)
	{
		int auxiliar=sizeof(int)*b+sizeof(char)*16+sizeof(t_list)*16;
		b++;
		return auxiliar;
	}


void lineaParaElAnalizador(char* sentencia, char * aDevolver){

		int i = string_length(sentencia);
		while (string_ends_with(sentencia, "\n")) {
			i--;
			sentencia = string_substring_until(sentencia, i);
		}
		aDevolver=sentencia;
}
