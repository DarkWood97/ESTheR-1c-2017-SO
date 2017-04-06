/*
 ============================================================================
 Name        : Kernel.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesGenericas.h"



typedef struct {
	int puerto_Prog;
	int puerto_Cpu;
	char *ip_FS;
	int puerto_Memoria;
	char *ip_Memoria;
	int puerto_FS;
	int quantum;
	int quantum_Sleep;
	char *algoritmo;
	int grado_Multiprog;
	unsigned char *sem_Ids;
	unsigned int *sem_Init;
	unsigned char *shared_Vars;
	int stack_Size;
}kernel;

kernel kernelCrear(t_config *configuracion){
	kernel kernelAuxiliar;
	if(dictionary_size(configuracion->properties)!=14){
			perror("Faltan parametros para inicializar el kernel");
	}
	kernelAuxiliar.puerto_Prog = config_get_int_value(configuracion, "PUERTO_PROG");
	kernelAuxiliar.puerto_Cpu = config_get_int_value(configuracion, "PUERTO_CPU");
	kernelAuxiliar.ip_FS = config_get_string_value (configuracion, "IP_FS");
	kernelAuxiliar.puerto_Memoria = config_get_int_value (configuracion, "PUERTO_MEMORIA");
	kernelAuxiliar.ip_Memoria = config_get_string_value(configuracion, "IP_MEMORIA");
	kernelAuxiliar.puerto_FS = config_get_int_value(configuracion, "PUERTO_FS");
	kernelAuxiliar.quantum = config_get_int_value(configuracion, "QUANTUM");
	kernelAuxiliar.quantum_Sleep = config_get_int_value(configuracion, "QUANTUM_SLEEP");
	kernelAuxiliar.algoritmo = config_get_string_value(configuracion, "ALGORITMO");
	kernelAuxiliar.grado_Multiprog = config_get_int_value(configuracion, "GRADO_MULTIPROG");
	kernelAuxiliar.sem_Ids = config_get_string_value(configuracion, "SEM_IDS");
	kernelAuxiliar.sem_Init = config_get_int_value(configuracion, "SEM_INIT");
	kernelAuxiliar.shared_Vars = config_get_array_value(configuracion, "SHARED_VARS");
	kernelAuxiliar.stack_Size = config_get_int_value(configuracion, "STACK_SIZE");
	return kernelAuxiliar;
}

kernel inicializarKernel (char *path){
	t_config *configuracionKernel;
	*configuracionKernel = generarT_ConfigParaCargar(path);
	kernel kernelSistema = kernelCrear(configuracionKernel);
	return kernelSistema;
	free(configuracionKernel);
}

int main(int argc, char *argv[]) {
	puts("!!!Hello World!!!");
		return EXIT_SUCCESS;
}

