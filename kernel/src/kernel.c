#include "funcionesGenericas.h"
#include "socket.h"
#include <parser/metadata_program.h>

//--TYPEDEF------------------------------------------------------

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
int TAM_PAGINA;

//-----FUNCIONES KERNEL------------------------------------------
void crearKernel(t_config *configuracion) {

	verificarParametrosCrear(configuracion, 14);
	puerto_Prog = config_get_int_value(configuracion,
			"PUERTO_PROG");
	puerto_Cpu = config_get_int_value(configuracion,
			"PUERTO_CPU");
	ip_FS.numero = config_get_string_value(configuracion,
			"IP_FS");
	puerto_Memoria = config_get_int_value(configuracion,
			"PUERTO_MEMORIA");
	ip_Memoria.numero = config_get_string_value(configuracion,
			"IP_MEMORIA");
	puerto_FS = config_get_int_value(configuracion, "PUERTO_FS");
	quantum = config_get_int_value(configuracion, "QUANTUM");
	quantum_Sleep = config_get_int_value(configuracion,
			"QUANTUM_SLEEP");
	algoritmo = config_get_string_value(configuracion,
			"ALGORITMO");
	grado_Multiprog = config_get_int_value(configuracion,
			"GRADO_MULTIPROG");
	sem_Ids = config_get_array_value(configuracion, "SEM_IDS");
	sem_Init = config_get_array_value(configuracion, "SEM_INIT");
	shared_Vars = config_get_array_value(configuracion,
			"SHARED_VARS");
	stack_Size = config_get_int_value(configuracion,
			"STACK_SIZE");
}

void inicializarKernel(char *path) {
	t_config *configuracionKernel = (t_config*) malloc(sizeof(t_config));
	*configuracionKernel = generarT_ConfigParaCargar(path);
	crearKernel(configuracionKernel);
	free(configuracionKernel);
}
void mostrarConfiguracionesKernel() {
	printf("PUERTO_PROG=%d\n", puerto_Prog);
	printf("PUERTO_CPU=%d\n", puerto_Cpu);
	printf("IP_FS=%s\n", ip_FS.numero);
	printf("PUERTO_FS=%d\n", puerto_FS);
	printf("IP_MEMORIA=%s\n", ip_Memoria.numero);
	printf("PUERTO_MEMORIA=%d\n", puerto_Memoria);
	printf("QUANTUM=%d\n", quantum);
	printf("QUANTUM_SLEEP=%d\n", quantum_Sleep);
	printf("ALGORITMO=%s\n", algoritmo);
	printf("GRADO_MULTIPROG=%d\n", grado_Multiprog);
	printf("SEM_IDS=");
	imprimirArrayDeChar(sem_Ids);
	printf("SEM_INIT=");
	imprimirArrayDeInt(sem_Init);
	printf("SHARED_VARS=");
	imprimirArrayDeChar(shared_Vars);
	printf("STACK_SIZE=%d\n", stack_Size);
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

void realizarHandshake(int socket) {
	int longitud = sizeof(HANDSHAKE_MEMORIA);
	int tipoMensaje = HANDSHAKE_MEMORIA;
	if (send(socket, &tipoMensaje, longitud, 0) == -1) {
		perror("Error de send");
		close(socket);
		exit(-1);
	}
}

int recibirCantidadPaginas(char* codigo) {
	int tamanio = stack_Size + strlen(codigo) - sizeof(HeapMetadata);
	int cantidad = (tamanio) % (TAM_PAGINA);
	if (tamanio % TAM_PAGINA != 0) {
		cantidad++;
	}
	return cantidad;
}

void inicializarPrograma(int socket,char*  buffer) {
	PCB nuevoPCB = inicializarPCB(buffer);
	paquete paqMemoria;
	void* mensaje = malloc(sizeof(int) * 2);
	int cantidadPaginas=recibirCantidadPaginas(buffer);
	memcpy(mensaje, nuevoPCB.PID, sizeof(int));
	memcpy(mensaje + sizeof(int), TAM_PAGINA, sizeof(int));
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
	//inicializarKernel(path);
	inicializarKernel(argv[1]);
	fd_set socketsCliente, socketsConPeticion, socketsMaster; //socketsMaster son los sockets clientes + sockets servidor
	FD_ZERO(&socketsCliente);
	FD_ZERO(&socketsConPeticion);
	FD_ZERO(&socketsMaster);
	mostrarConfiguracionesKernel();
	int socketParaMemoria, socketParaFileSystem, socketMaxCliente,socketMaxMaster, socketAChequear, socketAEnviarMensaje,socketQueAcepta, bytesRecibidos;
	int socketEscucha = ponerseAEscucharClientes(puerto_Prog, 0);
	socketParaMemoria = conectarAServer(ip_Memoria.numero,puerto_Memoria);
	socketParaFileSystem = conectarAServer(ip_FS.numero,puerto_FS);
	FD_SET(socketEscucha, &socketsCliente);
	FD_SET(socketParaMemoria, &socketsMaster);
	FD_SET(socketParaFileSystem, &socketsMaster);
	socketMaxCliente = socketEscucha;
	socketMaxMaster = calcularSocketMaximo(socketParaMemoria,socketParaFileSystem);
	realizarHandshake(socketParaMemoria);

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
						inicializarPrograma(socketAChequear,buffer);
						free(dataRecibida);
						free(buffer);
						break;
					case TAMANIO_PAGINA:
						 if(recv(socketAChequear,&TAM_PAGINA,sizeof(int),0)==-1){
						    perror("Error al recibir el tamanio de pagina");
						    exit(-1);
						  }
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
