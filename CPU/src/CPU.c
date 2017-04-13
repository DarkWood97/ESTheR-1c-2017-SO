/*
 ============================================================================
 Name        : CPU.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */


#include "funcionesGenericas.h"
#include "socket.h"

typedef struct {
	_ip ipKernel;
	int puertoKernel;
	_ip ipMemoria;
	int puertoMemoria;
}cpu;

cpu cpuCrear(t_config *configuracionCPU){
	cpu nuevaCPU;
	nuevaCPU.ipKernel.numero = config_get_string_value(configuracionCPU, "IP_KERNEL");
	nuevaCPU.puertoKernel = config_get_int_value(configuracionCPU, "PUERTO_KERNEL");
	nuevaCPU.ipMemoria.numero = config_get_string_value(configuracionCPU, "IP_MEMORIA");
	nuevaCPU.puertoMemoria = config_get_int_value(configuracionCPU, "PUERTO_MEMORIA");
	return nuevaCPU;
}

cpu inicializarCPU(char *path){
	t_config *configuracionCPU = malloc(sizeof(t_config));
	*configuracionCPU = generarT_ConfigParaCargar(path);
	cpu cpuSistemas = cpuCrear(configuracionCPU);
	return cpuSistemas;
	free(configuracionCPU);
}

void mostrarConfiguracionCPU(cpu cpuAMostrar){
	printf("PUERTO_KERNEL=%d\n",cpuAMostrar.puertoKernel);
	printf("IP_KENEL=%s\n",cpuAMostrar.ipKernel.numero);
	printf("PUERTO_MEMORIA=%d\n",cpuAMostrar.puertoMemoria);
	printf("IP_MEMORIA=%s\n",cpuAMostrar.ipMemoria.numero);
}



int main(int argc, char *argv[]) {
	if(argc != 2){
		perror("Faltan parametros");
		exit(-1);
	}
	cpu cpuDelSistema = inicializarCPU(argv[1]);
	mostrarConfiguracionCPU(cpuDelSistema);
	int socketParaMemoria, socketParaKernel;
	socketParaMemoria = conectarServer(cpuDelSistema.ipMemoria.numero, cpuDelSistema.puertoMemoria);
	socketParaKernel = conectarServer(cpuDelSistema.ipKernel.numero, cpuDelSistema.puertoKernel);
	return EXIT_SUCCESS;
}
