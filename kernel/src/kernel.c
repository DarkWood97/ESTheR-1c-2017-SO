#include "funcionesGenericas.h"
#include "socket.h"
//--TYPEDEF------------------------------------------------------

typedef struct {
	int puerto_Prog;
	int puerto_Cpu;
	_ip ip_FS;
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
//-----FUNCIONES KERNEL------------------------------------------
kernel crearKernel(t_config *configuracion) {
	kernel kernelAuxiliar;
	verificarParametrosCrear(configuracion,14);
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
	t_config *configuracionKernel = (t_config*)malloc(sizeof(t_config));
	*configuracionKernel = generarT_ConfigParaCargar(path);
	kernel kernelSistema = crearKernel(configuracionKernel);
	return kernelSistema;
	free(configuracionKernel);
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
	printf("SEM_IDS=");
	imprimirArrayDeChar(kernel.sem_Ids);
	printf("SEM_INIT=");
	imprimirArrayDeInt(kernel.sem_Init);
	printf("SHARED_VARS=");
	imprimirArrayDeChar(kernel.shared_Vars);
	printf("STACK_SIZE=%d\n", kernel.stack_Size);
}
//------FUNCIONES DE ARRAY----------------------------------------

void imprimirArrayDeChar(char* arrayDeChar[]){
	int i = 0;
	printf("[");
	for(; arrayDeChar[i]!=NULL; i++){
		printf("%s ",arrayDeChar[i]);
	}
	puts("]");
}

void imprimirArrayDeInt(int arrayDeInt[]){
	int i = 0;
	printf("[");
	for(; arrayDeInt[i]!=NULL; i++){
		printf("%d ",arrayDeInt[i]);
	}
	puts("]");
}

//-----------------------------------------------------------------------------------------------


int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	//char *path = "Debug/kernel.config";
	//kernel kernel = inicializarKernel(path);
	kernel kernel = inicializarKernel(argv[1]);
	fd_set master, read_Socket;
	FD_ZERO(&master);
	FD_ZERO(&read_Socket);
	mostrarConfiguracionesKernel(kernel);
	int socketParaMemoria, socketParaFileSystem, socketMax;
	int socketListener = ponerseAEscucharClientes(kernel.puerto_Prog, 0);
	socketMax=socketListener;
	FD_SET(socketListener,&master);
	//int socketListenerCPU = ponerseAEscuchar(kernel.puerto_Cpu, 0);
	//int socketAceptador = aceptarConexion(socketListener);
	//int socketAceptadorCPU = aceptarConexion(socketListenerCPU);
	socketParaMemoria = conectarAServer(kernel.ip_Memoria.numero, kernel.puerto_Memoria);
	socketParaFileSystem = conectarAServer(kernel.ip_FS.numero, kernel.puerto_FS);
	while(1){
		read_Socket=master;
		socketMax = seleccionarYAceptarConexiones(&master,socketMax,socketListener,&read_Socket);
		recibirYReenviarMensaje(socketMax,&master,socketListener,&read_Socket);
	}

	//seleccionarYAceptarSockets(socketListenerCPU);
	return EXIT_SUCCESS;
}

