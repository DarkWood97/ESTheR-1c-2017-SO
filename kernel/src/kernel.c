#include "funcionesGenericas.h"
#include "socket.h"
#include <parser/metadata_program.h>

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

//---------------indice de stack----

typedef struct {
	int pagina;
	int offset;
	int size;
} retVar;

typedef struct {
	int pos;
	t_list* args;
	t_list* vars;
	int retPos;
	retVar retVar;
} Stack;

//--------------indicide de codigo-----

typedef struct {
	int PID;
	int PC;
//  int paginas_Codigo;
	t_metadata_program* cod;
// etiquetas etiq;
//	Stack SP;
// int exitCode;
} PCB;

//--------------indice de etiquetas----
// typedef struct{
// char* funcion;
// int etiqueta;
// }etiquetas;

typedef struct
	__attribute__((packed)) {
		int tipoMsj;
		int tamMsj;
		void* mensaje; //CONTROLAR VOID O CHAR?
	} paquete;

	typedef struct {
		int pid;
		int tamaniosPaginas;
		int paginas;
	} tablaKernel;

typedef struct {
	uint32_t size;
	bool isFree;
} HeapMetadata;

//--VARIABLES GLOBALES------------------------------------------------------
#define  CONEXION_CONSOLA  201
#define CORTO  0
#define ERROR -1
#define MENSAJE_IMPRIMIR 101
#define TAMANIO_PAGINA 102
#define MENSAJE_PATH 103
#define HANDSHAKE_MEMORIA 1002
#define INICIAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503

bool YA_HAY_UNA_CONSOLA = false;
int pid_actual = 1;
tablaKernel *tablakernel;

//-----FUNCIONES KERNEL------------------------------------------
kernel crearKernel(t_config *configuracion) {
	kernel kernelAuxiliar;
	verificarParametrosCrear(configuracion, 14);
	kernelAuxiliar.puerto_Prog = config_get_int_value(configuracion,
			"PUERTO_PROG");
	kernelAuxiliar.puerto_Cpu = config_get_int_value(configuracion,
			"PUERTO_CPU");
	kernelAuxiliar.ip_FS.numero = config_get_string_value(configuracion,
			"IP_FS");
	kernelAuxiliar.puerto_Memoria = config_get_int_value(configuracion,
			"PUERTO_MEMORIA");
	kernelAuxiliar.ip_Memoria.numero = config_get_string_value(configuracion,
			"IP_MEMORIA");
	kernelAuxiliar.puerto_FS = config_get_int_value(configuracion, "PUERTO_FS");
	kernelAuxiliar.quantum = config_get_int_value(configuracion, "QUANTUM");
	kernelAuxiliar.quantum_Sleep = config_get_int_value(configuracion,
			"QUANTUM_SLEEP");
	kernelAuxiliar.algoritmo = config_get_string_value(configuracion,
			"ALGORITMO");
	kernelAuxiliar.grado_Multiprog = config_get_int_value(configuracion,
			"GRADO_MULTIPROG");
	kernelAuxiliar.sem_Ids = config_get_array_value(configuracion, "SEM_IDS");
	kernelAuxiliar.sem_Init = config_get_array_value(configuracion, "SEM_INIT");
	kernelAuxiliar.shared_Vars = config_get_array_value(configuracion,
			"SHARED_VARS");
	kernelAuxiliar.stack_Size = config_get_int_value(configuracion,
			"STACK_SIZE");
	return kernelAuxiliar;
}

kernel inicializarKernel(char *path) {
	t_config *configuracionKernel = (t_config*) malloc(sizeof(t_config));
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

//	Stack inicializarStack(){
//		Stack SP;
//		SP.pos = 0;
//		SP.args = list_create();
//		SP.vars = list_create();
//		SP.retPos = 0;
//		SP.retVar.offset = 0;
//		SP.retVar.pagina = 0;
//		SP.retVar.size = 0;
//		return SP;
//	}

t_metadata_program* inicializarCodigo(char* buffer) {
	t_metadata_program* cod;
	cod = metadata_desde_literal(buffer);
	return cod;
}

PCB inicializarPCB(char* buffer) {
	PCB nuevoPCB;
	nuevoPCB.PID = obtenerPID();
	nuevoPCB.PC = 0;
	//nuevoPCB.paginas_Codigo = 0;
	nuevoPCB.cod = inicializarCodigo(buffer);
	//nuevoPCB.SP = inicializarStack();
	return nuevoPCB;
}

void realizarHandshake(int socket, int tipoMsj) {
	int longitud = sizeof(tipoMsj);
	if (send(socket, tipoMsj, longitud, 0) == -1) {
		perror("Error de send");
		close(socket);
		exit(-1);
	}
}

int recibirCantidadPaginas(void* dataRecibida, PCB pcb, int stack_size) {
	int tamanio = stack_size + sizeof(pcb.cod) - sizeof(HeapMetadata);
	int cantidad = (tamanio) % ((int) &dataRecibida);
	if (tamanio % ((int) &dataRecibida) != 0) {
		cantidad++;
	}
	return cantidad;
}

void asignarPagina(int socket, int pid, int cantidadPaginas) {
	paquete paqMemoria;
	void* mensaje = malloc(sizeof(int) * 2);
	memcpy(mensaje, pid, sizeof(int));
	memcpy(mensaje + sizeof(int), cantidadPaginas, sizeof(int));
	paqMemoria.mensaje = mensaje;
	paqMemoria.tamMsj = sizeof(int) * 2;
	paqMemoria.tipoMsj = ASIGNAR_PAGINAS;
	free(mensaje);
}

int obtenerPID() {
	int pid = pid_actual;
	pid_actual = pid_actual++;
	return pid;
}

//------FUNCIONES DE ARRAY----------------------------------------

void imprimirArrayDeChar(char* arrayDeChar[]) {
	int i = 0;
	printf("[");
	for (; arrayDeChar[i] != NULL; i++) {
		printf("%s ", arrayDeChar[i]);
	}
	puts("]");
}

void imprimirArrayDeInt(int arrayDeInt[]) {
	int i = 0;
	printf("[");
	for (; arrayDeInt[i] != NULL; i++) {
		printf("%d ", arrayDeInt[i]);
	}
	puts("]");
}

//-----------------------------------MAIN-----------------------------------------------

int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	//char *path = "Debug/kernel.config";
	//kernel kernel = inicializarKernel(path);
	kernel kernel = inicializarKernel(argv[1]);
	fd_set socketsCliente, socketsConPeticion, socketsMaster; //socketsMaster son los sockets clientes + sockets servidor
	FD_ZERO(&socketsCliente);
	FD_ZERO(&socketsConPeticion);
	FD_ZERO(&socketsMaster);
	mostrarConfiguracionesKernel(kernel);
	int socketParaMemoria, socketParaFileSystem, socketMaxCliente,socketMaxMaster, socketAChequear, socketAEnviarMensaje,socketQueAcepta, bytesRecibidos;
	int socketEscucha = ponerseAEscucharClientes(kernel.puerto_Prog, 0);
	socketParaMemoria = conectarAServer(kernel.ip_Memoria.numero,kernel.puerto_Memoria);
	socketParaFileSystem = conectarAServer(kernel.ip_FS.numero,kernel.puerto_FS);
	FD_SET(socketEscucha, &socketsCliente);
	FD_SET(socketParaMemoria, &socketsMaster);
	FD_SET(socketParaFileSystem, &socketsMaster);
	socketMaxCliente = socketEscucha;
	socketMaxMaster = calcularSocketMaximo(socketParaMemoria,socketParaFileSystem);

	while (1) {
		socketsConPeticion = socketsCliente;
		if (select(socketMaxCliente + 1, &socketsConPeticion, NULL, NULL,NULL) == -1) {
			perror("Error de select");
			exit(-1);
		}
		for (socketAChequear = 0; socketAChequear <= socketMaxCliente;socketAChequear++) {
			PCB nuevoPCB;
			if (FD_ISSET(socketAChequear, &socketsConPeticion)) {
				if (socketAChequear == socketEscucha) {
					socketQueAcepta = aceptarConexionDeCliente(socketEscucha);
					FD_SET(socketQueAcepta, &socketsMaster);
					FD_SET(socketQueAcepta, &socketsCliente);
					socketMaxCliente = calcularSocketMaximo(socketQueAcepta,socketMaxCliente);
					socketMaxMaster = calcularSocketMaximo(socketQueAcepta,socketMaxMaster);
				} else {
					paquete mensajeAux;
					void* dataRecibida;

					recv(socketAChequear, &mensajeAux.tipoMsj, sizeof(int), 0);

					switch (mensajeAux.tipoMsj) {
					case CORTO:
						printf("El socket %d corto la conexion\n",socketAChequear);
						close(socketAChequear);
						FD_CLR(socketAChequear, &socketsMaster);
						FD_CLR(socketAChequear, &socketsCliente);
						break;
					case ERROR:
						perror("Error de recv");
						close(socketAChequear);
						FD_CLR(socketAChequear, &socketsMaster);
						FD_CLR(socketAChequear, &socketsCliente);
						exit(-1);
					case MENSAJE_IMPRIMIR:
						recv(socketAChequear, &mensajeAux.tamMsj, sizeof(int),0);
						dataRecibida = malloc(mensajeAux.tamMsj);
						recv(socketAChequear, &dataRecibida, mensajeAux.tamMsj,0);
						printf("Mensaje recibido: %s\n", dataRecibida);
						free(dataRecibida);
						break;
					case MENSAJE_PATH: //Recibir path de archivo
						recv(socketAChequear, &mensajeAux.tamMsj, sizeof(int),0);
						dataRecibida = malloc(mensajeAux.tamMsj);
						recv(socketAChequear, &dataRecibida, mensajeAux.tamMsj,0);
						char *buffer = malloc(mensajeAux.tamMsj);
						FILE *archivoRecibido = fopen(dataRecibida, "r+w");
						fscanf(archivoRecibido, "%s", buffer);
						nuevoPCB = inicializarPCB(buffer);
						realizarHandshake(socketParaMemoria, HANDSHAKE_MEMORIA);
						free(dataRecibida);
						free(buffer);
						break;
					case TAMANIO_PAGINA:
						recv(socketAChequear, &mensajeAux.tamMsj, sizeof(int),0);
						dataRecibida = malloc(mensajeAux.tamMsj);
						int cantpag = recibirCantidadPaginas(dataRecibida,nuevoPCB, kernel.stack_Size);
						asignarPagina(socketAChequear, nuevoPCB.PID, cantpag);
						free(dataRecibida);
						break;
					case CONEXION_CONSOLA:
						if (YA_HAY_UNA_CONSOLA) {
							perror("Ya hay una consola conectada");
							FD_CLR(socketAChequear, &socketsMaster);
							close(socketAChequear);
						} else {
							YA_HAY_UNA_CONSOLA = true;
							FD_SET(socketAChequear, &socketsMaster);
							recv(socketAChequear, &mensajeAux.tamMsj,sizeof(int), 0);
							dataRecibida = malloc(mensajeAux.tamMsj);
							recv(socketAChequear, &dataRecibida,mensajeAux.tamMsj, 0);
							printf("Handshake con Consola: %s\n", dataRecibida);
							free(dataRecibida);
						}
						break;
					}

//					if((bytesRecibidos = recv(socketAChequear, buff, 16, 0))<=0){
//						revisarSiCortoCliente(socketAChequear, bytesRecibidos);
//						close(socketAChequear);
//					} else {
//						for(socketAEnviarMensaje = 0; socketAEnviarMensaje<=socketMaxMaster; socketAEnviarMensaje++){
//							if(FD_ISSET(socketAEnviarMensaje,&socketsMaster)){
//								chequearErrorDeSend(socketAEnviarMensaje, bytesRecibidos,buff);
//							}
//						}
//						printf("%s\n",buff);
				}
			}
		}
	}
	return EXIT_SUCCESS;
}
