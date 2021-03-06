#include "funcionesGenericas.h"
#include "socket.h"
#include <stdio.h>
#include <math.h>
#include <parser/metadata_program.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <semaphore.h>

//HANDSHAKES
#define CPU 1003
#define HANDSHAKE_CPU 1013
#define CONSOLA 1005
#define SOY_KERNEL_MEMORIA 1002
#define SOY_KERNEL_FS 1003
#define TAMANIO_PAGINA 1020
#define SOY_KERNEL_FS 1003
#define HANG_UP 0

//OPERACIONES CPU
#define PETICION_VARIABLE_COMPARTIDA_KERNEL 100
#define PETICION_CAMBIO_VALOR_KERNEL 101
#define DATOS_VARIABLE_COMPARTIDA_KERNEL 102
#define RESERVAR_HEAP 103
#define LIBERAR_HEAP 104
#define WAIT_SEMAFORO 105
#define SIGNAL_SEMAFORO 106
#define BLOQUEO_POR_SEMAFORO 107
#define NO_BLOQUEO_POR_SEMAFORO 108
#define OPERACION_VARIABLE_COMPARTIDA_FALLIDA 109

#define ABRIR_ARCHIVO 300
#define NO_SE_PUDO_ABRIR_ARCHIVO 301
#define BORRAR_ARCHIVO 302
#define CERRAR_ARCHIVO 303
#define MOVER_CURSOR 304
#define ESCRIBIR_DATOS_ARCHIVO 305
#define LEER_DATOS_ARCHIVO 306

#define PROCESO_BLOQUEADO 2003
#define PROCESO_FINALIZADO 2002
#define PROCESO_ABORTADO 2001
#define PROCESO_FINALIZO_RAFAGA 2004
//ESTADOS PROCESOS
#define NUEVO 1
#define LISTO 2
#define EJECUTANDO 3
#define BLOQUEADO 4
#define FINALIZADO 5

#define MENSAJE_CODIGO 103
#define ENVIAR_PID 105
//#define FINALIZAR_PROGRAMA 503
#define INICIAR_PROGRAMA 501
#define ESCRIBIR_DATOS 505
#define MENSAJE_PCB 2000
#define ASIGNAR_PAGINAS 502
#define LIBERAR_PAGINA 506
/*#define SOY_KERNEL 1002*/
#define FINALIZAR_PROCESO 503
#define OPERACION_CON_CPU_FALLIDA -1
#define OPERACION_EXITOSA 1
#define ESPACIO_INSUFICIENTE -2
#define FINALIZO_CORRECTAMENTE 101
#define FINALIZO_INCORRECTAMENTE 102


//FILE SYSTEM
#define HANDSHAKE_ACEPTADO 9
#define GUARDAR_DATOS 403
#define OBTENER_DATOS 404
#define CREAR_ARCHIVO 405
#define BORRAR 406
#define VALIDAR_ARCHIVO 407

#define OPERACION_CON_ARCHIVO_EXITOSA 10

#define OPERACION_FINALIZADA_CORRECTAMENTE 410
#define OPERACION_FALLIDA 411
#define EXISTE_ARCHIVO 412
#define NO_EXISTE_ARCHIVO 413
#define DATOS_DE_LECTURA 414



//------------------------------------------------ESTRUCTURAS--------------------------------------//
typedef struct __attribute__((__packed__)) {
	uint32_t size;
	bool estaLibre;
}heapMetadata;

typedef struct __attribute__((__packed__)){
	heapMetadata* heapDeBloque;
	int offset;
}bloqueHeapMetadata;

typedef struct __attribute__((__packed__)) {
	int pagina;
	int tamanioDisponible;
	t_list* bloqueHeapMetadata;
}paginaHeap;

typedef struct __attribute__((__packed__)) {
	int pid;
	t_list* pagina;
}tablaHeap;

typedef struct __attribute__((__packed__))
{
  int pagina;
  int offset;
  int tamanio;
} direccion;

typedef struct __attribute__((__packed__))
{
	char* script;
	int pid;
}datosPrograma;

typedef struct __attribute__((__packed__)){
  int posicion;
  t_list* args;
  t_list* vars;
  int retPos;
  direccion retVar;
} stack;

typedef struct{
  char nombreVariable;
  direccion *direccionDeVariable;
}variable;

typedef struct __attribute__((__packed__)) {
	int pid;
	int estado;
	int programCounter;
	int posicionStackActual;
	t_intructions *indiceCodigo;
	int cantidadTIntructions;
	int cantidadPaginasCodigo;
	t_list* indiceStack;
	char* indiceEtiquetas;
	tablaHeap* tablaHeap;
	int exitCode;
	int socketConsola;
	int tamanioEtiquetas;
	bool estaAbortado;
	int rafagas;
	int tamanioContexto;
}PCB;

typedef struct __attribute__((__packed__)) {
	int globalFD;
	char* file;
	int open;
}tablaGlobalFS;

typedef struct __attribute__((__packed__)) {
	int FD;
	t_banderas* flags;
	int globalFD;
	int pid;
	int cursor;
}tablaArchivosFS;

typedef struct __attribute__((__packed__)) {
	int pid;
	int cantidadWait;
	int cantidadSignal;
	int cantidadSyscall; //EN CASO DE TENER QUE SABER CUANDO DE CADA UNA OTRA ESTRUCTURA APARTE <- NO!! PONGAMOSLE INTs A LA ESTRUCTURA
}registroSyscall;

typedef struct __attribute__((__packed__)) {
	int valorSemaforo;
	char* nombreSemaforo;
}semaforo;

typedef struct __attribute__((__packed__)) {
	int valorVariableCompartida;
	char* nombreVariableCompartida;
}variableCompartida;

typedef struct __attribute__((__packed__)) {
	PCB* unPCB;
	char* nombreSemaforoBloqueante;
}pcbBloqueado;

//-------------------------------------------VARAIBLES GLOBALES------------------------------

//LOGGER
t_log* loggerKernel;

//VARIABLES GLOBALES
int pidActual;
int socketMemoria;
int socketFS;
int tamanioPagina;
bool estadoPlanificacionesSistema;

//COLAS
t_queue* colaBloqueado;
t_queue* colaEjecutando;
t_queue* colaFinalizado;
t_queue* colaListo;
t_queue* colaNuevo;
t_queue* listaDeCPULibres;
t_queue* colaProcesos;
t_list* finalizadosPorConsola;
t_list* listaProgramas;

//TABLAS FS
t_list* tablaAdminArchivos;
t_list* tablaAdminGlobal;
t_list* registroDeSyscall;

//SEMAFOROS
t_list* listaSemaforos;

//VARIABLES COMPARTIDAS
t_list* listaVariablesCompartidas;


//VARIABLES DE CONFIGURACION DEL KERNEL
int PUERTO_PROG;
int PUERTO_CPU;
char* IP_FS;
int PUERTO_MEMORIA;
char* IP_MEMORIA;
int PUERTO_FS;
int QUANTUM;
int QUANTUM_SLEEP;
char* ALGORITMO;
int GRADO_MULTIPROG;
char** SEM_IDS;
char** SEM_INIT;
char** SHARED_VARS;
int STACK_SIZE;

//--------------------------------------------CONFIGURACION INICIAL DEL KERNEL---------------------------------
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
void crearKernel(t_config *configuracion) {
	verificarParametrosCrear(configuracion, 14);
	PUERTO_PROG = config_get_int_value(configuracion, "PUERTO_PROG");
	PUERTO_CPU = config_get_int_value(configuracion, "PUERTO_CPU");
	IP_FS = string_new();
	string_append(&IP_FS, config_get_string_value(configuracion,"IP_FS"));
	PUERTO_MEMORIA = config_get_int_value(configuracion, "PUERTO_MEMORIA");
	IP_MEMORIA = string_new();
	string_append(&IP_MEMORIA, config_get_string_value(configuracion,"IP_MEMORIA"));
	PUERTO_FS = config_get_int_value(configuracion, "PUERTO_FS");
	QUANTUM = config_get_int_value(configuracion, "QUANTUM");
	QUANTUM_SLEEP = config_get_int_value(configuracion, "QUANTUM_SLEEP");
	ALGORITMO = string_new();
	string_append(&ALGORITMO, config_get_string_value(configuracion,"ALGORITMO"));
	GRADO_MULTIPROG = config_get_int_value(configuracion,"GRADO_MULTIPROG");
	SEM_IDS = config_get_array_value(configuracion, "SEM_IDS");
	SEM_INIT = config_get_array_value(configuracion, "SEM_INIT");
	SHARED_VARS = config_get_array_value(configuracion, "SHARED_VARS");
	STACK_SIZE = config_get_int_value(configuracion, "STACK_SIZE");
}

void inicializarKernel(char *path) {
	t_config *configuracionKernel =  generarT_ConfigParaCargar(path);
	crearKernel(configuracionKernel);
	config_destroy(configuracionKernel);
}
void mostrarConfiguracionesKernel() {
	printf("PUERTO_PROG=%d\n",PUERTO_PROG);
	printf("PUERTO_CPU=%d\n",PUERTO_CPU);
	printf("IP_FS=%s\n", IP_FS);
	printf("PUERTO_FS=%d\n",PUERTO_FS);
	printf("IP_MEMORIA=%s\n",IP_MEMORIA);
	printf("PUERTO_MEMORIA=%d\n",PUERTO_MEMORIA);
	printf("QUANTUM=%d\n", QUANTUM);
	printf("QUANTUM_SLEEP=%d\n", QUANTUM_SLEEP);
	printf("ALGORITMO=%s\n", ALGORITMO);
	printf("GRADO_MULTIPROG=%d\n", GRADO_MULTIPROG);
	printf("SEM_IDS=");
	imprimirArrayDeChar(SEM_IDS);
	printf("SEM_INIT=");
	imprimirArrayDeInt(SEM_INIT);
	printf("SHARED_VARS=");
	imprimirArrayDeChar(SHARED_VARS);
	printf("STACK_SIZE=%d\n", STACK_SIZE);
}

bool finalizarProceso(int pid, int exitCode){
  bool esPCBProceso(PCB *pcbABuscar){
    return pcbABuscar->pid == pid;
  }

  PCB *pcbAFinalizar = list_find(colaProcesos->elements, (void*)esPCBProceso);
  if(pcbAFinalizar != NULL){
    switch (pcbAFinalizar->estado) {
      case NUEVO:
        list_remove_and_destroy_by_condition(colaNuevo->elements, (void*)esPCBProceso, free);
        log_info(loggerKernel, "El proceso %d se encuentra en la cola de nuevos...", pid);
        log_info(loggerKernel, "El proceso %d fue removido de la cola de nuevos...", pid);
        break;
      case LISTO:
        list_remove_and_destroy_by_condition(colaListo->elements, (void*)esPCBProceso, free);
        log_info(loggerKernel, "El proceso %d se encuentra en la cola de listos...", pid);
        log_info(loggerKernel, "El proceso %d fue removido de la cola de listos...", pid);
        break;
      case EJECUTANDO:
         //ACA HABRIA QUE ESPERAR A QUE CPU LO DEVUELVA
        log_info(loggerKernel, "El proceso %d se encuentra en la cola de ejecutando...", pid);
        log_info(loggerKernel, "El proceso %d fue removido de la cola de ejecutando...", pid);
        break;
      case BLOQUEADO:
        list_remove_and_destroy_by_condition(colaBloqueado->elements, (void*)esPCBProceso, free);
        log_info(loggerKernel, "El proceso %d se encuentra en la cola de bloqueados...", pid);
        log_info(loggerKernel, "El proceso %d fue removido de la cola de bloqueados...", pid);
        break;
      default:
        log_info(loggerKernel, "El proceso %d no se encuentra en ninguna de las colas activas, el proceso ya ha sido finalizado...", pid);
        break;
    }
    pcbAFinalizar->estado = FINALIZADO;
    pcbAFinalizar->exitCode = exitCode;
    return true;
  }else{
    puts("El proceso %d no existe, por lo tanto no se puede finalizar...");
    log_info(loggerKernel, "Se pidio la finalizacion de un proceso que no existe...");
    return false;
  }
}

//SEMAFOROS
void crearSemaforos(char** semIds, char** semInit)
{
	int i;
	for (i = 0; semIds[i] != NULL; i++) {
		semaforo* unSemaforo = malloc(sizeof(semaforo));
		unSemaforo->nombreSemaforo = semIds[i];
		unSemaforo->valorSemaforo = atoi(semInit[i]);
		list_add(listaSemaforos, unSemaforo);
	}
}

PCB* obtenerPCBPrograma(int pidAComparar,t_list* unaLista){
	int i;
	for(i=0;i<list_size(unaLista);i++){
		PCB* unPCB = list_get(unaLista,i);
		if(unPCB->pid==pidAComparar){
			return list_remove(unaLista,i);
		}
	}

	return NULL;
}

void waitSemaforo(void* mensaje,int socketDeCPU){
	int pid, tamanio, exitCode;
	pcbBloqueado* unPCBBloqueado;
	registroSyscall* unRegistro;
	PCB* unPCBBuscado;
	semaforo* semaforoBuscado;
	char* nombreSemaforo;

	memcpy(&pid,mensaje,sizeof(int));
	memcpy(&tamanio,mensaje+sizeof(int),sizeof(int));
	nombreSemaforo = malloc(tamanio);
	memcpy(nombreSemaforo,mensaje+sizeof(int)*2,tamanio);

	log_info(loggerKernel, "Se recibio un Wait de CPU, semaforo %s",nombreSemaforo);

	bool _obtenerSemaforo(semaforo* unSemaforo){
		return (strcmp(unSemaforo->nombreSemaforo, nombreSemaforo)==0);
	}

	bool _obtenerRegistro(registroSyscall* unRegSyscall){
		return (unRegSyscall->pid) == pid;
	}

	semaforoBuscado = list_find(listaSemaforos,(void*) _obtenerSemaforo);

	if(semaforoBuscado!=NULL){
		if(semaforoBuscado->valorSemaforo>0){
			log_info(loggerKernel,"El valor del semaforo: %s es mayor que cero y no se bloquea",nombreSemaforo);
			sendDeNotificacion(socketDeCPU,NO_BLOQUEO_POR_SEMAFORO);
		}
		else{
			log_info(loggerKernel,"El semaforo: %s se bloquea",nombreSemaforo);
			unPCBBuscado = obtenerPCBPrograma(pid,colaEjecutando->elements);

			if(unPCBBuscado!=NULL){
				unPCBBloqueado = malloc(sizeof(pcbBloqueado));
				unPCBBloqueado->nombreSemaforoBloqueante = string_new();
				string_append(&unPCBBloqueado->nombreSemaforoBloqueante,nombreSemaforo);
				unPCBBloqueado->unPCB = unPCBBuscado;
				unPCBBloqueado->unPCB->estado = BLOQUEADO;

				queue_push(colaBloqueado,unPCBBloqueado);

				log_info(loggerKernel,"El programa con el PID: %d ha entrado a la cola de bloqueados",pid);

				unRegistro = list_find(registroDeSyscall,(void*) _obtenerRegistro);
				unRegistro->cantidadSyscall++;
				unRegistro->cantidadWait++;

				sendDeNotificacion(socketDeCPU,BLOQUEO_POR_SEMAFORO);
			}
			else{
				log_error(loggerKernel,"No se encontro el PCB con el siguiente PID: %d",pid);
			}
		}
		semaforoBuscado->valorSemaforo--;
	}
	else{
		log_error(loggerKernel,"Semaforo: %s inexistente solicitado para operacion Wait... se finaliza programa",nombreSemaforo);
		exitCode = -10; //Exit Code para semaforos inexistentes
		finalizarProceso(pid, exitCode);
	}
}

void signalSemaforo(void* mensaje)
{
	int pid, tamanio, i, cantidadElementos, exitCode;
	bool encontrado = false;
	pcbBloqueado* unPCBBloqueado;
	registroSyscall* unRegistro;
	t_queue* auxiliar;
	semaforo* semaforoBuscado;
	PCB* pcbBuscado;
	char* nombreSemaforo;

	memcpy(&pid,mensaje,sizeof(int));
	memcpy(&tamanio,mensaje+sizeof(int),sizeof(int));
	nombreSemaforo = malloc(tamanio);
	memcpy(nombreSemaforo,mensaje+sizeof(int)*2,tamanio);

	log_info(loggerKernel, "Se recibio un Signal de CPU, semaforo %s",nombreSemaforo);

	bool _obtenerSemaforo(semaforo* unSemaforo){
		return (strcmp(unSemaforo->nombreSemaforo, nombreSemaforo)==0);
	}

	bool _obtenerPCB(PCB* unPCB){
		return (unPCB->pid) == unPCBBloqueado->unPCB->pid;
	}

	bool _obtenerRegistro(registroSyscall* unRegSyscall){
		return (unRegSyscall->pid) == unPCBBloqueado->unPCB->pid;
	}

	semaforoBuscado = list_find(listaSemaforos,(void*) _obtenerSemaforo);

	if(semaforoBuscado!=NULL){
		if(semaforoBuscado->valorSemaforo<0){
			auxiliar = colaBloqueado;
			cantidadElementos = queue_size(colaBloqueado);
			for(i=0;i<cantidadElementos;i++){
				unPCBBloqueado = queue_pop(colaBloqueado);
				if(strcmp(unPCBBloqueado->nombreSemaforoBloqueante,nombreSemaforo)==0 && encontrado==false){
					encontrado = true;
					pcbBuscado = list_find(colaProcesos->elements,(void*) _obtenerPCB);
					pcbBuscado->estado = LISTO;

					if (pcbBuscado->tablaHeap!=NULL){
						unPCBBloqueado->unPCB->tablaHeap = pcbBuscado->tablaHeap;
					}

					queue_push(colaListo,pcbBuscado);

					log_info(loggerKernel,"El programa con PID: %d salio de cola bloqueados e ingreso a listos por un signal",pid);

					unRegistro = list_find(registroDeSyscall,(void*) _obtenerRegistro);
					unRegistro->cantidadSyscall++;
					unRegistro->cantidadSignal++;

				}
				else{
					queue_push(auxiliar,unPCBBloqueado);
				}
			}
			colaBloqueado = auxiliar;
		}
	semaforoBuscado->valorSemaforo++;
	}
	else{
		log_error(loggerKernel,"El programa con PID: %d envio un semaforo: %s inexistente.. se finalizo el programa",pid,nombreSemaforo);
		exitCode = -10; //Exit Code para semaforos inexistentes
		finalizarProceso(pid, exitCode);
	}
}

//VARIABLES COMPARTIDAS
void crearVariablesCompartidas(char** sharedVars)
{
	int i;
	for (i = 0; sharedVars[i] != NULL; i++) {
		variableCompartida* unaVariableCompartida = malloc(sizeof(variableCompartida));
		unaVariableCompartida->nombreVariableCompartida = sharedVars[i];
		unaVariableCompartida->valorVariableCompartida = 0;
		list_add(listaVariablesCompartidas, unaVariableCompartida);
	}
}

variableCompartida* obtenerVariableCompartida(char* unaVariable)
{
	int i;
	for(i = 0;i<list_size(listaVariablesCompartidas);i++){
		variableCompartida* unaVariableDeLaLista = list_get(listaVariablesCompartidas,i);
		if(strcmp(unaVariableDeLaLista->nombreVariableCompartida,unaVariable)==0)
			return unaVariableDeLaLista;
	}
	return NULL;
}

void obtenerValorVariableCompartida(void* mensaje,int socketCPU){
	int pid, tamanio, exitCode;
	char* nombreVariableCompartida;

	memcpy(&pid,mensaje,sizeof(int));
	memcpy(&tamanio,mensaje+sizeof(int),sizeof(int));
	nombreVariableCompartida = malloc(tamanio);
	memcpy(nombreVariableCompartida,mensaje+sizeof(int)*2,tamanio);

	variableCompartida* variableCompartidaBuscada = obtenerVariableCompartida(nombreVariableCompartida);

	if(variableCompartidaBuscada!=NULL){
		log_info(loggerKernel,"Se obtuvo la variable compartida asociada al PID: %d",pid);
		sendRemasterizado(socketCPU,DATOS_VARIABLE_COMPARTIDA_KERNEL,sizeof(int),&variableCompartidaBuscada->valorVariableCompartida);
	}
	else{
		log_error(loggerKernel,"Variable Compartida recibida inexistente, se finaliza el programa con PID: %d",pid);
		exitCode = -11; //ExitCode = -11 para errores con Variables Compartidas
		finalizarProceso(pid,exitCode);
		sendRemasterizado(socketCPU,OPERACION_VARIABLE_COMPARTIDA_FALLIDA,0,NULL);
	}
}

void asignarValorVariableCompartida(void* mensaje,int socketCPU){
	int pid, tamanio, exitCode, valorNuevo;
	char* nombreVariableCompartida;

	memcpy(&pid,mensaje,sizeof(int));
	memcpy(&tamanio,mensaje+sizeof(int),sizeof(int));
	nombreVariableCompartida = malloc(tamanio);
	memcpy(nombreVariableCompartida,mensaje+sizeof(int)*2,tamanio);
	memcpy(&valorNuevo,mensaje+sizeof(int)*2+tamanio,sizeof(int));

	variableCompartida* variableCompartidaBuscada = obtenerVariableCompartida(nombreVariableCompartida);

	if(variableCompartidaBuscada!=NULL){
		log_info(loggerKernel,"Se obtuvo la variable compartida asociada al PID: %d",pid);
		variableCompartidaBuscada->valorVariableCompartida = valorNuevo;
		sendRemasterizado(socketCPU,DATOS_VARIABLE_COMPARTIDA_KERNEL,sizeof(int),&variableCompartidaBuscada->valorVariableCompartida);
	}
	else{
		log_error(loggerKernel,"Variable Compartida recibida inexistente, se finaliza el programa con PID: %d",pid);
		exitCode = -11; //ExitCode = -11 para errores con Variables Compartidas
		finalizarProceso(pid,exitCode);
		sendRemasterizado(socketCPU,OPERACION_VARIABLE_COMPARTIDA_FALLIDA,0,NULL);
	}
}


//------------------------------------------ FUNCIONES AUXILIARES PARA PCB------------------------------------
t_intructions* cargarCodeIndex(char* buffer,t_metadata_program* metadata_program) {
	t_intructions* code = malloc((sizeof(int)*2)*metadata_program->instrucciones_size);
	int i = 0;

	for (; i < metadata_program->instrucciones_size; i++) {
		log_info(loggerKernel, "Instruccion inicio:%d offset:%d %.*s",metadata_program->instrucciones_serializado[i].start,metadata_program->instrucciones_serializado[i].offset,metadata_program->instrucciones_serializado[i].offset,buffer+ metadata_program->instrucciones_serializado[i].start);
		code[i].start = metadata_program->instrucciones_serializado[i].start;
		code[i].offset = metadata_program->instrucciones_serializado[i].offset;
	}

	return code;
}

void inicializarStack(PCB *pcbNuevo){
  pcbNuevo->indiceStack = list_create();
  stack *primerStack = malloc(sizeof(stack));
  primerStack->args = list_create();
  primerStack->vars = list_create();
  primerStack->posicion = 0;
  list_add(pcbNuevo->indiceStack, primerStack);
}

char* cargarEtiquetas(t_metadata_program* metadata_program,int sizeEtiquetasIndex) {
	char* etiquetas = malloc(sizeEtiquetasIndex * sizeof(char));
	if(metadata_program->cantidad_de_etiquetas>0 || metadata_program->cantidad_de_funciones>0){
		memcpy(etiquetas, metadata_program->etiquetas,sizeEtiquetasIndex * sizeof(char));
		return etiquetas;
	}
	return NULL;
}

int obtenerCantidadPaginas(char* codigoDePrograma){
	int tamanioCodigo = string_length(codigoDePrograma);
  	int cantidadPaginas;

    if((tamanioCodigo % tamanioPagina) != 0){
    	cantidadPaginas = (tamanioCodigo/tamanioPagina)+STACK_SIZE+1; //CREO QUE ES ASI, NO SE RESTA EL SIZEOF(HEAPMETADATA)*2 ¡¡VER!!
    }
    else{
    	cantidadPaginas = (tamanioCodigo/tamanioPagina)+STACK_SIZE;	  //CREO QUE ES ASI, NO SE RESTA EL SIZEOF(HEAPMETADATA)*2 ¡¡VER!!
    }

    return cantidadPaginas;
}

int mandarInicioAMemoria(void* inicioDePrograma){
  sendRemasterizado(socketMemoria, INICIAR_PROGRAMA, sizeof(int)*2, inicioDePrograma);
  return recvDeNotificacion(socketMemoria);
}

//----------------------------------------PCB----------------------------------------------//
void iniciarPCB(char *codigoDePrograma, int socketConsolaDuenio)
{
	PCB *pcbNuevo = malloc(sizeof(PCB));
	datosPrograma* programa = malloc(sizeof(datosPrograma));
	t_metadata_program *metadataDelPrograma = metadata_desde_literal(codigoDePrograma);
	registroSyscall* unRegistro = malloc(sizeof(registroSyscall));

	pcbNuevo->pid = pidActual;
	pcbNuevo->estado = NUEVO;
	pcbNuevo->programCounter = metadataDelPrograma->instruccion_inicio;
	pcbNuevo->rafagas = 0;
	pcbNuevo->exitCode = 0;
	pcbNuevo->cantidadPaginasCodigo = ceil((double)string_length(codigoDePrograma)/(double)tamanioPagina);
	pcbNuevo->cantidadTIntructions = metadataDelPrograma->instrucciones_size;
	pcbNuevo->indiceCodigo = cargarCodeIndex(codigoDePrograma, metadataDelPrograma);
	pcbNuevo->tamanioEtiquetas = metadataDelPrograma->etiquetas_size;
	pcbNuevo->indiceEtiquetas = cargarEtiquetas(metadataDelPrograma, pcbNuevo->tamanioEtiquetas);
	pcbNuevo->posicionStackActual = 0;
	inicializarStack(pcbNuevo);
	pcbNuevo->tamanioContexto = 1;
	pcbNuevo->socketConsola = socketConsolaDuenio;
	pcbNuevo->estaAbortado = false;
	pcbNuevo->tablaHeap = malloc(sizeof(tablaHeap));
	pcbNuevo->tablaHeap->pagina = list_create();

	programa->pid = pidActual;
	programa->script = string_new();
	string_append(&programa->script,codigoDePrograma);

	unRegistro->cantidadSignal = 0;
	unRegistro->cantidadWait = 0;
	unRegistro->cantidadSyscall = 0;
	unRegistro->pid = pidActual;

	list_add(registroDeSyscall,unRegistro);

	metadata_destruir(metadataDelPrograma);
	pidActual++;

	log_info(loggerKernel, "PCB creada para el pid: %d ",pcbNuevo->pid);

	list_add(listaProgramas,programa);
	queue_push(colaProcesos,pcbNuevo);
	queue_push(colaNuevo,pcbNuevo);

	log_info(loggerKernel, "El pid: %d ha ingresado a la cola de nuevo.",pcbNuevo->pid);
}

//-----------------------------------DESERIALIZAR PCB------------------------------------//
void desserializarVariables(t_list *variables, paquete *paqueteConVariables, int* dondeEstoy){
    int cantidadDeVariables;
    memcpy(&cantidadDeVariables, paqueteConVariables->mensaje+*dondeEstoy, sizeof(int));
    *dondeEstoy += sizeof(int);
    int i;
    for(i = 0; i<cantidadDeVariables; i++){
        variable *variable = malloc(sizeof(variable));
        memcpy(&variable->nombreVariable, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(char));
        *dondeEstoy += sizeof(char);
        variable->direccionDeVariable = malloc(sizeof(direccion));
        memcpy(&variable->direccionDeVariable->pagina, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        memcpy(&variable->direccionDeVariable->offset, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        memcpy(&variable->direccionDeVariable->tamanio, paqueteConVariables->mensaje+(*dondeEstoy), sizeof(int));
        *dondeEstoy += sizeof(int);
        list_add(variables, variable);
    }
}

void desserializarContexto(PCB* pcbConContexto, paquete* paqueteConContexto, int dondeEstoy){
    int nivelDeContexto;
    for(nivelDeContexto = 0; nivelDeContexto<pcbConContexto->tamanioContexto; nivelDeContexto++){
        stack *contextoAAgregarAPCB = malloc(sizeof(stack));
        contextoAAgregarAPCB->args = list_create();
        contextoAAgregarAPCB->vars = list_create();
        memcpy(&contextoAAgregarAPCB->posicion, paqueteConContexto->mensaje+dondeEstoy, sizeof(int));
        dondeEstoy +=sizeof(int);
        desserializarVariables(contextoAAgregarAPCB->args, paqueteConContexto, &dondeEstoy);
        desserializarVariables(contextoAAgregarAPCB->vars, paqueteConContexto, &dondeEstoy);
        memcpy(&contextoAAgregarAPCB->retPos, paqueteConContexto->mensaje + dondeEstoy, sizeof(int));
        memcpy(&contextoAAgregarAPCB->retVar, paqueteConContexto->mensaje + dondeEstoy + sizeof(int), sizeof(direccion));
        list_add(pcbConContexto->indiceStack, contextoAAgregarAPCB);
        dondeEstoy += sizeof(int);
    }
}

int desserializarTIntructions(PCB* pcbDesserializada, paquete *paqueteConPCB){
    int i, auxiliar = 0;
    memcpy(&pcbDesserializada->cantidadTIntructions, paqueteConPCB->mensaje+(sizeof(int)*3), sizeof(int));
    pcbDesserializada->indiceCodigo = malloc(pcbDesserializada->cantidadTIntructions*sizeof(t_intructions));
    for(i = 0; i<pcbDesserializada->cantidadTIntructions; i++){
        memcpy(&pcbDesserializada->indiceCodigo[i].start, paqueteConPCB->mensaje+sizeof(int)*(4+auxiliar), sizeof(int));
        auxiliar++;
        memcpy(&pcbDesserializada->indiceCodigo[i].offset, paqueteConPCB->mensaje+sizeof(int)*(4+auxiliar), sizeof(int));
        auxiliar++;
    }
    int retorno = sizeof(int)*(4+auxiliar);
    return retorno;
}

PCB* deserializarPCB(paquete* paqueteConPCB){
    PCB* pcbDesserializada = malloc(paqueteConPCB->tamMsj);
    memcpy(&pcbDesserializada->pid, paqueteConPCB->mensaje, sizeof(int));
    memcpy(&pcbDesserializada->programCounter, paqueteConPCB->mensaje+sizeof(int), sizeof(int));
    memcpy(&pcbDesserializada->cantidadPaginasCodigo, paqueteConPCB->mensaje+sizeof(int)*2, sizeof(int));
    int dondeEstoy = desserializarTIntructions(pcbDesserializada, paqueteConPCB);
    memcpy(&pcbDesserializada->tamanioEtiquetas, paqueteConPCB->mensaje+dondeEstoy, sizeof(int));
    pcbDesserializada->indiceEtiquetas = string_new();//etiquetas
    if(pcbDesserializada->tamanioEtiquetas!=0){
    	 memcpy(pcbDesserializada->indiceEtiquetas, paqueteConPCB->mensaje+sizeof(int)+dondeEstoy, pcbDesserializada->tamanioEtiquetas);
    }
    memcpy(&pcbDesserializada->rafagas,paqueteConPCB->mensaje+sizeof(int)+dondeEstoy+pcbDesserializada->tamanioEtiquetas,sizeof(int));
    memcpy(&pcbDesserializada->posicionStackActual, paqueteConPCB->mensaje+dondeEstoy+sizeof(int)*2+pcbDesserializada->tamanioEtiquetas, sizeof(int));
    memcpy(&pcbDesserializada->tamanioContexto, paqueteConPCB->mensaje+sizeof(int)*3+dondeEstoy+pcbDesserializada->tamanioEtiquetas, sizeof(int));

    dondeEstoy +=sizeof(int)*4+pcbDesserializada->tamanioEtiquetas;
    pcbDesserializada->indiceStack = list_create();
    desserializarContexto(pcbDesserializada, paqueteConPCB, dondeEstoy);
    return pcbDesserializada;
}

//------------------------------------SERIALIZAR PCB--------------------------------------//
void *serializarT_Intructions(PCB *pcbConT_intruction){
  void *serializacion = malloc(sizeof(t_intructions)*pcbConT_intruction->cantidadTIntructions);
  int i, auxiliar1 = 0;
  for(i = 0; i<pcbConT_intruction->cantidadTIntructions; i++){
    memcpy(serializacion+sizeof(int)*auxiliar1, &pcbConT_intruction->indiceCodigo[i].start, sizeof(int));
    auxiliar1++;
    memcpy(serializacion+sizeof(int)*auxiliar1,&pcbConT_intruction->indiceCodigo[i].offset, sizeof(int));
    auxiliar1++;
  }
  return serializacion;
}

void* serializarVariable(t_list* variables){
    void* variableSerializada = malloc(list_size(variables)*(sizeof(char)+sizeof(direccion))+sizeof(int));
    int i, dondeEstoy = 0;
    int cantidadDeVariables = list_size(variables);
    memcpy(variableSerializada, &cantidadDeVariables, sizeof(int));
    dondeEstoy += sizeof(int);
    for(i = 0; i<cantidadDeVariables; i++){ //Antes i=1
        variable *variableObtenida = list_get(variables, i);
        memcpy(variableSerializada+dondeEstoy, &variableObtenida->nombreVariable, sizeof(char));
        dondeEstoy += sizeof(char);
        memcpy(variableSerializada+dondeEstoy, &variableObtenida->direccionDeVariable->pagina, sizeof(int));
        dondeEstoy += sizeof(int);
        memcpy(variableSerializada+dondeEstoy, &variableObtenida->direccionDeVariable->offset, sizeof(int));
        dondeEstoy += sizeof(int);
        memcpy(variableSerializada+dondeEstoy, &variableObtenida->direccionDeVariable->tamanio, sizeof(int));
        dondeEstoy += sizeof(int);
    }
    return variableSerializada;
}

int sacarTamanioDeLista(t_list* contexto){
    int i, tamanioDeContexto = 0;
    for(i = 0; i<list_size(contexto); i++){
        stack *contextoAObtener = list_get(contexto, i);
        tamanioDeContexto += sizeof(int)*4+sizeof(direccion)+(list_size(contextoAObtener->args)+list_size(contextoAObtener->vars))*(sizeof(char)+sizeof(direccion));
        //free(contextoAObtener);
    }
    return tamanioDeContexto;
}

void *serializarStack(PCB* pcbConContextos){
    void* contextoSerializado = malloc(sacarTamanioDeLista(pcbConContextos->indiceStack));
  int i, dondeEstoy = 0;
  for(i = 0; i<pcbConContextos->tamanioContexto; i++){
    stack* contexto = list_get(pcbConContextos->indiceStack, i);
    memcpy(contextoSerializado+dondeEstoy, &contexto->posicion, sizeof(int));
    dondeEstoy += sizeof(int);
    void *argsSerializadas = serializarVariable(contexto->args);
    void *varsSerializadas = serializarVariable(contexto->vars);
    memcpy(contextoSerializado+dondeEstoy, argsSerializadas, list_size(contexto->args)*(sizeof(char)+sizeof(direccion))+sizeof(int));
    dondeEstoy += ((list_size(contexto->args)*(sizeof(char)+sizeof(direccion)))+sizeof(int));
    memcpy(contextoSerializado+dondeEstoy, varsSerializadas, list_size(contexto->vars)*(sizeof(char)+sizeof(direccion))+sizeof(int));
    dondeEstoy += ((list_size(contexto->vars)*(sizeof(char)+sizeof(direccion)))+sizeof(int));
    memcpy(contextoSerializado+dondeEstoy, &contexto->retPos, sizeof(int));
    dondeEstoy += sizeof(direccion);
    free(argsSerializadas);
    free(varsSerializadas);
  }
  return contextoSerializado;
}

int sacarTamanioPCB(PCB* pcbASerializar){
    int tamaniosDeContexto = sacarTamanioDeLista(pcbASerializar->indiceStack);
    int tamanio= sizeof(int)*8+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+tamaniosDeContexto+pcbASerializar->tamanioEtiquetas;
    return tamanio;
}

void* serializarPCB(PCB* pcbASerializar){
    int tamanioPCB = sacarTamanioPCB(pcbASerializar);
    void* pcbSerializada = malloc(tamanioPCB);
    memcpy(pcbSerializada, &pcbASerializar->pid, sizeof(int));
    memcpy(pcbSerializada+sizeof(int), &pcbASerializar->programCounter, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*2, &pcbASerializar->cantidadPaginasCodigo, sizeof(int));
    void* indiceDeCodigoSerializado = serializarT_Intructions(pcbASerializar);
    memcpy(pcbSerializada+sizeof(int)*3, &pcbASerializar->cantidadTIntructions, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*4, indiceDeCodigoSerializado, sizeof(t_intructions)*pcbASerializar->cantidadTIntructions);
    memcpy(pcbSerializada+sizeof(int)*4+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions, &pcbASerializar->tamanioEtiquetas, sizeof(int));
    if(pcbASerializar->tamanioEtiquetas!=0){
    	memcpy(pcbSerializada+ sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions, pcbASerializar->indiceEtiquetas, pcbASerializar->tamanioEtiquetas); //CHEQUEAR QUE ES TAMETIQUETAS
    }
    memcpy(pcbSerializada+sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->rafagas, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*6+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->posicionStackActual, sizeof(int));
    void* stackSerializado = serializarStack(pcbASerializar);
    memcpy(pcbSerializada+sizeof(int)*7+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->tamanioContexto, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*8+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, stackSerializado, sacarTamanioDeLista(pcbASerializar->indiceStack));
    free(indiceDeCodigoSerializado);
    free(stackSerializado);
    return pcbSerializada;
}



//--------------ADMINISTRACION DE PROGRAMAS---------------//
void enviarCodigoDeProgramaAMemoria(int pid, char* codigo){
	void *codigoParaMemoria;
	int i;
	int offset = 0;
	int tamanioDeCodigoRestante = string_length(codigo);
	int cantidadDePaginasQueOcupa = tamanioDeCodigoRestante/tamanioPagina;
	if((tamanioDeCodigoRestante % tamanioPagina) != 0){
		cantidadDePaginasQueOcupa++;
	}
	for(i = 0; i<cantidadDePaginasQueOcupa; i++){
		int tamanioMensaje;
		codigoParaMemoria = malloc(tamanioDeCodigoRestante+sizeof(int)*4);
		memcpy(codigoParaMemoria, &pid, sizeof(int));
		memcpy(codigoParaMemoria+sizeof(int), &i, sizeof(int));
		memcpy(codigoParaMemoria+sizeof(int)*2, &offset, sizeof(int));
		if(tamanioDeCodigoRestante>tamanioPagina){
			memcpy(codigoParaMemoria+sizeof(int)*3, &tamanioPagina, sizeof(int));
			memcpy(codigoParaMemoria+sizeof(int)*4, codigo+(i*tamanioPagina), tamanioPagina);
			tamanioMensaje = tamanioPagina;
		}else{
			memcpy(codigoParaMemoria+sizeof(int)*3, &tamanioDeCodigoRestante, sizeof(int));
			memcpy(codigoParaMemoria+sizeof(int)*4, codigo+(i*tamanioPagina), tamanioDeCodigoRestante);
			tamanioMensaje = tamanioDeCodigoRestante;
		}
		sendRemasterizado(socketMemoria, ESCRIBIR_DATOS, tamanioMensaje+sizeof(int)*4, codigoParaMemoria);
		free(codigoParaMemoria);
		tamanioDeCodigoRestante -= tamanioPagina;
	}
}

void iniciarProceso(paquete* paqueteConCodigo, int socketConsola){
  char* codigo = string_new();
  string_append(&codigo, paqueteConCodigo->mensaje);
  iniciarPCB(codigo, socketConsola);
  free(codigo);
}

void recibirPCBDeCPU(paquete *paqueteConPCB){
  PCB *pcbRecibida = deserializarPCB(paqueteConPCB);
  bool esDelProceso(int pid){
	  return pid == pcbRecibida->pid;
  }
  if(list_any_satisfy(finalizadosPorConsola, (void*)esDelProceso)){
    list_remove_and_destroy_by_condition(colaListo->elements, (void*)esDelProceso, free); //EN CASO DE NO FUNCIONAR LA FUNCION, AGREGAR ESDEPROCESO
    pcbRecibida->estado = FINALIZADO;
    pcbRecibida->exitCode = -7;
  }
}

int cantidadDeRafagasDe(int pid){
  bool esPCBProceso(PCB *pcbABuscar){
    return pcbABuscar->pid == pid;
  }
  PCB *pcbConRafagas = list_find(colaProcesos->elements, (void*)esPCBProceso); //REVISAR SI HACE FALTA LIBERARLO
  if(pcbConRafagas != NULL){
      log_info(loggerKernel, "Se encontro el proceso %d en la lista de PCBs activas...", pid);
  }
  else if(pcbConRafagas == NULL){
    pcbConRafagas = list_find(colaFinalizado->elements, (void*)esPCBProceso);
    log_info(loggerKernel, "Se encontro el proceso %d en la lista de PCBs finalizadas...", pid);
  }else{
    log_info(loggerKernel, "No se encontro el proceso %d en ninguna de las listas...", pid);
    return -1;
  }
  return pcbConRafagas->rafagas;
}

void finalizarProcesoEnviadoDeConsola(paquete* paqueteConProceso, int socketConsola){
	int pidFinalizado = *(int*)paqueteConProceso->mensaje;
	int exitCode = -6;
	if(finalizarProceso(pidFinalizado,exitCode)){
		char* mensajeFinalizado = string_new();
		string_append(&mensajeFinalizado,"Se finalizo correctamente el proceso");
		int tamanio = sizeof(char)*strlen(mensajeFinalizado);
		void* mensaje = malloc(sizeof(int)*2+tamanio);
		memcpy(mensaje,&pidFinalizado,sizeof(int));
		memcpy(mensaje+sizeof(int),&tamanio,sizeof(int));
		memcpy(mensaje+sizeof(int)*2,mensajeFinalizado,tamanio);
		sendRemasterizado(socketConsola,FINALIZAR_PROCESO,sizeof(int)*2+tamanio,mensaje);
		free(mensajeFinalizado);
		free(mensaje);
	}
	else{
		char* mensaje = string_new();
		string_append(&mensaje,"El programa con el PID indicado no existe, inconsistencia de datos de consola.");
		int tamanio = strlen(mensaje);
		void* mensajeConTamanio = malloc(sizeof(int)+tamanio);
		memcpy(mensajeConTamanio,&tamanio,sizeof(int));
		memcpy(mensajeConTamanio+sizeof(int),mensaje,tamanio);
		sendRemasterizado(socketConsola,FINALIZO_INCORRECTAMENTE,tamanio+sizeof(int),mensaje);
		free(mensaje);
		free(mensajeConTamanio);
	}
}

//-------------------------------------MANEJO DE HEAP----------------------------//
bool estaLibre(bloqueHeapMetadata* bloqueHeapDePagina){
	return bloqueHeapDePagina->heapDeBloque->estaLibre;
}

void liberarHeap(bloqueHeapMetadata *bloqueHeap){
	free(bloqueHeap);
}

void chequearHeapMetadata(PCB *pcbConHeap, int numeroPagina){
	bool esDeLaPagina(paginaHeap *paginaConHeap){
		return paginaConHeap->pagina == numeroPagina;
	}
	paginaHeap *paginaConHeap = list_find(pcbConHeap->tablaHeap->pagina, (void*)esDeLaPagina);
	if(list_all_satisfy(paginaConHeap->bloqueHeapMetadata, (void*)estaLibre)){
		void *paqueteDeLiberacion = malloc(sizeof(int)*2);
		memcpy(paqueteDeLiberacion, &pcbConHeap->pid, sizeof(int));
		memcpy(paqueteDeLiberacion+sizeof(int), &numeroPagina, sizeof(int));
		sendRemasterizado(socketMemoria, LIBERAR_PAGINA, sizeof(int)*2, paqueteDeLiberacion);
		if(recvDeNotificacion(socketMemoria) != OPERACION_EXITOSA){

		}
	}else if(list_count_satisfying(paginaConHeap->bloqueHeapMetadata, (void*)estaLibre)>1){
		bloqueHeapMetadata *bloqueHeapMetadataAnterior;
		int cantidadDeBloquesDeHeap = list_size(paginaConHeap->bloqueHeapMetadata)-1;
		for(; cantidadDeBloquesDeHeap>=0; cantidadDeBloquesDeHeap--){
			bloqueHeapMetadata* bloqueHeapMetadataChequeado = list_get(paginaConHeap->bloqueHeapMetadata, cantidadDeBloquesDeHeap);
			if((cantidadDeBloquesDeHeap!=list_size(paginaConHeap->bloqueHeapMetadata)-1) && bloqueHeapMetadataAnterior->heapDeBloque->estaLibre && bloqueHeapMetadataChequeado->heapDeBloque->estaLibre){
				bloqueHeapMetadataChequeado->heapDeBloque->size += (bloqueHeapMetadataAnterior->heapDeBloque->size+sizeof(heapMetadata));
				list_remove_and_destroy_element(paginaConHeap->bloqueHeapMetadata, cantidadDeBloquesDeHeap+1, (void*)liberarHeap);
			}

		}
	}

}

void avisarFinalizacion(PCB *pcbFinalizada){
	sendRemasterizado(pcbFinalizada->socketConsola, FINALIZAR_PROCESO, sizeof(int), &pcbFinalizada->pid);
	if(recvDeNotificacion(pcbFinalizada->socketConsola) != OPERACION_EXITOSA){

	}
}

void liberarDatosDeProceso(int pid, t_puntero punteroADatos, int socketCPU){
	bool esPCBBuscada(PCB *pcbAChequear){
		return pcbAChequear->pid == pid;
	}
	//punteroADatos -= sizeof(heapMetadata);
	int pagina, offset;
	pagina = punteroADatos/tamanioPagina;
	bool esPaginaBuscada(paginaHeap *paginaAChequear){
		return paginaAChequear->pagina == pagina;
	}
	offset = punteroADatos%tamanioPagina;
	bool esElOffsetBuscado(bloqueHeapMetadata *bloqueAChequear){
		return bloqueAChequear->offset == offset;
	}
	if(offset<(tamanioPagina-sizeof(heapMetadata))){
		PCB *pcbDeConPeticion = list_find(colaProcesos->elements, (void*)esPCBBuscada);
		if(list_any_satisfy(pcbDeConPeticion->tablaHeap->pagina, (void*)esPaginaBuscada)){
			paginaHeap *paginaConLiberacion = list_find(pcbDeConPeticion->tablaHeap->pagina, (void*)esPaginaBuscada);
			bloqueHeapMetadata *bloqueDeHeapBuscado = list_find(paginaConLiberacion->bloqueHeapMetadata, (void*)esElOffsetBuscado);
			bloqueDeHeapBuscado->heapDeBloque->estaLibre = true;
			paginaConLiberacion->tamanioDisponible += bloqueDeHeapBuscado->heapDeBloque->size;
			chequearHeapMetadata(pcbDeConPeticion, pagina);
		}
	}else{
		sendDeNotificacion(socketCPU, OPERACION_CON_CPU_FALLIDA);
	}
}




void pedirEscrituraAMemoria(int pid, paginaHeap *paginaAEscribir, bloqueHeapMetadata *bloqueAEscibir){
	void *mensajeDeEscritura = malloc(sizeof(int)*4+sizeof(heapMetadata));
	int tamanio = sizeof(heapMetadata);
	int offsetDeEscritura = bloqueAEscibir->offset - sizeof(heapMetadata);
	memcpy(mensajeDeEscritura, &pid, sizeof(int));
	memcpy(mensajeDeEscritura+sizeof(int), &paginaAEscribir->pagina, sizeof(int));
	memcpy(mensajeDeEscritura+sizeof(int)*2, &offsetDeEscritura, sizeof(int));
	memcpy(mensajeDeEscritura+sizeof(int)*3, &tamanio, sizeof(int));
	memcpy(mensajeDeEscritura+sizeof(int)*4, bloqueAEscibir->heapDeBloque, tamanio);
	sendRemasterizado(socketMemoria, ESCRIBIR_DATOS, sizeof(int)*4+sizeof(heapMetadata), mensajeDeEscritura);
}


paginaHeap *pedirPaginaHeap(PCB *pcbConNecesidad){
	void *mensajeDePeticion = malloc(sizeof(int)*2);
	int cantidad = 1;
	memcpy(mensajeDePeticion, &pcbConNecesidad->pid, sizeof(int));
	memcpy(mensajeDePeticion+sizeof(int), &cantidad, sizeof(int));
	sendRemasterizado(socketMemoria, ASIGNAR_PAGINAS, sizeof(int)*2, mensajeDePeticion);
	if(recvDeNotificacion(socketMemoria) == OPERACION_EXITOSA){
		int numeroPagina = list_size(pcbConNecesidad->tablaHeap->pagina)-1; //ES POSIBLE QUE EL -1 NO VAYA
		paginaHeap *paginaHeapAAgregar = malloc(sizeof(paginaHeap));
		paginaHeapAAgregar->pagina = numeroPagina;
		paginaHeapAAgregar->tamanioDisponible = tamanioPagina - sizeof(heapMetadata)*2;
		paginaHeapAAgregar->bloqueHeapMetadata = list_create();
		bloqueHeapMetadata *bloqueHeapMetadataDelComienzo = malloc(sizeof(bloqueHeapMetadata));
		bloqueHeapMetadata *bloqueHeapMetadataDelFinal = malloc(sizeof(bloqueHeapMetadata));
		bloqueHeapMetadataDelComienzo->heapDeBloque = malloc(sizeof(heapMetadata));
		bloqueHeapMetadataDelFinal->heapDeBloque = malloc(sizeof(heapMetadata));
		//----CREO EL PRIMER BLOQUE DE HEAP
		bloqueHeapMetadataDelComienzo->heapDeBloque->estaLibre = true;
		bloqueHeapMetadataDelComienzo->heapDeBloque->size = paginaHeapAAgregar->tamanioDisponible;
		bloqueHeapMetadataDelComienzo->offset = numeroPagina*tamanioPagina + sizeof(heapMetadata);
		list_add(paginaHeapAAgregar->bloqueHeapMetadata, bloqueHeapMetadataDelComienzo);
		//-----CREO EL ULTIMO BLOQUE DE HEAP
		bloqueHeapMetadataDelFinal->heapDeBloque->estaLibre = true;
		bloqueHeapMetadataDelFinal->heapDeBloque->size = 0;
		bloqueHeapMetadataDelFinal->offset = tamanioPagina - sizeof(heapMetadata);
		list_add(paginaHeapAAgregar->bloqueHeapMetadata, bloqueHeapMetadataDelFinal);
		pedirEscrituraAMemoria(pcbConNecesidad->pid, paginaHeapAAgregar, bloqueHeapMetadataDelComienzo);
		pedirEscrituraAMemoria(pcbConNecesidad->pid, paginaHeapAAgregar, bloqueHeapMetadataDelFinal);
		//-----AGREGO LA PAGINA CON LOS HEAPS YA CARGADOS A LA TABLA DE HEAP DEL PROCESO
		list_add(pcbConNecesidad->tablaHeap->pagina, paginaHeapAAgregar);
		return paginaHeapAAgregar;
	}else{
		return NULL;
	}
}

bool noTienePaginasHeap(PCB *pcbAChequear, int tamanioAAlocar){
	bool esMenorAlTamanioAAlocar(paginaHeap *paginaAChequear){
		return paginaAChequear->tamanioDisponible<tamanioAAlocar;
	}
	if((list_size(pcbAChequear->tablaHeap->pagina)) == 0){
		return true;
	}else if(list_all_satisfy(pcbAChequear->tablaHeap->pagina, (void*)esMenorAlTamanioAAlocar)){
		return true;
	}
	return false;
}

paginaHeap *buscarPaginaHeapConSuficienteTamanio(PCB *pcbAChequear, int tamanioAAlocar){
	bool tieneBloqueUtil(bloqueHeapMetadata *bloqueHeapMetadata){
		return bloqueHeapMetadata->heapDeBloque->size > tamanioAAlocar;
	}
	bool tieneBloqueConTamanioSuficiente(paginaHeap* paginaAChequear){
		return list_any_satisfy(paginaAChequear->bloqueHeapMetadata, (void*)tieneBloqueUtil);
	}
	paginaHeap *paginaHeapConSuficienteTamanio = list_find(pcbAChequear->tablaHeap->pagina, (void*)tieneBloqueConTamanioSuficiente);
	return paginaHeapConSuficienteTamanio;
}

bloqueHeapMetadata *obtenerBloqueUtil(paginaHeap* paginaConBloque, int tamanioAAlocar){
	bool tieneBloqueUtil(bloqueHeapMetadata *bloqueHeapMetadata){
		return bloqueHeapMetadata->heapDeBloque->size > tamanioAAlocar;
	}
	return list_find(paginaConBloque->bloqueHeapMetadata, (void*)tieneBloqueUtil);
}

void alocarMemoria(int pid, int tamanioAAlocar, int socketCPU){
	bool esPCBBuscada(PCB *pcbAChequear){
		return pcbAChequear->pid == pid;
	}
	PCB *pcbAChequear = list_find(colaProcesos->elements, (void*)esPCBBuscada);
	paginaHeap *paginaConBloque;
	bloqueHeapMetadata *bloqueAOcupar;
	bloqueHeapMetadata *bloqueNuevo;
	if(tamanioAAlocar>(tamanioPagina-sizeof(heapMetadata)*2)){
        sendDeNotificacion(socketCPU, OPERACION_CON_CPU_FALLIDA);
	}else {
		if(noTienePaginasHeap(pcbAChequear, tamanioAAlocar)){
			paginaConBloque = pedirPaginaHeap(pcbAChequear);
			/*if(paginaConBloque != NULL){
				bloqueHeapMetadata *bloqueAOcupar = list_get(paginaConBloque->bloqueHeapMetadata, 0);
			}*/
		}else{
			paginaConBloque = buscarPaginaHeapConSuficienteTamanio(pcbAChequear, tamanioAAlocar);
		}
		if(paginaConBloque != NULL){
			bloqueAOcupar = obtenerBloqueUtil(paginaConBloque, tamanioAAlocar);
			bloqueNuevo = malloc(sizeof(bloqueHeapMetadata));
			bloqueNuevo->heapDeBloque->estaLibre = true;
			bloqueNuevo->heapDeBloque->size = bloqueAOcupar->heapDeBloque->size - tamanioAAlocar - sizeof(heapMetadata);
			bloqueNuevo->offset = bloqueAOcupar->offset + tamanioAAlocar + sizeof(heapMetadata);
			list_add(paginaConBloque->bloqueHeapMetadata, bloqueNuevo);
			pedirEscrituraAMemoria(pid, paginaConBloque, bloqueNuevo);
			t_puntero *punteroParaCPU = malloc(sizeof(t_puntero));
			*punteroParaCPU = paginaConBloque->pagina * tamanioPagina + bloqueNuevo->offset;
			sendRemasterizado(socketCPU, OPERACION_EXITOSA, sizeof(t_puntero), punteroParaCPU);
			free(punteroParaCPU);
		}else{
			sendDeNotificacion(socketCPU, OPERACION_CON_CPU_FALLIDA);
		}
	}
}

//--------------------------------------HILOS-----------------------------------//
//------------------------------------MANEJADOR CONSOLA-------------------------------------------------/////////
void* manejadorConexionConsola(void* socketAceptado){
	while(1){
		paquete* paqueteRecibidoDeConsola;
		int socketDeConsola = *(int*)socketAceptado;
		paqueteRecibidoDeConsola = recvRemasterizado(socketDeConsola);
		switch(paqueteRecibidoDeConsola->tipoMsj){
		case MENSAJE_CODIGO:
			iniciarProceso(paqueteRecibidoDeConsola, socketDeConsola);
			break;
		case FINALIZAR_PROCESO:
			finalizarProcesoEnviadoDeConsola(paqueteRecibidoDeConsola, socketDeConsola);
			break;
		case HANG_UP:
			close(socketDeConsola);
			break;
		default:
			perror("No se recibio correctamente el mensaje");
			log_error(loggerKernel, "No se reconoce la peticion hecha por el kernel...");
		}
		destruirPaquete(paqueteRecibidoDeConsola);
	}
}
//CONSOLA KERNEL
void modificarGradoMultiprogramacion(int grado){
	GRADO_MULTIPROG=grado;
}

void mostrarProcesosEnCola(t_queue* unaCola){
	int i = 0;
	if(!(list_is_empty(unaCola->elements))){
		printf("Los programas son: \n");
		for(;i<list_size(unaCola->elements);i++){
			PCB* unPCB = list_get(unaCola->elements,i);
			printf("PID-PROGRAMA: %d \n", unPCB->pid);
		}
	}
	else{
		printf("No hay Procesos.");
	}
}

void consultarListado(){
	mostrarProcesosEnCola(colaProcesos);
}

void consultarEstado(char* estado){
	if(string_equals_ignore_case(estado, "nuevo")){
		mostrarProcesosEnCola(colaNuevo);
	} else if(string_equals_ignore_case(estado, "bloqueado")){
		mostrarProcesosEnCola(colaBloqueado);
	} else if(string_equals_ignore_case(estado, "listo")){
		mostrarProcesosEnCola(colaListo);
	} else if(string_equals_ignore_case(estado, "ejecutando")){
		mostrarProcesosEnCola(colaEjecutando);
	} else if(string_equals_ignore_case(estado, "finalizado")){
		mostrarProcesosEnCola(colaFinalizado);
	} else{
		printf("El estado %s es incorrecto.",estado);
	}


}

void detenerPlanificacion(){
	estadoPlanificacionesSistema = false;
}

void reanudarPlanificacion(){
	estadoPlanificacionesSistema = true;
}

void obtenerRafagas(int pid){
	int i = 0;
	if(!(list_is_empty(colaProcesos->elements))){
		for(;i<list_size(colaProcesos->elements);i++){
			PCB* unPCB = list_get(colaProcesos->elements,i);
			if(unPCB->pid==pid){
				printf("Cantidad de rafagas ejecutadas por el pid %d fueron %d",pid,unPCB->rafagas);
			}
		}
	}
	else{
		printf("No hay procesos cargados en el sistema.");
	}
}

void* manejadorTeclado(){
	char* comando = malloc(50*sizeof(char));
	size_t tamanioMaximo = 50;

	while(1){
		puts("Esperando comando...");

		getline(&comando, &tamanioMaximo, stdin);

		int tamanioRecibido = strlen(comando);
		comando[tamanioRecibido-1] = '\0';
		char* posibleFinalizarPrograma = string_substring_until(comando, strlen("finalizarPrograma"));
		char* posibleConsultarEstado = string_substring_until(comando, strlen("consultarEstado"));
		char* posibleModificarGrado = string_substring_until(comando, strlen("modificarGradoDeMultiprogramacion"));
		char* posibleObtenerRafaga = string_substring_until(comando,strlen("obtenerRafagas"));

		if(string_equals_ignore_case(posibleFinalizarPrograma,"finalizarPrograma")){
			int valorPid = atoi(string_substring_from(comando,strlen("finalizarPrograma")+1));
			int exitCode = -7;
			finalizarProceso(valorPid,exitCode);
			list_add(finalizadosPorConsola, &valorPid);
			free(posibleFinalizarPrograma);
		}else if(string_equals_ignore_case(comando, "consultarListado")){
			consultarListado();
		}else if(string_equals_ignore_case(posibleConsultarEstado, "consultarEstado")){
			char* estado=string_substring_from(comando,strlen("consultarEstado")+1);
			consultarEstado(estado);
			free(posibleConsultarEstado);
			free(estado);
		}else if(string_equals_ignore_case(comando, "detenerPlanificacion")){
			detenerPlanificacion();
		}else if(string_equals_ignore_case(comando, "reanudarPlanificacion")){
			reanudarPlanificacion();
		}else if(string_equals_ignore_case(posibleModificarGrado, "modificarGradoDeMultiprogramacion")){
			int valorGrado=atoi(string_substring_from(comando,strlen("modificarGradoDeMultiprogramacion")+1));
			modificarGradoMultiprogramacion(valorGrado);
			free(posibleModificarGrado);
		}else if(string_equals_ignore_case(posibleObtenerRafaga, "obtenerRafagas")){
			int valorPid=atoi(string_substring_from(comando,strlen("obtenerRafagas")+1));
			obtenerRafagas(valorPid);
			free(posibleObtenerRafaga);
		}else{
			puts("Error de comando");
		}

		comando = realloc(comando,50*sizeof(char));
	}
}

char* obtenerCodigoPrograma(int pidAComparar){
	int i;
	for(i=0;i<list_size(listaProgramas);i++){
		datosPrograma* unPrograma = list_get(listaProgramas,i);
		if(unPrograma->pid==pidAComparar){
			datosPrograma* unProgramaBuscado = list_remove(listaProgramas,i);
			return unProgramaBuscado->script;
		}
	}

	return NULL;
}

void nuevoAListo(PCB* unaPCB){
	char* codigoPrograma = obtenerCodigoPrograma(unaPCB->pid);
	int cantidadPaginasCodigo = obtenerCantidadPaginas(codigoPrograma);
	void *inicioDePrograma = malloc(sizeof(int)*2);
	memcpy(inicioDePrograma, &unaPCB->pid, sizeof(int));
	memcpy(inicioDePrograma+sizeof(int), &cantidadPaginasCodigo, sizeof(int));
	int sePudo = mandarInicioAMemoria(inicioDePrograma);
	if(sePudo == OPERACION_EXITOSA){
		enviarCodigoDeProgramaAMemoria(unaPCB->pid, codigoPrograma);
		sendRemasterizado(unaPCB->socketConsola, ENVIAR_PID, sizeof(int), &unaPCB->pid);
		queue_push(colaListo,unaPCB);
	}else{
		char* mensaje = string_new();
		string_append(&mensaje,"No hay espacio para iniciar el programa.");
		int tamanio = strlen(mensaje);
		void* mensajeConTamanio = malloc(sizeof(int)+tamanio);
		memcpy(mensajeConTamanio,&tamanio,sizeof(int));
		memcpy(mensajeConTamanio+sizeof(int),mensaje,tamanio);
		sendRemasterizado(unaPCB->socketConsola,ESPACIO_INSUFICIENTE,tamanio+sizeof(int),mensaje);
		free(unaPCB); //VER SI SE MUEVE A UNA COLA DE RECHAZADOS O LIBERAR EL PCB CREADO
		free(mensaje);
		free(mensajeConTamanio);
	}
	free(codigoPrograma);
	free(inicioDePrograma);
}

//PLANIFICACION DEL SISTEMA

void manejarColaNueva(){
	if(queue_size(colaListo)<GRADO_MULTIPROG){
		PCB* pcbNueva;
		pcbNueva=queue_pop(colaNuevo);
		nuevoAListo(pcbNueva);
		log_info(loggerKernel,"Se ha agregado la PCB del proceso %d a la cola de Listo", pcbNueva->pid);
	}else{
		log_info(loggerKernel,"No se ha podido agregar ningun proceso a la cola de listo por grado de multiprogramacion.");
	}

}


void *planificarSistema(){
	while(1){
		while(estadoPlanificacionesSistema){
			if(!queue_is_empty(colaNuevo)){
				manejarColaNueva();
			}

		}
	}
}

//--------------------------------------------------------------CPU---------------------------------------------------
//ABRIR ARCHIVO
bool buscarEnTG(char* path){
	int i;
	tablaGlobalFS* entradaATG;
	for(i=0;i<list_size(tablaAdminGlobal);i++){
		entradaATG=(tablaGlobalFS*)list_get(tablaAdminGlobal,i);
		if(string_equals_ignore_case(entradaATG->file,path)){
			return true;
		}
	}
	return false;
}

int nuevoGFD(){

	bool ordenarLista(tablaGlobalFS* menorFd, tablaGlobalFS* mayorFd){
		return menorFd->globalFD<mayorFd->globalFD;
	}

	list_sort(tablaAdminGlobal,(void*)ordenarLista);
	int i;
	int FD=0;
	tablaGlobalFS* entradaATG;
	for(i=0;i<list_size(tablaAdminGlobal);i++){
		entradaATG=(tablaGlobalFS*)list_get(tablaAdminGlobal,i);
		if(FD < entradaATG->globalFD){
			return FD;
		}
		FD++;
	}
	return FD;

}

int crearEntradaTG(char* path){
	tablaGlobalFS* entradaATG;
	entradaATG=malloc(sizeof(int)*2+string_length(path));
	entradaATG->globalFD=nuevoGFD();
	string_append(&entradaATG->file,path);
	entradaATG->open=1;
	list_add(tablaAdminGlobal,entradaATG);
	return entradaATG->globalFD;
}

int agregarEntradaTG(char* path){
	tablaGlobalFS* entradaGFS;

	bool existePath(tablaGlobalFS* pathBuscado){
		return string_equals_ignore_case(entradaGFS->file,pathBuscado->file);
	}

	if(buscarEnTG(path)){
		return crearEntradaTG(path);
	}else{
		entradaGFS=list_find(tablaAdminGlobal,(void*)existePath);
		entradaGFS->open++;
		return entradaGFS->globalFD;
	}
}

int nuevoFD(int pid){
	t_list* listaDelPID=list_create();

	bool pidIguales(tablaArchivosFS* pidActual){
		return pidActual->pid==pid;
	}

	bool ordenarLista(tablaGlobalFS* menorFd, tablaGlobalFS* mayorFd){
		return menorFd->globalFD<mayorFd->globalFD;
	}

	listaDelPID=list_filter(tablaAdminArchivos,(void*)pidIguales); //LA FUNCION NO PUEDE RECIBIR DOS PARAMETROS, HAY QUE DECLARARLA ADENTRO
	list_sort(listaDelPID,(void*)ordenarLista);
	int i;
	int FD=3;
	tablaGlobalFS* entradaATG;
	for(i=0;i<list_size(listaDelPID);i++){
		entradaATG=(tablaGlobalFS*)list_get(listaDelPID,i);
		if(FD < entradaATG->globalFD){
			return FD;
		}
		FD++;
	}
	return FD;
}

void agregarEntradaTP(int pid, int fd,t_banderas* flags,int fdGlobal){
	tablaArchivosFS* entradaATP=malloc(sizeof(int)*3+sizeof(t_banderas));
	entradaATP->pid=pid;
	entradaATP->FD=fd;
	entradaATP->flags=flags;
	entradaATP->globalFD=fdGlobal;
	list_add(tablaAdminArchivos,entradaATP);
}

bool validarArchivo(char* path){
	void *buffer=malloc(string_length(path)+sizeof(int));
	int tamanioPath=string_length(path);
	memcpy(buffer,&tamanioPath, sizeof(int));
	memcpy(buffer+sizeof(int),path, tamanioPath);
	sendRemasterizado(socketFS,VALIDAR_ARCHIVO,tamanioPath+sizeof(int),buffer);
	if(recvDeNotificacion(socketFS)==EXISTE_ARCHIVO){
		return true;
	}else{
		return false;
	}
}

bool crearArchivo(char* path){
	void *buffer=malloc(string_length(path));
	int tamanioPath=string_length(path);
	memcpy(buffer,&tamanioPath, sizeof(int));
	memcpy(buffer+sizeof(int),path, tamanioPath);
	sendRemasterizado(socketFS,CREAR_ARCHIVO,tamanioPath+sizeof(int),buffer);
	if(recvDeNotificacion(socketFS)==OPERACION_FINALIZADA_CORRECTAMENTE){
		log_info(loggerKernel,"Se abrio correctamente el archivo %s.",path);
		return true;
	}else{
		log_info(loggerKernel,"No se pudo abrir el archivo %s.",path);
		return false;
	}
}

void agregarRegistoSyscall(int pid){
	registroSyscall* registro=malloc(sizeof(int)*2);
	int i;
	for(i=0;i<list_size(registroDeSyscall);i++){
		registro=(registroSyscall*)list_get(registroDeSyscall,i);
		if(pid==registro->pid){
			registro->cantidadSyscall++;
		}
	}
}

int abrirArchivo(int pid,char* path,t_banderas* flags){
	int fdGlobal, fd;
	if(validarArchivo(path)){
		fdGlobal=agregarEntradaTG(path);
		fd = nuevoFD(pid);
		agregarEntradaTP(pid,fd,flags,fdGlobal);
		log_info(loggerKernel,"Se genero correctamente el FD %d , del pid %d.  ",fd,pid);
		agregarRegistoSyscall(pid);
		return fd;
	}else{
		if(flags->creacion==true){
			if(!crearArchivo(path)){
				return -1;
			}
			fdGlobal=agregarEntradaTG(path);
			fdGlobal=agregarEntradaTG(path);
			fd = nuevoFD(pid);
			agregarEntradaTP(pid,fd,flags,fdGlobal);
			agregarRegistoSyscall(pid);
			log_info(loggerKernel,"Se genero correctamente el FD %d , del pid %d.  ",fd,pid);
			return fd;
		}else{
			log_info(loggerKernel,"Error al abrir archivo por permiso incorrecto");
			return -1;
		}
	}

}

//CERRAR ARCHIVO
 int cerrarArchivo(int pid,int fd){
	 bool sonIguales(tablaArchivosFS* procesoActual){
	 	 		return procesoActual->pid==pid && procesoActual->FD==fd;
	 	 	}
 	tablaArchivosFS* entradaTP=list_find(tablaAdminArchivos,(void*)sonIguales);

 	bool tieneEntrada(tablaGlobalFS* pidActual){
 		return pidActual->globalFD==entradaTP->globalFD;
 	}
 	if(!list_any_satisfy(tablaAdminGlobal,(void*)tieneEntrada)){
 		log_info(loggerKernel,"Error hay cerrar archivo. No tiene registro en la tabla global de archivos");
 		return -1;
 	}else{
 		tablaGlobalFS* entradaTG=list_find(tablaAdminGlobal,(void*)tieneEntrada);
 		void eliminarEntrada(tablaArchivosFS* entrada){
 			free(entrada->flags);
 			free(entrada);
 		}
 		list_remove_and_destroy_by_condition(tablaAdminArchivos,(void*)sonIguales,(void*)eliminarEntrada);
 		entradaTG->open--;
 		if(entradaTG->open==0){
 			void eliminarEntradaTG(tablaGlobalFS* entrada){
 				free(entrada->file);
 				free(entrada);
 			}
 			list_remove_and_destroy_by_condition(tablaAdminGlobal,(void*)tieneEntrada,(void*)eliminarEntradaTG);
 			agregarRegistoSyscall(pid);
 		}
		return 1;
 	}


 }
 //BORRAR ARCHIVO
 bool borrarArchivoFS(char* path){
 	void *buffer=malloc(string_length(path)+sizeof(int));
 	int tamanioPath=string_length(path);
 	memcpy(buffer,&tamanioPath, sizeof(int));
 	memcpy(buffer+sizeof(int),path, tamanioPath);
 	sendRemasterizado(socketFS,BORRAR,tamanioPath+sizeof(int),buffer);
 	if(recvDeNotificacion(socketFS)==OPERACION_FINALIZADA_CORRECTAMENTE){
 		log_info(loggerKernel,"Se Borro correctamente el archivo %s.",path);
 		return true;
 	}else{
 		log_info(loggerKernel,"No se pudo borrar el archivo %s.",path);
 		return false;
 	}
 }

 int borrarArchivo(int pid, int fd){
	 bool sonIguales(tablaArchivosFS* procesoActual){
	 	 		return procesoActual->pid==pid && procesoActual->FD==fd;
	 	 	}
 	tablaArchivosFS* entradaTP=list_find(tablaAdminArchivos,(void*)sonIguales);

 	bool tieneEntrada(tablaArchivosFS* pidActual){
 		return pidActual->globalFD==entradaTP->globalFD;
 	}
 	tablaGlobalFS* hayUnaEntradaEnTG= list_find(tablaAdminGlobal,(void*)tieneEntrada);
 	if((hayUnaEntradaEnTG->open>=1)){
 		if(borrarArchivoFS(hayUnaEntradaEnTG->file)){
 			agregarRegistoSyscall(pid);
 			return 1;
 		}else{
 			return -1;
 		}
 	}else{
 		log_info(loggerKernel,"No se puedo borrar archivo del FD %d , del pid %d",fd,pid);
 		return -1;
 	}

 }

 //ESCRIBIR ARCHIVO
 bool guardarArchivo(char* path,int offset,int size, void* buffer){
 	void *bufferAMandar=malloc(string_length(path)+size+sizeof(int)*3);
 	int tamanioPath=string_length(path);
 	memcpy(bufferAMandar,&tamanioPath, sizeof(int));
 	memcpy(bufferAMandar+sizeof(int),path, tamanioPath);
 	memcpy(bufferAMandar+sizeof(int)+tamanioPath,&offset,sizeof(int));
 	memcpy(bufferAMandar+sizeof(int)*2+tamanioPath,&size,sizeof(int));
 	memcpy(bufferAMandar+sizeof(int)*3+tamanioPath,buffer,size);
 	sendRemasterizado(socketFS,OBTENER_DATOS,string_length(path)+size+sizeof(int)*3,bufferAMandar);
 	if(recvDeNotificacion(socketFS)==OPERACION_FINALIZADA_CORRECTAMENTE){
 		log_info(loggerKernel,"Se guardo correctamente el contenido %s en el archivo %s.",buffer,path);
 		return true;
 	}else{
 		log_info(loggerKernel,"No se pudo guardar el contenido %s en el archivo %s.",buffer, path);
 		return false;
 	}
 }

 int escribirArchivo(int pid,int fd,int tamanio, void* buffer){
	 bool sonIguales(tablaArchivosFS* procesoActual){
	 	 		return procesoActual->pid==pid && procesoActual->FD==fd;
	 	 	}
 	tablaArchivosFS* entradaTP=list_find(tablaAdminArchivos,(void*)sonIguales);
 	bool tieneEntrada(tablaArchivosFS* pidActual){
 		return pidActual->globalFD==entradaTP->globalFD;
 	}
 	tablaGlobalFS* hayUnaEntradaEnTG= list_find(tablaAdminGlobal,(void*)tieneEntrada);
 	if(guardarArchivo(hayUnaEntradaEnTG->file,entradaTP->cursor,tamanio,buffer)){
 		agregarRegistoSyscall(pid);
 		return 1;
 	}else{
 		return -1;
 	}
 }

 //LEER ARCHIVO
 char* mandarALeer(char* path,int offset,int size){
 	void *buffer=malloc(string_length(path)+sizeof(int)*3);
 	int tamanioPath=string_length(path);
 	memcpy(buffer,&tamanioPath, sizeof(int));
 	memcpy(buffer+sizeof(int),path, tamanioPath);
 	memcpy(buffer+sizeof(int)+tamanioPath,&offset,sizeof(int));
	memcpy(buffer+sizeof(int)*2+tamanioPath,&size,sizeof(int));
 	sendRemasterizado(socketFS,OBTENER_DATOS,string_length(path)+sizeof(int)*3,buffer);
 	if(recvDeNotificacion(socketFS)==OPERACION_FALLIDA){
 		log_info(loggerKernel,"No se pudo leer el archivo %s.",path);
 		return NULL;
 	}else{
 		paquete* paqueteRecibido=recvRemasterizado(socketFS);
 		return (char*)paqueteRecibido->mensaje;
 	}
 }

 char* leerArchivo(int pid, int fd, int tamanioALeer){
	 bool sonIguales(tablaArchivosFS* procesoActual){
	 	 		return procesoActual->pid==pid && procesoActual->FD==fd;
	 	 	}
 	tablaArchivosFS* entradaTP=list_find(tablaAdminArchivos,(void*)sonIguales);
 	bool tieneEntrada(tablaArchivosFS* pidActual){
 		return pidActual->globalFD==entradaTP->globalFD;
 	}
 	tablaGlobalFS* hayUnaEntradaEnTG= list_find(tablaAdminGlobal,(void*)tieneEntrada);
 	char* contenidoALeer=mandarALeer(hayUnaEntradaEnTG->file,entradaTP->cursor,tamanioALeer);
 	agregarRegistoSyscall(pid);
 	return contenidoALeer;
	}

 //MOVER CURSOR

 int moverCursor(int pid, int fd, int offset){
		bool sonIguales(tablaArchivosFS* procesoActual){
	 		return procesoActual->pid==pid && procesoActual->FD==fd;
	 	}
	 	tablaArchivosFS* entradaTP=list_find(tablaAdminArchivos,(void*)sonIguales);
	 	entradaTP->cursor=entradaTP->cursor+offset;
	 	agregarRegistoSyscall(pid);
	 	return 1;
 }

//ENVIAR PCB
void enviarPCB(int socketCPU, PCB* unProcesoParaCPU){
	int tamanioPCB = sacarTamanioPCB(unProcesoParaCPU);
	void* pcbSerializada = serializarPCB(unProcesoParaCPU);
	sendRemasterizado(socketCPU,MENSAJE_PCB,tamanioPCB,pcbSerializada);
	free(pcbSerializada);
}

//HILO
void *manejadorCPU(void* socketAceptado){
  int socketCPU = *(int*)socketAceptado;
  bool estaOcupadaCPU = false;
  while(1){
	  PCB* procesoParaCPU;
	   while(!estaOcupadaCPU && estadoPlanificacionesSistema){
	     if(!queue_is_empty(colaListo)){
	       //SEMAFOROS
	         procesoParaCPU = queue_pop(colaListo);
	         queue_push(colaEjecutando,procesoParaCPU);
	         estaOcupadaCPU = true;
	       //SEMAFOROS
	     }
	   }
	   enviarPCB(socketCPU, procesoParaCPU);
	   //VARIABLES
	   PCB* pcbAfectado;
	   int pid=0;
	   int FD=0;
	   char* path=string_new();
	   int tamanioPath;
	   t_banderas* flags=malloc(sizeof(t_banderas));
	   int estadoOperacionArchivo=0;
	   int tamanio=0;
	   char* contenido;
	   void* buffer;
	   int punteroDeLiberacion;

	   while(estaOcupadaCPU){
	     paquete *paqueteRecibidoDeCPU;
	     paqueteRecibidoDeCPU = recvRemasterizado(socketCPU);
	     switch (paqueteRecibidoDeCPU->tipoMsj) {
	       case WAIT_SEMAFORO:
	    	   waitSemaforo(paqueteRecibidoDeCPU->mensaje,socketCPU);
	         break;
	       case SIGNAL_SEMAFORO:
	    	   signalSemaforo(paqueteRecibidoDeCPU->mensaje);
	         break;
	       case PETICION_VARIABLE_COMPARTIDA_KERNEL:
	    	   obtenerValorVariableCompartida(paqueteRecibidoDeCPU->mensaje,socketCPU);
	    	 break;
	       case PETICION_CAMBIO_VALOR_KERNEL:
	    	   asignarValorVariableCompartida(paqueteRecibidoDeCPU->mensaje,socketCPU);
	    	 break;
	       case RESERVAR_HEAP:
	    	   memcpy(&pid, paqueteRecibidoDeCPU->mensaje, sizeof(int));
	    	   memcpy(&tamanio, paqueteRecibidoDeCPU->mensaje+sizeof(int), sizeof(int));
	    	   alocarMemoria(pid, tamanio, socketCPU);
	    	   break;
	       case LIBERAR_HEAP:
	    	   memcpy(&pid, paqueteRecibidoDeCPU->mensaje, sizeof(int));
	    	   memcpy(&punteroDeLiberacion, paqueteRecibidoDeCPU->mensaje+sizeof(int), sizeof(t_puntero));
	    	   liberarDatosDeProceso(pid, punteroDeLiberacion, socketCPU);
	    	   break;
	       case PROCESO_ABORTADO:
	         pcbAfectado = deserializarPCB(paqueteRecibidoDeCPU);
	         pcbAfectado->estaAbortado=true;
	         pcbAfectado->exitCode=-10;//DEFINIR EXIT CODE
	         queue_push(colaFinalizado, pcbAfectado);
	         sendRemasterizado(socketMemoria, FINALIZAR_PROCESO, sizeof(int), &pcbAfectado->pid);// PROBAR SI FUNCIONA
	         estaOcupadaCPU = false;
	         break;
	       case PROCESO_FINALIZADO:
	     	pcbAfectado= deserializarPCB(paqueteRecibidoDeCPU);
	     	pcbAfectado->estado=false;
	     	pcbAfectado->exitCode=0;
	         queue_push(colaFinalizado, pcbAfectado);
	         estaOcupadaCPU = false;
	         break;
	       case PROCESO_BLOQUEADO:
	     	pcbAfectado = deserializarPCB(paqueteRecibidoDeCPU);
	     	pcbAfectado->estaAbortado=false;
	         queue_push(colaBloqueado, pcbAfectado);
	         estaOcupadaCPU = false;
	         break;
	       case PROCESO_FINALIZO_RAFAGA:
	     	  pcbAfectado = deserializarPCB(paqueteRecibidoDeCPU);
	     	  queue_push(colaListo, pcbAfectado);
	     	  log_info(loggerKernel, "Se recibio la pcb por finalizacion de rafaga");
	     	  estaOcupadaCPU = false;
	     	  break;
	       case ABRIR_ARCHIVO:
	     	  memcpy(&pid,paqueteRecibidoDeCPU->mensaje+sizeof(int),sizeof(int));
	     	  tamanioPath=paqueteRecibidoDeCPU->tamMsj-sizeof(int)-sizeof(t_banderas);
	     	  memcpy(path,paqueteRecibidoDeCPU->mensaje+sizeof(int)+tamanioPath,tamanioPath);
	     	  memcpy(flags,paqueteRecibidoDeCPU->mensaje+sizeof(int)+tamanioPath+sizeof(t_banderas),sizeof(t_banderas));
	     	  estadoOperacionArchivo=abrirArchivo(pid,path,flags);
	     	  if(estadoOperacionArchivo!=-1){
	     		  sendDeNotificacion(socketCPU,OPERACION_CON_ARCHIVO_EXITOSA);
	     	  }else{
	     		  sendDeNotificacion(socketCPU,OPERACION_FALLIDA);
	     	  }
	     	  break;
	       case BORRAR_ARCHIVO:
	     	  memcpy(&pid,paqueteRecibidoDeCPU->mensaje+sizeof(int),sizeof(int));
	     	  memcpy(&FD,paqueteRecibidoDeCPU->mensaje+sizeof(int)*2,sizeof(int));
	     	  estadoOperacionArchivo=borrarArchivo(pid,FD);
	     	  if(estadoOperacionArchivo!=-1){
	     		  sendDeNotificacion(socketCPU,OPERACION_CON_ARCHIVO_EXITOSA);
	     	  }else{
	     		  sendDeNotificacion(socketCPU,OPERACION_FALLIDA);
	     	  }
	         break;
	       case CERRAR_ARCHIVO:
	     	  memcpy(&pid,paqueteRecibidoDeCPU->mensaje+sizeof(int),sizeof(int));
	     	  memcpy(&FD,paqueteRecibidoDeCPU->mensaje+sizeof(int)*2,sizeof(int));
	     	  estadoOperacionArchivo=cerrarArchivo(pid,FD);
	     	  if(estadoOperacionArchivo!=-1){
	     		  sendDeNotificacion(socketCPU,OPERACION_CON_ARCHIVO_EXITOSA);
	     	  }else{
	     		  sendDeNotificacion(socketCPU,OPERACION_FALLIDA);
	     	  }
	         break;
	       case MOVER_CURSOR:
	     	  memcpy(&pid,paqueteRecibidoDeCPU->mensaje+sizeof(int),sizeof(int));
	     	  memcpy(&FD,paqueteRecibidoDeCPU->mensaje+sizeof(int)*2,sizeof(int));
	     	  memcpy(&tamanio,paqueteRecibidoDeCPU->mensaje+sizeof(int)*3,sizeof(int));
	     	  estadoOperacionArchivo=moverCursor(pid,FD,tamanio);
	     	  if(estadoOperacionArchivo==-1){
	     		  sendDeNotificacion(socketCPU,OPERACION_CON_ARCHIVO_EXITOSA);
	     	  }else{
	     		  sendDeNotificacion(socketCPU,OPERACION_FALLIDA);
	     	  }
	         break;
	       case LEER_DATOS_ARCHIVO:
	     	  memcpy(&pid,paqueteRecibidoDeCPU->mensaje+sizeof(int),sizeof(int));
	     	  memcpy(&FD,paqueteRecibidoDeCPU->mensaje+sizeof(int)*2,sizeof(int));
	     	  memcpy(&tamanio,paqueteRecibidoDeCPU->mensaje+sizeof(int)*3,sizeof(int));
	     	  contenido=malloc(tamanio);
	 		  contenido=leerArchivo(pid,FD,tamanio);
	 		  buffer=malloc(tamanio+sizeof(int));
	 		  memcpy(buffer,&tamanio,sizeof(int));
	 		  memcpy(buffer+sizeof(int),contenido,tamanio);
	     	  if(contenido!=NULL){
	     	   		  sendRemasterizado(socketCPU,DATOS_DE_LECTURA,tamanio+sizeof(int),buffer);
	     	  }else{
	     		  	  sendDeNotificacion(socketCPU,OPERACION_FALLIDA);
	     	  }
	          break;
	       case ESCRIBIR_DATOS_ARCHIVO:
	     	  memcpy(&pid,paqueteRecibidoDeCPU->mensaje+sizeof(int),sizeof(int));
	     	  memcpy(&FD,paqueteRecibidoDeCPU->mensaje+sizeof(int)*2,sizeof(int));
	     	  memcpy(&tamanio,paqueteRecibidoDeCPU->mensaje+sizeof(int)*3,sizeof(int));
	     	  buffer =malloc(tamanio);
	     	  memcpy(buffer,paqueteRecibidoDeCPU->mensaje+sizeof(int)*3+tamanio,tamanio);
	     	  estadoOperacionArchivo=escribirArchivo(pid,FD,tamanio,buffer);
	     	  if(estadoOperacionArchivo!=-1){
	     		  sendDeNotificacion(socketCPU,OPERACION_CON_ARCHIVO_EXITOSA);
	     	  }else{
	     		  sendDeNotificacion(socketCPU,OPERACION_FALLIDA);
	     	  }
	          break;
	     }
	     destruirPaquete(paqueteRecibidoDeCPU);
  }

  }
}


//--------------------------------------HANDSHAKES-----------------------------------//
void realizarHandshakeMemoria(int socket) {
	sendRemasterizado(socket, SOY_KERNEL_MEMORIA, 0, NULL);
	paquete* paqueteConPaginas = recvRemasterizado(socket);
	if(paqueteConPaginas->tipoMsj == TAMANIO_PAGINA){
		tamanioPagina = *(int*)paqueteConPaginas->mensaje;
	}else{
		log_info(loggerKernel, "Error al recibir el tamanio de pagina de memoria.");
		exit(-1);
	}
	destruirPaquete(paqueteConPaginas);
}
void realizarHandshakeFS(int socket) {
	sendDeNotificacion(socket, SOY_KERNEL_FS);
	int notificacion = recvDeNotificacion(socket);
	if(notificacion==HANDSHAKE_ACEPTADO){
		log_info(loggerKernel, "Se pudo conectar con File System.");
	}else{
		log_info(loggerKernel, "Error al conectarse con File System.");
		exit(-1);
	}
}

void realizarhandshakeCPU(int socket){
	int tamanio = string_length(ALGORITMO)+sizeof(int)*3;
	void *buffer = malloc(tamanio);
	memcpy(buffer, ALGORITMO, string_length(ALGORITMO));
	if(string_equals_ignore_case(ALGORITMO, "RR")){
		memcpy(buffer+string_length(ALGORITMO), &QUANTUM, sizeof(int));
	}else{
		int quantumDeMentira = -1;
		memcpy(buffer+string_length(ALGORITMO), &quantumDeMentira, sizeof(int));
	}
	memcpy(buffer+string_length(ALGORITMO)+sizeof(int), &QUANTUM_SLEEP, sizeof(int));
	memcpy(buffer+string_length(ALGORITMO)+sizeof(int)*2,&STACK_SIZE,sizeof(int));
	sendRemasterizado(socket, HANDSHAKE_CPU, tamanio, buffer);
	free(buffer);
}

//--------------------------------------MAIN-----------------------------------//

int main (int argc, char *argv[]){
	loggerKernel = log_create("Kernel.log", "Kernel", 0, 0);
	//PUESTA EN MARCHA
	verificarParametrosInicio(argc);
	inicializarKernel(argv[1]);
	//char *path = "Debug/kernel.config";
	//inicializarKernel(path);

	//INICIALIZANDO VARIABLES
	pidActual = 1;
	int socketEscuchaCPU, socketEscuchaConsolas, socketMaxCliente, socketAChequear, socketQueAceptaClientes;
	int *socketConsola;
	int *socketCPU;
	pthread_t hiloManejadorTeclado, hiloManejadorConsola, hiloManejadorCPU, hiloManejadorPlanificacion;
	fd_set socketsCliente, socketsAuxiliaresCliente;
	FD_ZERO(&socketsCliente);
	FD_ZERO(&socketsAuxiliaresCliente);
	mostrarConfiguracionesKernel();
	estadoPlanificacionesSistema=true;

	//INICIALIZANDO LISTAS FS
	tablaAdminArchivos=list_create();
	tablaAdminGlobal=list_create();

	finalizadosPorConsola = list_create();
	listaProgramas = list_create();
	listaSemaforos = list_create();
	listaVariablesCompartidas = list_create();
	registroDeSyscall = list_create();

	//INICIALIZANDO COLAS
	colaBloqueado = queue_create();
	colaEjecutando = queue_create();
	colaFinalizado = queue_create();
	colaProcesos = queue_create();
	colaListo = queue_create();
	colaNuevo = queue_create();
	listaDeCPULibres=queue_create();

	//CREAR SEMAFOROS Y VARIABLES COMPARTIDAS
	crearSemaforos(SEM_IDS, SEM_INIT);
	crearVariablesCompartidas(SHARED_VARS);

	//CONEXIONES
	socketEscuchaCPU = ponerseAEscucharClientes(PUERTO_CPU, 0);
	socketEscuchaConsolas = ponerseAEscucharClientes(PUERTO_PROG, 0);
	socketMemoria = conectarAServer(IP_MEMORIA, PUERTO_MEMORIA);
	socketFS = conectarAServer(IP_FS, PUERTO_FS);
	FD_SET(socketEscuchaCPU, &socketsCliente);
	FD_SET(socketEscuchaConsolas, &socketsCliente);
	socketMaxCliente = calcularSocketMaximo(socketEscuchaConsolas, socketEscuchaCPU);
	realizarHandshakeMemoria(socketMemoria);
	//realizarHandshakeFS(socketFS);

	//ADMINISTRACION DE CONEXIONES
	pthread_create(&hiloManejadorTeclado, NULL, manejadorTeclado, NULL);
	pthread_create(&hiloManejadorPlanificacion, NULL, planificarSistema, NULL);

	while(1){
		socketsAuxiliaresCliente = socketsCliente;
		if(select(socketMaxCliente + 1, &socketsAuxiliaresCliente, NULL, NULL, NULL) == -1){
			perror("Error de select...");
			exit(-1);
		}
		for(socketAChequear = 0; socketAChequear <= socketMaxCliente; socketAChequear++){
			if(FD_ISSET(socketAChequear, &socketsAuxiliaresCliente)){
				if(socketAChequear == socketEscuchaConsolas){
					socketQueAceptaClientes = aceptarConexionDeCliente(socketEscuchaConsolas);
					FD_SET(socketQueAceptaClientes, &socketsCliente);
					socketMaxCliente = calcularSocketMaximo(socketQueAceptaClientes, socketMaxCliente);
				}else if(socketAChequear == socketEscuchaCPU){
					socketQueAceptaClientes = aceptarConexionDeCliente(socketEscuchaCPU);
					FD_SET(socketQueAceptaClientes, &socketsCliente);
					socketMaxCliente = calcularSocketMaximo(socketQueAceptaClientes, socketMaxCliente);
				}else{
					int tipoMsjRecibido = recvDeNotificacion(socketAChequear);
					switch(tipoMsjRecibido){
						case CONSOLA:
							socketConsola = malloc(sizeof(int));
							*socketConsola = socketAChequear;
							log_info(loggerKernel, "Se conecto una nueva consola con el socket %d...", socketAChequear);
							pthread_create(&hiloManejadorConsola, NULL, manejadorConexionConsola, (void*)socketConsola);
							FD_CLR(socketAChequear, &socketsCliente);
							break;
						case CPU:
							socketCPU = malloc(sizeof(int));
							*socketCPU = socketAChequear;
							log_info(loggerKernel, "Se conecto una nueva CPU con el socket %d...", socketAChequear);
							realizarhandshakeCPU(socketAChequear);
							pthread_create(&hiloManejadorCPU, NULL, manejadorCPU, (void*)socketCPU);
							FD_CLR(socketAChequear, &socketsCliente);
							break;
						default:
							log_info(loggerKernel, "Se recibio una conexion erronea...");
							log_info(loggerKernel, "Cerrando socket recibido...");
							FD_CLR(socketAChequear, &socketsCliente);
							close(socketAChequear);
					}
				}
			}
		}
	}
	return EXIT_SUCCESS;
}
