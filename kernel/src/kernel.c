#include "funcionesGenericas.h"
#include "socket.h"


typedef struct {
	int puerto_Prog;
	int puerto_Cpu;
	_ip ip_FS; /* Se pone _ para que lo tome de la libreria */
	int puerto_Memoria;
	_ip ip_Memoria;
	int puerto_FS;
	int quantum;
	int quantum_Sleep;
	char *algoritmo;
	int grado_Multiprog;
	unsigned char *sem_Ids;
	unsigned int *sem_Init;
	unsigned char *shared_Vars;
	int stack_Size;
} kernel;

kernel kernelCrear(t_config *configuracion) {
	kernel kernelAuxiliar;
	if (dictionary_size(configuracion->properties) != 14) {
		perror("Faltan parametros para inicializar el kernel");
	}
	kernelAuxiliar.puerto_Prog = config_get_int_value(configuracion,"PUERTO_PROG");
	kernelAuxiliar.puerto_Cpu = config_get_int_value(configuracion,"PUERTO_CPU");
	kernelAuxiliar.ip_FS.numero = config_get_string_value(configuracion, "IP_FS");
	kernelAuxiliar.puerto_Memoria = config_get_int_value(configuracion,"PUERTO_MEMORIA");
	kernelAuxiliar.ip_Memoria.numero = config_get_string_value(configuracion,"IP_MEMORIA");
	kernelAuxiliar.puerto_FS = config_get_int_value(configuracion, "PUERTO_FS");
	kernelAuxiliar.quantum = config_get_int_value(configuracion, "QUANTUM");
	kernelAuxiliar.quantum_Sleep = config_get_int_value(configuracion,"QUANTUM_SLEEP");
	kernelAuxiliar.algoritmo = config_get_string_value(configuracion,"ALGORITMO");
	kernelAuxiliar.grado_Multiprog = config_get_int_value(configuracion,"GRADO_MULTIPROG");
	kernelAuxiliar.sem_Ids = config_get_array_value(configuracion, "SEM_IDS");
	kernelAuxiliar.sem_Init = config_get_array_value(configuracion, "SEM_INIT");
	kernelAuxiliar.shared_Vars = config_get_array_value(configuracion,"SHARED_VARS");
	kernelAuxiliar.stack_Size = config_get_int_value(configuracion,"STACK_SIZE");
	return kernelAuxiliar;
}

kernel inicializarKernel(char *path) {
	t_config *configuracionKernel;
	*configuracionKernel = generarT_ConfigParaCargar(path);
	kernel kernelSistema = kernelCrear(configuracionKernel);
	return kernelSistema;
	free(configuracionKernel);
}

void imprimirArrayDeChar(char* arrayDeChar[]){
	int i = 0;
	for(; arrayDeChar[i]!=NULL; i++){
		printf("%s/n",arrayDeChar[i]);
	}
}

void imprimirArrayDeInt(int arrayDeInt[]){
	int i = 0;
	for(; arrayDeInt[i]!=NULL; i++){
		printf("%d/n",arrayDeInt[i]);
	}
}

void mostrarConfiguracionesKernel(kernel kernel) {
	printf("PUERTO_PROG=%d\n", kernel.puerto_Prog);
	printf("PUERTO_CPU=%d\n", kernel.puerto_Cpu);
	printf("IP_FS=%s\n", kernel.ip_FS.numero);
	printf("PUERTO_FS=%d\n", kernel.puerto_FS);
	printf("IP_MEMORIA=%s\n", kernel.ip_Memoria.numero);
	printf("PUERTO_MEMORIA=%d\n", kernel.puerto_Memoria);
	printf("QUANTUM=%d\n", kernel.quantum);
	printf("QUANTUM_SLEEP=%d\n", kernel.quantum_Sleep);
	printf("ALGORITMO=%s\n", kernel.algoritmo);
	printf("GRADO_MULTIPROG=%d\n", kernel.grado_Multiprog);
	puts("SEM_IDS=");
	imprimirArrayDeChar(kernel.sem_Ids);
	puts("SEM_INIT=");
	imprimirArrayDeInt(kernel.sem_Init);
	puts("SHARED_VARS=");
	imprimirArrayDeChar(kernel.shared_Vars);
	printf("STACK_SIZE=%d\n", kernel.stack_Size);
}

int main(int argc, char *argv[]) {
	if (argc != 1) {
		perror("Faltan parametros");
	}
	kernel kernel = inicializarKernel(argv[0]);
	mostrarConfiguracionesKernel(kernel);
	int socketMemoria, socketFileSystem;
	int socketListener = ponerseAEscuchar(kernel.puerto_Prog, 0);
	int socketListenerCPU = ponerseAEscuchar(kernel.puerto_Cpu, 0);
//	int socketAceptador = aceptarConexion(socketListener);
	socketMemoria = conectarServer(kernel.ip_Memoria.numero, kernel.puerto_Memoria);
	socketFileSystem = conectarServer(kernel.ip_FS.numero, kernel.puerto_FS);
	seleccionarYAceptarSockets(socketListener);
	return EXIT_SUCCESS;
}
