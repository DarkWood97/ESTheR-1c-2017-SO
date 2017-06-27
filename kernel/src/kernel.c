#include "funcionesGenericas.h"
#include "socket.h"
#include <stdio.h>
#include <math.h>
#include <parser/metadata_program.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <semaphore.h>

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
} direccion;

typedef struct __attribute__((packed)) {
	char nombreVariable;
	direccion direccionDeVariable;
}variable;

typedef struct __attribute__((packed)) {
	int pos;
	t_list* args;
	t_list* vars;
	int retPos;
	direccion retVar;
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
	uint32_t size;
	bool isFree;
} HeapMetadata;

typedef struct __attribute__((packed)) {
	t_queue* nuevo;
	t_queue* listo;
	t_queue* ejecutando;
	t_queue* bloqueado;
	t_queue* finalizado;
} Estados;

typedef struct __attribute__((packed)) {
	unsigned char nombreVariables;
	int valorVariables;
} SharedVars;

typedef struct __attribute__((packed)) {
  int socket_CPU;
  int socket_CONSOLA;
  PCB *pcb;
  bool abortado;

} t_proceso;

//--VARIABLES GLOBALES------------------------------------------------------
#define CONEXION_CONSOLA 1005
#define CORTO 0
#define ERROR -1
#define ESPACIO_INSUFICIENTE -2
#define MENSAJE_IMPRIMIR 101
#define TAMANIO_PAGINA 102
#define	MENSAJE_CODIGO 103
#define MENSAJE_PID   105
#define MEMORIA 1002
#define CPU 1003
#define CONSOLA 1005
#define INICIAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define INICIO_EXITOSO 50
#define MENSAJE_PCB 1015
#define RECIBIR_PCB 1016
#define WAIT_SEMAFORO 1017
#define SIGNAL_SEMAFORO 1018
#define SEMAFORO_BLOQUEADO 1019
#define SEMAFORO_NO_BLOQUEADO 1020
#define ASIGNAR_VARIABLE_COMPARTIDA 666
#define OBTENER_VARIABLE_COMPARTIDA 667
#define ESCRIBIR_DATOS 505
#define LEER_DATOS 504

//VARIABLES GLOBALES
t_log * loggerKernel;
bool YA_HAY_UNA_CONSOLA = false;
int pid_actual = 1;
int TAM_PAGINA;
int cantPag=0;
PCB* nuevoPCB;
t_list* tablaKernel;
Estados* estado;
t_queue* cola_CPU_libres;
t_queue** colas_semaforos;
bool estadoPlanificacion =true;
t_list* variablesCompartidas;
sem_t sem_ready;
int socketConsola;
int socketParaMemoria;
int socketCPU;

//////////////////////////////FUNCIONES SEMAFOROS/////////////////////////////////////////////////////////////////////////////////////
int tamanioDeArray(char** array){
	int tamanio = ((strlen((char*)array)/sizeof(char*))*sizeof(int)); //Asi se saca el tamanio del array, calculo cuantos char* hay, lo divido por el tamanio de char*, lo que me da la cantidad de elementos, y lo multiplico por el tamanio de un int
	return tamanio;
}

int *inicializarDesdeArrays(char** array1, char** array2){
  int contador;
  int* arrayInicializado;
  arrayInicializado = malloc(tamanioDeArray(array2));
  int i = 0;
  for(; array2[i]!=NULL && array1[i]!=NULL; i++){
    arrayInicializado[i]=atoi(array1[i]);
  }
  return arrayInicializado;
}

void signalSemaforo(char *semaforo) {
	int i; t_proceso *proceso;

	for (i = 0; i < strlen((char*)sem_Ids) / sizeof(char*); i++) {
		if (strcmp((char*)sem_Ids[i], semaforo) == 0) {

			if(list_size(colas_semaforos[i]->elements)){
				proceso = queue_pop(colas_semaforos[i]);
				queue_push(estado->listo, proceso);
				sem_post(&sem_ready);
			}else{
				sem_Init[i]++;
			}
		}
	}
	perror("No existe el semaforo");
}

int waitSemaforo(t_proceso *proceso, char *semaforo) {
	int i;

	for (i = 0; i < strlen((char*)sem_Ids) / sizeof(char*); i++) {
		if (strcmp((char*)sem_Ids[i], semaforo) == 0) {
			queue_push(colas_semaforos[i], proceso);
			return SEMAFORO_BLOQUEADO;
		}
		else{
			return SEMAFORO_NO_BLOQUEADO;
		}
	}
	perror("No existe el semaforo");
}
/////////////////////////////////////////VARIABLES COMPARTIDAS/////////////////////////////////////////////////////////////////////////
void asignarVariableCompartida(char* nombreVariable,int valor){
	int i = 0;
	for(;i<=list_size(variablesCompartidas);i++){
		SharedVars* variable = list_remove(variablesCompartidas,i);
		if((variable->nombreVariables)==nombreVariable){
			variable->valorVariables = valor;
			list_add_in_index(variablesCompartidas, i, variable);
		}
		else{
			perror("No existe la variable compartida");
			exit(-1);
		}
	}
}

int obtenerVariableCompartida(char* nombreVariable){
	int i = 0;
	for(;i<=list_size(variablesCompartidas);i++){
		SharedVars* variable = list_get(variablesCompartidas,i);
		if((variable->nombreVariables)==nombreVariable){
			return variable->valorVariables;
		}
		else{
			perror("No existe la variable compartida");
			exit(-1);
		}
	}
}

void inicializarVariablesCompartidas(){
	int i = 0;
	for(;shared_Vars[i]!=NULL;i++){
		SharedVars* variables;
		variables->nombreVariables=shared_Vars[i];
		list_add(variablesCompartidas,variables);
	}
}

////////////////////////////////////////FUNCIONES COLAS////////////////////////////////////////////////////////////////////////////////

t_proceso* dameProceso(t_queue *cola, int pid ) {
	int a = 0;

	for (; a<queue_size(cola); a++ ){
		t_proceso *procesoAux=(t_proceso*)list_get(cola->elements, a);
		if (procesoAux->pcb->PID == pid){
			return (t_proceso*)list_remove(cola->elements, a);
		}
	}
	return NULL;
}


void enviarANuevo(PCB* pcbNuevo){
	t_proceso* proceso;
	proceso->pcb = pcbNuevo;
	proceso->abortado=false;
	if(queue_size(estado->nuevo)<grado_Multiprog){
	  log_info(loggerKernel, "Se agrega nuevo proceso a ejecutar.");
	  queue_push(estado->nuevo,proceso);
	}else{
	  log_info(loggerKernel, "No se pudo agregar el proceso %d, por el grado de multiprogramacion",proceso->pcb->PID);
	}
}

void enviarAListo(int pid){
	t_proceso *proceso = dameProceso(estado->nuevo,pid);
	queue_push(estado->listo,proceso);
}

void enviarAEjecutar(int pid){
	t_proceso* proceso = dameProceso(estado->listo,pid);
	queue_push(estado->ejecutando,proceso);
}

void enviarASalida(t_queue* cola,int pid){
	t_proceso *proceso = dameProceso(cola,pid);
	queue_push(estado->finalizado,proceso);
}

int obtenerNumeroPagina(int pid,int* offset){
	int i = 0;

	for(;i<=(list_size(tablaKernel));i++){
		TablaKernel* tk = list_get(tablaKernel,i);
		if(tk->pid==pid){
			t_proceso* proceso = dameProceso(estado->ejecutando,pid);
			list_add(estado->ejecutando, proceso);
			(*offset) = proceso->pcb->cod.comienzo;
			int valor =((TAM_PAGINA*tk->paginas)-(proceso->pcb->cod.comienzo));
			int cantidad = (valor) % (TAM_PAGINA);
			if (valor % TAM_PAGINA != 0) {
				cantidad++;
			}
			return valor;
		}
	}
}

void escribirDatos(int socketMemoria,void* buffer, int tamanio,int pid){
	paquete paquete;
	paquete.mensaje = malloc((sizeof(int)*4)+tamanio);
	paquete.tipoMsj = ESCRIBIR_DATOS;
	paquete.tamMsj = (sizeof(int)*4)+tamanio;
	int offset;
	int numPagina = obtenerNumeroPagina(pid,offset);
	memcpy(paquete.mensaje,pid,sizeof(int));
	memcpy(paquete.mensaje+sizeof(int),numPagina, sizeof(int));
	memcpy(paquete.mensaje+(sizeof(int)*2), offset,sizeof(int));
	memcpy(paquete.mensaje+(sizeof(int)*3), tamanio,sizeof(int));
	memcpy(paquete.mensaje+(sizeof(int)*4), buffer,tamanio);
	if(send(socketMemoria,&paquete,paquete.tamMsj,0) == -1){
		perror("Error al enviar datos a memoria");
	    log_info(loggerKernel, "Error al enviar datos a memoria");
	    exit(-1);
	}
	free(paquete.mensaje);
}

void leerDatosSolicitud(int pid,int tamALeer,int socketMemoria){
	paquete paquete;
	paquete.mensaje = malloc(sizeof(int)*4);
	paquete.tipoMsj = LEER_DATOS;
	paquete.tamMsj = sizeof(int)*4;
	int offset;
	int numPagina = obtenerNumeroPagina(pid,offset);
	memcpy(paquete.mensaje,pid,sizeof(int));
	memcpy(paquete.mensaje+sizeof(int),numPagina, sizeof(int));
	memcpy(paquete.mensaje+(sizeof(int)*2), tamALeer,sizeof(int));
	memcpy(paquete.mensaje+(sizeof(int)*3), offset,sizeof(int));
	if(send(socketMemoria,&paquete,paquete.tamMsj,0) == -1){
		perror("Error al enviar datos para leer de memoria");
		log_info(loggerKernel, "Error al enviar datos para leer de memoria");
		exit(-1);
	}
	free(paquete.mensaje);
}

//////////////////////////////////////////MANEJADOR DE CPU/////////////////////////////////////////////////////////////////////////
void *manejadorProceso (void* socketCPU,void* pid,void* socketMemoria){
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
	          t_proceso* proceso = dameProceso(estado->ejecutando,*(int*)pid);
	          proceso->pcb = nuevoPCB;
	          list_add(estado->ejecutando, proceso);
	          break;
	        case FINALIZAR_PROGRAMA:
	        	enviarASalida(estado->ejecutando,pid);
	        break;
	        case WAIT_SEMAFORO: ;
	        	t_proceso* procesoWait = dameProceso(estado->ejecutando,*(int*)pid);
	        	paquete paqueteWait;
	        	paqueteWait.mensaje = malloc(sizeof(int));
	        	int caso = waitSemaforo(procesoWait, paqueteRecibidoDeCPU.mensaje);
	        	paqueteWait.tamMsj = sizeof(int);
	        	paqueteWait.tipoMsj = WAIT_SEMAFORO;
	        	paqueteWait.mensaje = &caso;
	        	if(send(socketCPU, &paqueteWait, paqueteWait.tamMsj, 0) == -1){
	        		perror("Error al enviar si el semaforo fue bloqueado");
	        	    log_info(loggerKernel, "Error al enviar si el semaforo fue bloqueado");
	        	    exit(-1);
	        	}
	        	list_add(estado->ejecutando, procesoWait);
	        	break;
	        case SIGNAL_SEMAFORO:
	        	signalSemaforo(paqueteRecibidoDeCPU.mensaje);
	        	break;
	        case ASIGNAR_VARIABLE_COMPARTIDA: ;
	        	char* nombreVariable;
	        	int valorVariable;
	        	int tamanioNombre = paqueteRecibidoDeCPU.tamMsj-sizeof(int);
				nombreVariable = malloc(tamanioNombre);
	        	memcpy(nombreVariable, paqueteRecibidoDeCPU.mensaje,tamanioNombre);
	            memcpy(&valorVariable, paqueteRecibidoDeCPU.mensaje + tamanioNombre,sizeof(int));
	            asignarVariableCompartida(nombreVariable,valorVariable);
	        	break;
	        case OBTENER_VARIABLE_COMPARTIDA: ;
	        	paquete paqueteVars;
	        	paqueteVars.mensaje = malloc(sizeof(int));
	        	paqueteVars.tipoMsj = OBTENER_VARIABLE_COMPARTIDA;
	        	paqueteVars.tamMsj = sizeof(int);
	        	paqueteVars.mensaje = obtenerVariableCompartida(paqueteRecibidoDeCPU.mensaje);
	        	if(send(*(int*)socketCPU, &paqueteVars, paqueteVars.tamMsj, 0) == -1){
	        		perror("Error al enviar el valor de la variable compartida");
	        		log_info(loggerKernel, "Error al enviar el valor de la variable compartida");
	        	    exit(-1);
	        	}
	        	free(paqueteVars.mensaje);
	        	break;
	        case ESCRIBIR_DATOS: ;
	        	int tamanio;
	        	memcpy(&tamanio,paqueteRecibidoDeCPU.mensaje,sizeof(int));
	        	void* buffer = malloc(tamanio);
	        	memcpy(buffer,paqueteRecibidoDeCPU.mensaje+sizeof(int),tamanio);
	        	escribirDatos(*(int*)socketMemoria,buffer,tamanio,*(int*)pid);
	        	free(buffer);
	        	break;
	        case LEER_DATOS:
	        	leerDatosSolicitud(*(int*)pid,paqueteRecibidoDeCPU.mensaje,*(int*)socketMemoria);
	        	break;
	        default:
	        	perror("No se reconoce el mensaje enviado por CPU");
	      }
	  }
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int comenzarAEjecutar(int pid, int socketMemoria){
	while(estadoPlanificacion){
		//Semaforos
		if((!queue_is_empty(estado->listo))&&(!queue_is_empty(cola_CPU_libres))){
			t_proceso* proceso = queue_peek(estado->ejecutando);

			int cpuUtilizada = queue_peek(cola_CPU_libres);
			queue_pop(cola_CPU_libres);

			pthread_t hilo_manejadorProceso;
			pthread_create(&hilo_manejadorProceso,NULL,manejadorProceso,(void*)(cpuUtilizada,proceso->pcb->PID,socketMemoria));
			pthread_attr_destroy(hilo_manejadorProceso);
			return cpuUtilizada;
		}
	}
}

t_proceso* chequearListas(int pid){
	t_proceso* proceso;
	t_queue* colaAux = queue_create();
	colaAux = estado->nuevo;
	if((proceso=dameProceso(colaAux,pid))!=NULL){
		queue_destroy(colaAux);
		return proceso;
	}
	else if((proceso=dameProceso(colaAux,pid))!=NULL){
		queue_destroy(colaAux);
		return proceso;
	}
	else if((proceso=dameProceso(colaAux,pid))!=NULL){
		queue_destroy(colaAux);
		return proceso;
	}
	else if((proceso=dameProceso(colaAux,pid))!=NULL){
		queue_destroy(colaAux);
		return proceso;
	}
	else{
		queue_destroy(colaAux);
		return NULL;
	}
}

t_queue* chequearEstado(int pid){
	t_proceso* proceso;
	t_queue* colaAux = queue_create();
	colaAux = estado->nuevo;
	t_queue* colaAux2 = queue_create();
	colaAux2 = estado->listo;
	t_queue* colaAux3 = queue_create();
	colaAux3 = estado->ejecutando;
	t_queue* colaAux4 = queue_create();
	colaAux4 = estado->bloqueado;

	if((proceso=dameProceso(colaAux,pid))!=NULL){
		queue_destroy(colaAux);
		queue_destroy(colaAux2);
		queue_destroy(colaAux3);
		queue_destroy(colaAux4);
		return estado->nuevo;
	}
	else if((proceso=dameProceso(colaAux2,pid))!=NULL){
		queue_destroy(colaAux);
		queue_destroy(colaAux2);
		queue_destroy(colaAux3);
		queue_destroy(colaAux4);
		return estado->listo;
	}
	else if((proceso=dameProceso(colaAux3,pid))!=NULL){
		queue_destroy(colaAux);
		queue_destroy(colaAux2);
		queue_destroy(colaAux3);
		queue_destroy(colaAux4);
		return estado->ejecutando;
	}
	else if((proceso=dameProceso(colaAux4,pid))!=NULL){
		queue_destroy(colaAux);
		queue_destroy(colaAux2);
		queue_destroy(colaAux3);
		queue_destroy(colaAux4);
		return estado->bloqueado;
	}
	else{
		queue_destroy(colaAux);
		queue_destroy(colaAux2);
		queue_destroy(colaAux3);
		queue_destroy(colaAux4);
		return NULL;
	}
}


//-----------------------------------------------FUNCIONES DE CONSOLA KERNEL------------------------------------------

t_list* filtrarLista(int socket){
	t_list* listaFiltrada = list_create();
	int i = 0;

	t_queue* colaAuxiliar = queue_create();
	colaAuxiliar = estado->ejecutando;
	for(;i<(queue_size(estado->ejecutando));i++){
		t_proceso* proceso = queue_peek(colaAuxiliar);
		queue_pop(colaAuxiliar);
		if((proceso->socket_CONSOLA)==socket){
			list_add(listaFiltrada, proceso);
		}
	}
	queue_destroy(colaAuxiliar);

	colaAuxiliar = queue_create();
	colaAuxiliar = estado->bloqueado;
	for(;i<(queue_size(estado->bloqueado));i++){
		t_proceso* proceso = queue_peek(colaAuxiliar);
		queue_pop(colaAuxiliar);
		if((proceso->socket_CONSOLA)==socket){
			list_add(listaFiltrada, proceso);
		}
	}
	queue_destroy(colaAuxiliar);

	colaAuxiliar = queue_create();
	colaAuxiliar = estado->listo;
	for(;i<(queue_size(estado->listo));i++){
		t_proceso* proceso = queue_peek(colaAuxiliar);
		queue_pop(colaAuxiliar);
		if((proceso->socket_CONSOLA)==socket){
			list_add(listaFiltrada, proceso);
		}
	}
	queue_destroy(colaAuxiliar);

	colaAuxiliar = queue_create();
	colaAuxiliar = estado->nuevo;
	for(;i<(queue_size(estado->nuevo));i++){
		t_proceso* proceso = queue_peek(colaAuxiliar);
		queue_pop(colaAuxiliar);
		if((proceso->socket_CONSOLA)==socket){
			list_add(listaFiltrada, proceso);
		}
	}
	queue_destroy(colaAuxiliar);

	return listaFiltrada;
	list_destroy(listaFiltrada);
}

void verificarProcesos(socketConsola){
	t_list* listaFiltrada = list_create();
	listaFiltrada = filtrarLista(socketConsola);
	int i=0;
	for(;i<(list_size(listaFiltrada));i++){
		t_proceso* proceso = list_get(listaFiltrada, i);
		proceso->pcb->exitCode = -6;
		t_queue* cola = chequearEstado(proceso->pcb->PID);
		enviarASalida(cola,proceso->pcb->PID);
	}
	list_destroy(listaFiltrada);
}

void finalizarPrograma(int pid){
	t_proceso* proceso;
	if((proceso=chequearListas(pid))!=NULL){
		proceso->abortado=true;
		t_queue* cola = chequearEstado(pid);
		enviarASalida(cola,proceso->pcb->PID);
		proceso->pcb->exitCode=-7;
	}
	else{
		log_info(loggerKernel,"No se encontro el proceso a finalizar xD");
	}
}

void consultarListado(){
	int tamanio = list_size(tablaKernel);
	int posicion = 0;
	for(; posicion<tamanio;posicion++){
		TablaKernel* tk = list_get(tablaKernel, posicion); //A los gomasos
		printf("%d ", tk->pid);
	}
}

t_queue* chequearQueColaEs(char* estadoRecibido){
	if(string_equals_ignore_case(estadoRecibido, "ready")){
		return estado->listo;
	} else if(string_equals_ignore_case(estadoRecibido, "exec")){
		return estado->ejecutando;
	} else if(string_equals_ignore_case(estadoRecibido, "new")){
		return estado->nuevo;
	} else if(string_equals_ignore_case(estadoRecibido, "exit")){
		return estado->finalizado;
	} else {
		return NULL;
	}
}

void consultarEstado(char* estado){
	t_queue* cola = chequearQueColaEs(estado);
	int tamanio = list_size(cola);
	int posicion = 0;
	if(cola!=NULL){
		for(; posicion<tamanio;posicion++){
			t_proceso* tp = list_get(cola, posicion);
			printf("%d ", tp->pcb->PID);
		}
	}
	else{
		puts("Ecolecua");
	}
}

void modificarGradoMultiprogramacion (){
	if (!queue_is_empty(estado->nuevo)){
		grado_Multiprog = queue_size(estado->nuevo);
	}
}

void detenerPlanificacion(){
	estadoPlanificacion=false;
}

void reanudarPlanificacion(){
	estadoPlanificacion=true;
}
//----------------------------------------FUNCIONES KERNEL------------------------------------------
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

void inicializarPCB(char* buffer,int tamanioBuffer) {
	t_metadata_program* metadata_program = metadata_desde_literal(buffer);

	nuevoPCB->PID = aumentarPID();
	nuevoPCB->ProgramCounter = 0;
	nuevoPCB->paginas_Codigo = (int)ceil((double)tamanioBuffer / (double)cantPag);
	nuevoPCB->cod = cargarCodeIndex(buffer, metadata_program);
	nuevoPCB->tamEtiquetas = metadata_program->etiquetas_size;
	nuevoPCB->etiquetas = cargarEtiquetasIndex(metadata_program,nuevoPCB->tamEtiquetas);
	nuevoPCB->contextoActual = list_create();
	nuevoPCB->contextoActual = inicializarStack(nuevoPCB->contextoActual);
	nuevoPCB->tamContextoActual = 1;

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

void recibirPCB(void* mensaje){
	memcpy(nuevoPCB->PID,mensaje, sizeof(int));
	memcpy(nuevoPCB->ProgramCounter,mensaje, sizeof(int));
	memcpy(nuevoPCB->paginas_Codigo,mensaje+(sizeof(int)*2), sizeof(int));
	memcpy(nuevoPCB->cod.comienzo,mensaje+(sizeof(int)*3), sizeof(int));
	memcpy(nuevoPCB->cod.offset,mensaje+(sizeof(int)*4), sizeof(int));
	memcpy(nuevoPCB->exitCode,mensaje+(sizeof(int)*5), sizeof(int));
	memcpy(nuevoPCB->tamContextoActual,mensaje+(sizeof(int)*6), sizeof(int));
	memcpy(nuevoPCB->contextoActual,mensaje+(sizeof(int)*7), nuevoPCB->tamContextoActual);
	memcpy(nuevoPCB->tamEtiquetas,mensaje+(sizeof(int)*7)+nuevoPCB->tamContextoActual, sizeof(int));
	memcpy(nuevoPCB->etiquetas, mensaje+(sizeof(int)*8)+nuevoPCB->tamContextoActual, nuevoPCB->tamEtiquetas);
	memcpy(nuevoPCB->tablaKernel.paginas, mensaje+(sizeof(int)*8)+nuevoPCB->tamContextoActual+nuevoPCB->tamEtiquetas, sizeof(int));
	memcpy(nuevoPCB->tablaKernel.pid,mensaje+(sizeof(int)*9)+nuevoPCB->tamContextoActual+nuevoPCB->tamEtiquetas, sizeof(int));
	memcpy(nuevoPCB->tablaKernel.tamaniosPaginas,mensaje+(sizeof(int)*10)+nuevoPCB->tamContextoActual+nuevoPCB->tamEtiquetas, sizeof(int));

	free(mensaje);
}

void enviarPCB(int socketCPU){
	paquete paqCPU;
	void* mensaje = malloc((sizeof(int)*10)+nuevoPCB->tamContextoActual+nuevoPCB->tamEtiquetas);
	memcpy(mensaje, nuevoPCB->PID, sizeof(int));
	memcpy(mensaje+sizeof(int), nuevoPCB->ProgramCounter, sizeof(int));
	memcpy(mensaje+(sizeof(int)*2), nuevoPCB->paginas_Codigo, sizeof(int));
	memcpy(mensaje+(sizeof(int)*3), nuevoPCB->cod.comienzo, sizeof(int));
	memcpy(mensaje+(sizeof(int)*4), nuevoPCB->cod.offset, sizeof(int));
	memcpy(mensaje+(sizeof(int)*5), nuevoPCB->exitCode, sizeof(int));
	memcpy(mensaje+(sizeof(int)*6), nuevoPCB->tamContextoActual, sizeof(int));
	memcpy(mensaje+(sizeof(int)*7), nuevoPCB->contextoActual, nuevoPCB->tamContextoActual);
	memcpy(mensaje+(sizeof(int)*7)+nuevoPCB->tamContextoActual, nuevoPCB->tamEtiquetas, sizeof(int));
	memcpy(mensaje+(sizeof(int)*8)+nuevoPCB->tamContextoActual, nuevoPCB->etiquetas, nuevoPCB->tamEtiquetas);
	memcpy(mensaje+(sizeof(int)*8)+nuevoPCB->tamContextoActual+nuevoPCB->tamEtiquetas, nuevoPCB->tablaKernel.paginas, sizeof(int));
	memcpy(mensaje+(sizeof(int)*9)+nuevoPCB->tamContextoActual+nuevoPCB->tamEtiquetas, nuevoPCB->tablaKernel.pid, sizeof(int));
	memcpy(mensaje+(sizeof(int)*10)+nuevoPCB->tamContextoActual+nuevoPCB->tamEtiquetas, nuevoPCB->tablaKernel.tamaniosPaginas, sizeof(int));

	paqCPU.mensaje = mensaje;
	paqCPU.tamMsj = (sizeof(int)*10)+nuevoPCB->tamContextoActual+nuevoPCB->tamEtiquetas;
	paqCPU.tipoMsj = MENSAJE_PCB;

	if ((send(socketCPU, &paqCPU, sizeof(nuevoPCB), 0)) == -1) {
		perror("Error al enviar PCB al CPU");
		exit(-1);
	}

	free(paqCPU.mensaje);
}

//--------------------------------------FUNCIONES DE ARRAY----------------------------------------

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
		char* posibleFinalizarPrograma = string_substring_until(comando, strlen(finalizarPrograma));
		char* posibleConsultarEstado = string_substring_until(comando, strlen(consultarEstado));

		if(string_equals_ignore_case(posibleFinalizarPrograma,"finalizarPrograma")){
			char* valorPid=string_substring_from(comando,strlen(finalizarPrograma)+1);
			finalizarPrograma((int)valorPid);
		}else if(string_equals_ignore_case(comando, "consultarListado")){
			consultarListado();
		}else if(string_equals_ignore_case(posibleConsultarEstado, "consultarEstado")){
			char* estado=string_substring_from(comando,strlen(finalizarPrograma)+1);
			consultarEstado(estado);
		}else if(string_equals_ignore_case(comando, "detenerPlanificacion")){
			detenerPlanificacion();
		}else if(string_equals_ignore_case(comando, "reanudarPlanificacion")){
			reanudarPlanificacion();
		}else{ puts("Error de comando");
		}
		comando = realloc(comando,50*sizeof(char));
	}

	free(comando);
}

void* manejadorConexionConsola (void* socketDeConsola){
  while(1){
	  paquete* paqueteRecibidoDeConsola;
	  paqueteRecibidoDeConsola = recvRemasterizado(*(int*)socketDeConsola);
	      paquete* mensajeAux;
	      switch (paqueteRecibidoDeConsola->tipoMsj) {
			  case MENSAJE_CODIGO:
				  mensajeAux = recvRemasterizado(*(int*)socketDeConsola);
				  log_info(loggerKernel,"Archivo recibido correctamente...\n");
				  cantPag = recibirCantidadPaginas(mensajeAux->mensaje);
				  inicializarPCB(mensajeAux->mensaje,mensajeAux->tamMsj);
				  prepararProgramaEnMemoria(*(int*)socketParaMemoria,ASIGNAR_PAGINAS);
				  free(mensajeAux);
				  break;
			  case CORTO:
				  printf("El socket %d corto la conexion\n",*(int*)socketDeConsola);
				  verificarProcesos(*(int*)socketDeConsola);
				  close(*(int*)socketDeConsola);
				  puts("Te la comes igual que ivan el trolazo");
				  break;
			  case FINALIZAR_PROGRAMA:
				  mensajeAux = recvRemasterizado(*(int*)socketDeConsola);
				  finalizarPrograma(mensajeAux->mensaje);
				  break;
			  default:
					perror("No se reconoce el mensaje enviado por Consola");
			  }
	  }
  }

//-----------------------------------MANEJADOR MEMORIA----------------------------------
void *manejadorConexionMemoria (void* socketMemoria){
  while(1){
	  paquete* paqueteRecibidoDeMemoria;
	  paqueteRecibidoDeMemoria = recvRemasterizado(*(int*)socketMemoria);
	  paquete* mensajeAux;
	      switch (paqueteRecibidoDeMemoria->tipoMsj) {
	      	  case TAMANIO_PAGINA:
	      		mensajeAux = recvRemasterizado(*(int*)socketMemoria);
	      		TAM_PAGINA = mensajeAux->mensaje;
	      		  break;
	      	  case ESPACIO_INSUFICIENTE: ;
	      		  char* espacioInsuficiente = malloc(sizeof(char)*60);
	      		  espacioInsuficiente = "No hay espacio suficiente para ejecutar el programa";
	      		  sendRemasterizado(socketConsola, pid_actual, sizeof(int), &espacioInsuficiente);
	      		  disminuirPID();
	      		  free(espacioInsuficiente);
	      		  break;
	      	  case INICIO_EXITOSO: ;
	      	  	  TablaKernel *proceso;
	      	  	  t_proceso* t_proceso;
	      	  	  t_proceso = malloc(sizeof(t_proceso));
	      	  	  proceso->pid = pid_actual;
	      	  	  proceso->tamaniosPaginas = TAM_PAGINA;
	      	  	  proceso->paginas = cantPag;
	      	  	  list_add(tablaKernel, proceso);
	      	  	  log_info(loggerKernel,"Paginas disponibles para el proceso...\n");
	      	  	  prepararProgramaEnMemoria(*(int*)socketMemoria,INICIAR_PROGRAMA);
	      	  	  enviarPCB(socketCPU);
	      	  	  t_proceso->abortado=false;
	      	  	  t_proceso->pcb = nuevoPCB;
	      	  	  t_proceso->socket_CPU = socketCPU;
	      	  	  t_proceso->socket_CONSOLA = socketConsola;
	      	  	  queue_push(estado->listo,t_proceso);
	      	  	  sendRemasterizado(socketConsola, MENSAJE_PID, sizeof(int), t_proceso->pcb->PID);
	      	  	  break;
	      	  case LEER_DATOS: ;
	      		mensajeAux = recvRemasterizado(*(int*)socketMemoria);
	      		sendRemasterizado(socketCPU, LEER_DATOS, mensajeAux->tamMsj, &(mensajeAux->mensaje));
	      		free(mensajeAux);
	      		break;
	      	  default:
	      		  perror("No se reconoce el mensaje enviado por Memoria");
	      	  }
	  }
  }


//-----------------------------------MAIN-----------------------------------------------

int main(int argc, char *argv[]) {
	loggerKernel = log_create("Kernel.log", "Kernel", 0, 0);
	tablaKernel = list_create();
	verificarParametrosInicio(argc);
	//char *path = "Debug/kernel.config";
	//inicializarKernel(path);
	inicializarKernel(argv[1]);
	cola_CPU_libres = queue_create();
	sem_init(&sem_ready, 0, 0);
	pthread_t hiloManejadorMemoria, hiloManejadorTeclado, hiloManejadorConsola, hiloManejadorCPU;
	int socketParaFileSystem, socketMaxCliente, socketMaxMaster, socketAChequear, socketAEnviarMensaje, socketQueAcepta, bytesRecibidos;
	fd_set socketsCliente, socketsConPeticion, socketsMaster;
	FD_ZERO(&socketsCliente);
	FD_ZERO(&socketsConPeticion);
	FD_ZERO(&socketsMaster);
	mostrarConfiguracionesKernel();
	socketCPU = ponerseAEscucharClientes(puerto_Cpu, 0);
	socketConsola = ponerseAEscucharClientes(puerto_Prog, 0);
	socketParaMemoria = conectarAServer(ip_Memoria.numero, puerto_Memoria);
	socketParaFileSystem = conectarAServer(ip_FS.numero, puerto_FS);
	FD_SET(socketCPU, &socketsCliente);
	FD_SET(socketConsola, &socketsCliente);
	FD_SET(socketParaMemoria, &socketsMaster);
	FD_SET(socketParaFileSystem, &socketsMaster);
	socketMaxCliente = calcularSocketMaximo(socketCPU,socketConsola);
	socketMaxMaster = calcularSocketMaximo(socketParaMemoria,socketParaFileSystem);
	realizarHandshake(socketParaMemoria,MEMORIA);
	//realizarHandshake(socketParaFileSystem,FILESYSTEM);

	pthread_create(&hiloManejadorTeclado,NULL,manejadorTeclado,NULL);

	while (1) {
		socketsConPeticion = socketsCliente;
		if (select(socketMaxCliente + 1, &socketsConPeticion, NULL, NULL,NULL) == -1) {
			perror("Error de select");
			exit(-1);
		}
		for (socketAChequear = 0; socketAChequear <= socketMaxCliente;socketAChequear++) {
			if (FD_ISSET(socketAChequear, &socketsConPeticion)) {
				if (socketAChequear == socketConsola) {
					socketQueAcepta = aceptarConexionDeCliente(socketConsola);
					FD_SET(socketQueAcepta, &socketsMaster);
					FD_SET(socketQueAcepta, &socketsCliente);
					socketMaxCliente = calcularSocketMaximo(socketQueAcepta,socketMaxCliente);
					socketMaxMaster = calcularSocketMaximo(socketQueAcepta,socketMaxMaster);
				}
				else if(socketAChequear == socketCPU){
					socketQueAcepta = aceptarConexionDeCliente(socketCPU);
					FD_SET(socketQueAcepta, &socketsMaster);
					FD_SET(socketQueAcepta, &socketsCliente);
					socketMaxCliente = calcularSocketMaximo(socketQueAcepta,socketMaxCliente);
					socketMaxMaster = calcularSocketMaximo(socketQueAcepta,socketMaxMaster);
				}
				else {
					int tipoMsj;
					if(recv(socketAChequear,&tipoMsj,sizeof(int),0)==-1){
						perror("Error de recv en kernel");
						exit(-1);
					}else{
						switch (tipoMsj) {
							case CONSOLA:
								log_info(loggerKernel,"Se conecto nueva consola...\n");
								FD_CLR(socketConsola,&socketsCliente);
								pthread_create(&hiloManejadorConsola,NULL,manejadorConexionConsola,(void*)socketAChequear);
								break;
							case MEMORIA:
								pthread_create(&hiloManejadorMemoria,NULL,manejadorConexionMemoria,(void*)socketAChequear);
								FD_CLR(socketParaMemoria,&socketsMaster);
								break;
							case CPU:
								queue_push(cola_CPU_libres,socketCPU);
								//enviarAEjecutar(int pid);
								//socketCPU = comenzarAEjecutar(int pid);
								log_info(loggerKernel,"Se registro nueva CPU...\n");
								FD_CLR(socketCPU,&socketsCliente);
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
