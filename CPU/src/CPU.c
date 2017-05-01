/*
 ============================================================================
 Name        : CPU.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "socket.h"
#include <parser/metadata_program.h>
#include <parser/parser.h>
#include "ansiSop.h"
#include "funcionesCPU.h"
//----------------------VARIABLES GLOBALES-TIPO MENSAJES---------------------------
#define HANDSHAKE_MEMORIA 1001
#define HANDSHAKE_KERNEL 1002
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define ERROR -1
#define CORTO 0
//---ANSISOP--------------------------------------------------------------
AnSISOP_funciones primitivas = {
		.AnSISOP_definirVariable		= definirVariable,
		.AnSISOP_obtenerPosicionVariable = obtenerPosicionVariable,
		.AnSISOP_dereferenciar = dereferenciar,
		.AnSISOP_asignar = asignar,
		.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_irAlLabel = irAlLabel,
		.AnSISOP_llamarSinRetorno = llamarSinRetorno,
		.AnSISOP_llamarConRetorno = llamarConRetorno,
		.AnSISOP_finalizar = finalizar,
		.AnSISOP_retornar = retornar,
};
//--TYPEDEF----------------------------------------------------------------
typedef struct {
	char *numero;
}ip;
typedef struct {
	ip ipKernel;
	int puertoKernel;
	ip ipMemoria;
	int puertoMemoria;
}cpu;
typedef struct __attribute__((__packed__)){
	int tamMsj;
	int tipoMsj;
	void* mensaje;
}paquete;


//----FUNCIONES CPU--------------------------------------------------------
cpu cpuCrear(t_config *configuracionCPU){
	cpu nuevaCPU;
	nuevaCPU.ipKernel.numero = config_get_string_value(configuracionCPU, "IP_KERNEL");
	nuevaCPU.puertoKernel = config_get_int_value(configuracionCPU, "PUERTO_KERNEL");
	nuevaCPU.ipMemoria.numero = config_get_string_value(configuracionCPU, "IP_MEMORIA");
	nuevaCPU.puertoMemoria = config_get_int_value(configuracionCPU, "PUERTO_MEMORIA");
	return nuevaCPU;
}

cpu inicializarCPU(char *path){
	t_config *configuracionCPU = (t_config*)malloc(sizeof(t_config));
	configuracionCPU =generarT_ConfigParaCargar(path);
	cpu cpuSistemas;
	cpuSistemas= cpuCrear(configuracionCPU);
	return cpuSistemas;
	free(configuracionCPU);
}

void mostrarConfiguracionCPU(cpu cpuAMostrar){
	printf("PUERTO_KERNEL=%d\n",cpuAMostrar.puertoKernel);
	printf("IP_KERNEL=%s\n",cpuAMostrar.ipKernel.numero);
	printf("PUERTO_MEMORIA=%d\n",cpuAMostrar.puertoMemoria);
	printf("IP_MEMORIA=%s\n",cpuAMostrar.ipMemoria.numero);
}
//----------------ENVIAR HANDSHAKE---------------------------------------------
void realizarHandshake(int socket, paquete mensaje) { //Socket que envia mensaje

	int longitud = sizeof(mensaje); //sino no lee \0
		if (send(socket, &mensaje, longitud, 0) == -1) {
			perror("Error de send");
			close(socket);
			exit(-1);
	}

}
//-------------------SERIALIZACION---------------------------------------------
int obtenerLongitudBuff(char* path)
{
	int longitudPath=strlen(path)+1;
	return longitudPath;

}
paquete serializar(void *bufferDeData, int tipoDeMensaje) {
  paquete paqueteAEnviar;
  int longitud= obtenerLongitudBuff(bufferDeData);
  paqueteAEnviar.mensaje=malloc(sizeof(char)*16);
  paqueteAEnviar.tamMsj = longitud;
  paqueteAEnviar.tipoMsj=tipoDeMensaje;
  paqueteAEnviar.mensaje = bufferDeData;
  return paqueteAEnviar;
}
//----------------RECIBIR HANDSHAKE--------------------------------------------
void recibirMensaje(int socket, void * aRecibir){
	void *buff = malloc(sizeof(void*));
	if(recv(socket,buff,16,0) == -1){
		perror("Error de receive");
		exit(-1);
	}
	aRecibir=buff;
	free(buff);
}
void deserealizarMensaje(int socket)
{
	int *tamanio;
	recv(socket,tamanio,16,0);
	paquete recibido;
	recv(socket,&recibido,(int)tamanio,0);
	int caso=recibido.tipoMsj;
	switch(caso)
	{
	case MENSAJE_IMPRIMIR:
		printf("Mensaje recibido: %p",recibido.mensaje);
		break;
	case HANDSHAKE_KERNEL:
		printf("Handshake con kernel \n Mensaje recibido: %p", recibido.mensaje);
		break;
	case HANDSHAKE_MEMORIA:
		printf("Handshake con memoria \n Mensaje recibido:%p", recibido.mensaje);
		break;
	case CORTO:
		printf("El socket %d corto la conexion", socket);
		close(socket);
		break;
	case ERROR:
		perror("Error de recv");
		exit(-1);
		break;
	case MENSAJE_PATH:
		printf("Path recibido: %p ", recibido.mensaje);
		break;
	}
}

//----------------------MAIN---------------------------------------------------
int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	cpu cpuDelSistema = inicializarCPU(argv[1]);
	mostrarConfiguracionCPU(cpuDelSistema);
	int socketParaMemoria, socketParaKernel;
	socketParaMemoria = conectarAServer(cpuDelSistema.ipMemoria.numero, cpuDelSistema.puertoMemoria);
	deserealizarMensaje(socketParaMemoria);
	socketParaKernel = conectarAServer(cpuDelSistema.ipKernel.numero, cpuDelSistema.puertoKernel);
	deserealizarMensaje(socketParaKernel);
	/*handshake con memoria */
	paquete paqueteAEnviar,auxiliar;
	auxiliar=serializar(NULL,HANDSHAKE_MEMORIA);
	memcpy(&paqueteAEnviar, &auxiliar, sizeof(auxiliar)); /*no se si es sizeof(paquete)*/
	realizarHandshake(socketParaMemoria,paqueteAEnviar);
	/*Handshake kernel*/
	paquete paqueteKernel, auxiliarKernel;
	auxiliar=serializar(NULL, HANDSHAKE_KERNEL);
	memcpy(&paqueteKernel,&auxiliar,sizeof(auxiliarKernel));
	realizarHandshake(socketParaKernel,paqueteKernel);
	close(socketParaMemoria);
	close(socketParaKernel);
	return EXIT_SUCCESS;
}
