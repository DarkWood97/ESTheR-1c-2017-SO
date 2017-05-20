#include "funcionesGenericas.h"
#include "socket.h"
#include <stdio.h>
#include <math.h>
#include <parser/metadata_program.h>
#include <pthread.h>

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

typedef struct __attribute__((packed)) {
	int pagina;
	int offset;
	int tam;
} retVar;

typedef struct __attribute__((packed)) {
	int pos;
	t_list* args;
	t_list* vars;
	int retPos;
	retVar retVar;
	int tamArgs;
	int tamVars;
} Stack;

//--------------indice de codigo----
typedef struct __attribute__((packed)) {
	int comienzo;
	int offset;
} codeIndex;

//--------------indice de codigo-----

typedef struct __attribute__((packed)) {
	int pid;
	int tamaniosPaginas;
	int paginas;
} TablaKernel;

typedef struct __attribute__((packed)) {
	int PID;
	int ProgramCounter;
	int paginas_Codigo;
	codeIndex cod;
	char* etiquetas;
	int exitCode;
	t_list *contextoActual;
	int tamContextoActual;
	int tamEtiquetas;
	TablaKernel tablaKernel;
} PCB;

typedef struct __attribute__((packed)) {
	int tipoMsj;
	int tamMsj;
	void* mensaje;
} paquete;

typedef struct __attribute__((packed)) {
	uint32_t size;
	bool isFree;
} HeapMetadata;

//--VARIABLES GLOBALES------------------------------------------------------
#define CONEXION_CONSOLA 1005
#define CORTO 0
#define ERROR -1
#define ESPACIO_INSUFICIENTE -2
#define MENSAJE_IMPRIMIR 101
#define TAMANIO_PAGINA 102
#define MENSAJE_PATH 103
#define MEMORIA 1002
#define CPU 1003
#define CONSOLA 1005
#define INICIAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define INICIO_EXITOSO 50
#define MENSAJE_PCB 1015
#define RECIBIR_PCB 1016

t_log * loggerKernel;
bool YA_HAY_UNA_CONSOLA = false;
int pid_actual = 1;
int TAM_PAGINA;
int cantPag=0;
PCB nuevoPCB;
t_list* tablaKernel;

//-----FUNCIONES KERNEL------------------------------------------
void crearKernel(t_config *configuracion) {
	verificarParametrosCrear(configuracion, 14);
	puerto_Prog = config_get_int_value(configuracion, "PUERTO_PROG");
	puerto_Cpu = config_get_int_value(configuracion, "PUERTO_CPU");
	ip_FS.numero = config_get_string_value(configuracion, "IP_FS");
	puerto_Memoria = config_get_int_value(configuracion, "PUERTO_MEMORIA");
	ip_Memoria.numero = config_get_string_value(configuracion,"IP_MEMORIA");
	puerto_FS = config_get_int_value(configuracion, "PUERTO_FS");
	quantum = config_get_int_value(configuracion, "QUANTUM");
	quantum_Sleep = config_get_int_value(configuracion, "QUANTUM_SLEEP");
	algoritmo = config_get_string_value(configuracion, "ALGORITMO");
	grado_Multiprog = config_get_int_value(configuracion,"GRADO_MULTIPROG");
	sem_Ids = config_get_array_value(configuracion, "SEM_IDS");
	sem_Init = config_get_array_value(configuracion, "SEM_INIT");
	shared_Vars = config_get_array_value(configuracion, "SHARED_VARS");
	stack_Size = config_get_int_value(configuracion, "STACK_SIZE");
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

int aumentarPID() {
	int pid = pid_actual;
	pid_actual = pid_actual + 1;
	return pid;
}

int disminuirPID() {
	int pid = pid_actual;
	pid_actual = pid_actual - 1;
	return pid;
}


codeIndex cargarCodeIndex(char* buffer,t_metadata_program* metadata_program) {
	codeIndex code[metadata_program->instrucciones_size];
	int i = 0;

	for (; i < metadata_program->instrucciones_size; i++) {
		log_info(loggerKernel, "Instruccion inicio:%d offset:%d %.*s",metadata_program->instrucciones_serializado[i].start,metadata_program->instrucciones_serializado[i].offset,metadata_program->instrucciones_serializado[i].offset,buffer+ metadata_program->instrucciones_serializado[i].start);
		code[i].comienzo = metadata_program->instrucciones_serializado[i].start;
		code[i].offset = metadata_program->instrucciones_serializado[i].offset;
	}

	return code[metadata_program->instrucciones_size];
}

char* cargarEtiquetasIndex(t_metadata_program* metadata_program,int sizeEtiquetasIndex) {
	char* etiquetas = malloc(sizeEtiquetasIndex * sizeof(char));
	memcpy(etiquetas, metadata_program->etiquetas,sizeEtiquetasIndex * sizeof(char));
	return etiquetas;
}

t_list* inicializarStack(t_list *contexto) {
	Stack* stack;
	t_list *contextocero;
	contextocero = malloc(sizeof(t_list));

	stack->args = list_create();
	stack->vars = list_create();
	stack->pos = 0;
	stack->tamArgs = 0;
	stack->tamVars = 0;

	list_add(contextocero, stack);

	return contextocero;
	free(contextocero);
}

void inicializarPCB(char* buffer) {
	t_metadata_program* metadata_program = metadata_desde_literal(buffer);

	nuevoPCB.PID = aumentarPID();
	nuevoPCB.ProgramCounter = 0;
	nuevoPCB.paginas_Codigo = (int)ceil((double)strlen(buffer) / (double)cantPag);
	nuevoPCB.cod = cargarCodeIndex(buffer, metadata_program);
	nuevoPCB.tamEtiquetas = metadata_program->etiquetas_size;
	nuevoPCB.etiquetas = cargarEtiquetasIndex(metadata_program,nuevoPCB.tamEtiquetas);
	nuevoPCB.contextoActual = list_create();
	nuevoPCB.contextoActual = inicializarStack(nuevoPCB.contextoActual);
	nuevoPCB.tamContextoActual = 1;

	metadata_destruir(metadata_program);
}

void realizarHandshake(int socket, int HANDSHAKE) {
	int longitud = sizeof(HANDSHAKE);
	int tipoMensaje = HANDSHAKE;
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

void prepararProgramaEnMemoria(int socket,int FUNCION) {
	paquete paqMemoria;
	void* mensaje = malloc(sizeof(int) * 2);
	memcpy(mensaje, pid_actual, sizeof(int));
	memcpy(mensaje + sizeof(int), cantPag, sizeof(int));
	paqMemoria.mensaje = mensaje;
	paqMemoria.tamMsj = sizeof(int) * 2;
	paqMemoria.tipoMsj = FUNCION;
	if ((send(socket, &paqMemoria, sizeof(int) * 2, 0)) == -1) {
		perror("Error al enviar el PID y tamanio de pagina");
		exit(-1);
	}
	free(mensaje);
}

void* serializarPCB(){
	void* mensaje = malloc(sizeof(nuevoPCB));

	memcpy(mensaje, nuevoPCB.PID, sizeof(int));
	memcpy(mensaje+sizeof(int), nuevoPCB.ProgramCounter, sizeof(int));
	memcpy(mensaje+(sizeof(int)*2), nuevoPCB.paginas_Codigo, sizeof(int));
	memcpy(mensaje+(sizeof(int)*3), nuevoPCB.cod.comienzo, sizeof(int));
	memcpy(mensaje+(sizeof(int)*4), nuevoPCB.cod.offset, sizeof(int));
	memcpy(mensaje+(sizeof(int)*5), nuevoPCB.etiquetas, sizeof(char)*16);
	memcpy(mensaje+(sizeof(int)*5)+(sizeof(char)*16), nuevoPCB.exitCode, sizeof(int));
	memcpy(mensaje+(sizeof(int)*6)+(sizeof(char)*16), nuevoPCB.contextoActual, sizeof(t_list*)*16);
	memcpy(mensaje+(sizeof(int)*6)+(sizeof(char)*16)+(sizeof(t_list*)*16), nuevoPCB.tamContextoActual, sizeof(int));
	memcpy(mensaje+(sizeof(int)*7)+(sizeof(char)*16)+(sizeof(t_list*)*16), nuevoPCB.tamEtiquetas, sizeof(int));
	memcpy(mensaje+(sizeof(int)*8)+(sizeof(char)*16)+(sizeof(t_list*)*16), nuevoPCB.tablaKernel.paginas, sizeof(int));
	memcpy(mensaje+(sizeof(int)*9)+(sizeof(char)*16)+(sizeof(t_list*)*16), nuevoPCB.tablaKernel.pid, sizeof(int));
	memcpy(mensaje+(sizeof(int)*10)+(sizeof(char)*16)+(sizeof(t_list*)*16), nuevoPCB.tablaKernel.tamaniosPaginas, sizeof(int));

	return mensaje;
	free(mensaje);
}

void recibirPCB(int tamMsj, int socket){
	void* mensaje;
	mensaje = malloc(tamMsj);

	if (recv(socket,mensaje, tamMsj, 0)) {
		perror("Error al recibir PCB");
		exit(-1);
	}

	memcpy(nuevoPCB.PID,mensaje, sizeof(int));
	memcpy(nuevoPCB.ProgramCounter,mensaje+sizeof(int), sizeof(int));
	memcpy(nuevoPCB.paginas_Codigo,mensaje+(sizeof(int)*2), sizeof(int));
	memcpy(nuevoPCB.cod.comienzo,mensaje+(sizeof(int)*3), sizeof(int));
	memcpy(nuevoPCB.cod.offset,mensaje+(sizeof(int)*4), sizeof(int));
	memcpy(nuevoPCB.etiquetas, mensaje+(sizeof(int)*5), sizeof(char)*16);
	memcpy(nuevoPCB.exitCode,mensaje+(sizeof(int)*5)+(sizeof(char)*16), sizeof(int));
	memcpy(nuevoPCB.contextoActual,mensaje+(sizeof(int)*6)+(sizeof(char)*16), sizeof(t_list*)*16);
	memcpy(nuevoPCB.tamContextoActual,mensaje+(sizeof(int)*6)+(sizeof(char)*16)+(sizeof(t_list*)*16),sizeof(int));
	memcpy(nuevoPCB.tamEtiquetas,mensaje+(sizeof(int)*7)+(sizeof(char)*16)+(sizeof(t_list*)*16),sizeof(int));
	memcpy(nuevoPCB.tablaKernel.paginas,mensaje+(sizeof(int)*8)+(sizeof(char)*16)+(sizeof(t_list*)*16), sizeof(int));
	memcpy(nuevoPCB.tablaKernel.pid,mensaje+(sizeof(int)*9)+(sizeof(char)*16)+(sizeof(t_list*)*16), sizeof(int));
	memcpy(nuevoPCB.tablaKernel.tamaniosPaginas,mensaje+(sizeof(int)*10)+(sizeof(char)*16)+(sizeof(t_list*)*16), sizeof(int));

	free(mensaje);
}

void enviarPCB(int socketCPU){
	paquete paqCPU;
	void* mensajePCB = serializarPCB();
	paqCPU.mensaje = mensajePCB;
	paqCPU.tamMsj = sizeof(nuevoPCB);
	paqCPU.tipoMsj = MENSAJE_PCB;
	if ((send(socketCPU, &paqCPU, sizeof(nuevoPCB), 0)) == -1) {
		perror("Error al enviar PCB al CPU");
		exit(-1);
	}
	free(mensajePCB);
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

//-----------------------------------MANEJADOR KERNEL----------------------------------

void *manejadorTeclado(){
	char* comando = malloc(50*sizeof(char));
	size_t tamanioMaximo = 50;

	while(1){
		puts("Esperando comando...");

		getline(&comando, &tamanioMaximo, stdin);

		int tamanioRecibido = strlen(comando);
		comando[tamanioRecibido-1] = '\0';
		if(string_equals_ignore_case(comando,"finalizar programa")){
			//finalizarPrograma();
		}else if(string_equals_ignore_case(comando, "consultar listado")){
			//consultarListado();
		}else if(string_equals_ignore_case(comando, "consultar estado")){
			//consultarEstado();
		}else if(string_equals_ignore_case(comando, "detener planificacion")){
			//detenerPlanificacion();
		}else{
			puts("Error de comando");
		}
		comando = realloc(comando,50*sizeof(char));
	}

	free(comando);
}

void *manejadorConexionCPU (void* socketCPU){
  while(1){
	  paquete paqueteRecibidoDeCPU;
	      if(recv(*(int*)socketCPU, &paqueteRecibidoDeCPU.tipoMsj,sizeof(int),0)==-1){
	        perror("Error al recibir el tipo de mensaje de CPU");
	        exit(-1);
	      }
	      if(recv(*(int*)socketCPU, &paqueteRecibidoDeCPU.tamMsj,sizeof(int),0)==-1){
	        perror("Error al recibir tamanio de mensaje de CPU");
	        exit(-1);
	      }
	      switch (paqueteRecibidoDeCPU.tipoMsj) {
	        case MENSAJE_PCB:
	          enviarPCB(*(int*)socketCPU);
	          break;
	        case RECIBIR_PCB:
	          recibirPCB(*(int*)socketCPU,paqueteRecibidoDeCPU.tamMsj);
	          break;
	        default:
	        	perror("No se reconoce el mensaje enviado por CPU");
	      }
	  }
  }


void* manejadorConexionConsola (void* socketConsola,fd_set (*socketsMaster),void* socketParaMemoria){
  while(1){
	  paquete paqueteRecibidoDeConsola;
	      if(recv(*(int*)socketConsola, &paqueteRecibidoDeConsola.tipoMsj,sizeof(int),0)==-1){
	        perror("Error al recibir el tipo de mensaje de Consola");
	        exit(-1);
	      }
	      if(recv(*(int*)socketConsola, &paqueteRecibidoDeConsola.tamMsj,sizeof(int),0)==-1){
	        perror("Error al recibir tamanio de mensaje de Consola");
	        exit(-1);
	      }
	      paquete mensajeAux;
	      void* dataRecibida;
	      switch (paqueteRecibidoDeConsola.tipoMsj) {
			  case MENSAJE_IMPRIMIR:
				  recv(*(int*)socketConsola, &mensajeAux.tamMsj,sizeof(int), 0);
				  dataRecibida = malloc(mensajeAux.tamMsj);
				  recv(*(int*)socketConsola, &dataRecibida,mensajeAux.tamMsj, 0);
				  printf("Mensaje recibido: %p\n", dataRecibida);
				  free(dataRecibida);
				  break;
			  case MENSAJE_PATH:
				  recv(*(int*)socketConsola, &mensajeAux.tamMsj,sizeof(int), 0);
				  dataRecibida = malloc(mensajeAux.tamMsj);
				  recv(*(int*)socketConsola, &dataRecibida,mensajeAux.tamMsj, 0);
				  char *buffer = malloc(mensajeAux.tamMsj);
				  FILE *archivoRecibido = fopen(dataRecibida, "r+w");
				  fscanf(archivoRecibido, "%s", buffer);
				  log_info(loggerKernel,"Archivo recibido correctamente...\n");
				  cantPag = recibirCantidadPaginas(buffer);
				  inicializarPCB(buffer);
				  prepararProgramaEnMemoria(*(int*)socketParaMemoria,ASIGNAR_PAGINAS);
				  free(dataRecibida);
				  free(buffer);
				  break;
			  case CONEXION_CONSOLA:
				  if (YA_HAY_UNA_CONSOLA) {
					  perror("Ya hay una consola conectada");
					  FD_CLR(*(int*)socketConsola, &(*socketsMaster));
					  close(socketConsola);
				  }
				  else {
					  YA_HAY_UNA_CONSOLA = true;
					  log_info(loggerKernel,"Se registro conexion de Consola...\n");
					  FD_SET(*(int*)socketConsola, &(*socketsMaster));
					  recv(*(int*)socketConsola, &mensajeAux.tamMsj,sizeof(int), 0);
					  dataRecibida = malloc(mensajeAux.tamMsj);
					  recv(*(int*)socketConsola, &dataRecibida,mensajeAux.tamMsj, 0);
					  printf("Handshake con Consola: %p\n",dataRecibida);
					  free(dataRecibida);
				  }
				  break;
			  default:
					perror("No se reconoce el mensaje enviado por Consola");
			  }
	  }
  }

void *manejadorConexionMemoria (void* socketMemoria,void* socketConsola,void* socketCPU){
  while(1){
	  paquete paqueteRecibidoDeMemoria;
	      if(recv(*(int*)socketMemoria, &paqueteRecibidoDeMemoria.tipoMsj,sizeof(int),0)==-1){
	        perror("Error al recibir el tipo de mensaje de Memoria");
	        exit(-1);
	      }
	      if(recv(*(int*)socketMemoria, &paqueteRecibidoDeMemoria.tamMsj,sizeof(int),0)==-1){
	        perror("Error al recibir tamanio de mensaje de Memoria");
	        exit(-1);
	      }
	      paquete mensajeAux;
	      void* dataRecibida;
	      switch (paqueteRecibidoDeMemoria.tipoMsj) {
	      	  case TAMANIO_PAGINA:
	      		  if (recv(*(int*)socketMemoria, &TAM_PAGINA, sizeof(int),0) == -1) {
	      			perror("Error al recibir el tamanio de pagina");
	      			exit(-1);
	      		  }
	      		  break;
	      	  case ESPACIO_INSUFICIENTE:
	      		  disminuirPID();
	      		  int espacioInsuficiente = ESPACIO_INSUFICIENTE;
	      		  if(send(*(int*)socketConsola, &espacioInsuficiente, sizeof(int),0) == -1) {
	      			  error("Error de send");
	      			  exit(-1);
	      		  }
	      		  break;
	      	  case INICIO_EXITOSO: ;
	      	  	  TablaKernel *proceso;
	      	  	  proceso->pid = pid_actual;
	      	  	  proceso->tamaniosPaginas = TAM_PAGINA;
	      	  	  proceso->paginas = cantPag;
	      	  	  list_add(tablaKernel, proceso);
	      	  	  log_info(loggerKernel,"Paginas disponibles para el proceso...\n");
	      	  	  prepararProgramaEnMemoria(*(int*)socketMemoria,INICIAR_PROGRAMA);
	      	  	  enviarPCB(*(int*)socketCPU);
	      	  	  break;
	      	  default:
	      		  perror("No se reconoce el mensaje enviado por Memoria");
	      	  }
	  }
  }
//-------------------FIN DE FUNCIONES MANEJADOR KERNEL----------------------------------

//-----------------------------------MAIN-----------------------------------------------

int main(int argc, char *argv[]) {
	loggerKernel = log_create("Kernel.log", "Kernel", 0, 0);
	tablaKernel = list_create();
	verificarParametrosInicio(argc);
	//char *path = "Debug/kernel.config";
	//inicializarKernel(path);
	inicializarKernel(argv[1]);
	pthread_t hiloManejadorMemoria, hiloManejadorTeclado, hiloManejadorConsola, hiloManejadorCPU;
	int socketParaMemoria, socketCPU, socketConsola, socketParaFileSystem, socketMaxCliente, socketMaxMaster, socketAChequear, socketAEnviarMensaje, socketQueAcepta, bytesRecibidos;
	fd_set socketsCliente, socketsConPeticion, socketsMaster;
	FD_ZERO(&socketsCliente);
	FD_ZERO(&socketsConPeticion);
	FD_ZERO(&socketsMaster);
	mostrarConfiguracionesKernel();
	int socketEscucha = ponerseAEscucharClientes(puerto_Prog, 0);
	socketParaMemoria = conectarAServer(ip_Memoria.numero, puerto_Memoria);
	socketParaFileSystem = conectarAServer(ip_FS.numero, puerto_FS);
	FD_SET(socketEscucha, &socketsCliente);
	FD_SET(socketCPU, &socketsCliente);
	FD_SET(socketConsola, &socketsMaster);
	FD_SET(socketParaMemoria, &socketsMaster);
	FD_SET(socketParaFileSystem, &socketsMaster);
	socketMaxCliente = calcularSocketMaximo(socketEscucha,socketCPU);
	socketMaxMaster = calcularSocketMaximo(calcularSocketMaximo(socketParaMemoria,socketParaFileSystem),socketConsola);
	realizarHandshake(socketParaMemoria,MEMORIA);
	realizarHandshake(socketConsola,CONSOLA);
	realizarHandshake(socketCPU,CPU);
	pthread_create(&hiloManejadorTeclado,NULL,manejadorTeclado,NULL);

	while (1) {
		socketsConPeticion = socketsCliente;
		if (select(socketMaxCliente + 1, &socketsConPeticion, NULL, NULL,NULL) == -1) {
			perror("Error de select");
			exit(-1);
		}
		for (socketAChequear = 0; socketAChequear <= socketMaxCliente;socketAChequear++) {
			if (FD_ISSET(socketAChequear, &socketsConPeticion)) {
				if (socketAChequear == socketEscucha) {
					socketQueAcepta = aceptarConexionDeCliente(socketEscucha);
					FD_SET(socketQueAcepta, &socketsMaster);
					FD_SET(socketQueAcepta, &socketsCliente);
					socketMaxCliente = calcularSocketMaximo(socketQueAcepta,socketMaxCliente);
					socketMaxMaster = calcularSocketMaximo(socketQueAcepta,socketMaxMaster);
				}
				else {
					paquete mensajeAux;
					if(recv(socketAChequear,&mensajeAux.tipoMsj,sizeof(int),0)==-1){
						perror("Error de recv en kernel");
						exit(-1);
					}else{
						switch (mensajeAux.tipoMsj) {
							case CONSOLA:
								pthread_create(&hiloManejadorCPU,NULL,manejadorConexionConsola,(socketAChequear,&socketsMaster,socketParaMemoria));
								log_info(loggerKernel,"Se conecto nueva consola...\n");
								FD_CLR(socketAChequear,&socketsMaster);
								break;
							case MEMORIA:
								pthread_create(&hiloManejadorCPU,NULL,manejadorConexionMemoria,(socketAChequear,socketConsola,socketCPU));
								FD_CLR(socketAChequear,&socketsMaster);
								break;
							case CPU:
								pthread_create(&hiloManejadorCPU,NULL,manejadorConexionCPU,socketAChequear);
								log_info(loggerKernel,"Se registro nueva CPU...\n");
								FD_CLR(socketAChequear,&socketsCliente);
								break;
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
							default:
								puts("Conexion erronea");
								FD_CLR(socketAChequear, &socketsMaster);
								FD_CLR(socketAChequear, &socketsCliente);
								close(socketAChequear);
							}
						}
					}
				}
			}
		}
		return EXIT_SUCCESS;
}
