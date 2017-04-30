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
	configuracionCPU = generarT_ConfigParaCargar(path);
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


int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	cpu cpuDelSistema = inicializarCPU(argv[1]);
	mostrarConfiguracionCPU(cpuDelSistema);
	int socketParaMemoria, socketParaKernel;
	socketParaMemoria = conectarAServer(cpuDelSistema.ipMemoria.numero, cpuDelSistema.puertoMemoria);
	socketParaKernel = conectarAServer(cpuDelSistema.ipKernel.numero, cpuDelSistema.puertoKernel);
	recibirMensajeDeKernel(socketParaKernel);
	//deserealizar
	close(socketParaMemoria);
	close(socketParaKernel);
	return EXIT_SUCCESS;
}
