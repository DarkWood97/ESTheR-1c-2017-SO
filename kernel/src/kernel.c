#include "funcionesGenericas.h"
#include "socket.h"
#include <stdio.h>
#include <math.h>
#include <parser/metadata_program.h>
#include <pthread.h>
#include <commons/collections/queue.h>
#include <semaphore.h>


#define NUEVO 1
#define LISTO 1
#define EJECUTANDO 1
#define BLOQUEADO 1
#define FINALIZADO 1

#define SOY_KERNEL 1002
#define FINALIZAR_PROGRAMA_MEMORIA 503
#define TAMANIO_PAGINA 102

//------------------------------------------------ESTRUCTURAS--------------------------------------//
typedef struct __attribute__((__packed__)) {
	uint32_t size;
	bool estaLibre;
}heap;


typedef struct __attribute__((__packed__)) {
	int pagina;
	int tamanioDisponible;
	t_list* bloqueHeapMetadata;
}paginaHeap;

typedef struct __attribute__((__packed__)) {
	int pid;
	t_list* pagina;
}tablaHeap;

typedef struct __attribute__((__packed_))
{
  int pagina;
  int offset;
  int tamanio;
} direccion;

typedef struct __attribute__((__packed_)){
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

typedef struct __attribute__((__packed_)) {
	int pid;
	int estado;
	int programCounter;
	t_intructions *indiceCodigo;
	int cantidadTIntructions;
	int cantidadPaginasCodigo; //Lo usa CPU?
	t_list* indiceStack;
	char* indice_etiquetas;
	tablaHeap* tablaHeap;
	int exitCode;
	int socket;
	int tamanioEtiquetas;
	bool estaAbortado;
	int rafagas;
	int tamanioContexto;
}PCB;

typedef struct __attribute__((__packed__)) {
	int FD;
	int globalFD;
	char* file;
	int open;
}tablaGlobalFS;

typedef struct __attribute__((__packed__)) {
	int pid;
	int FD;
	char* flags;
	int globalFD;
}tablaArchivosFS;

//-------------------------------------------VARAIBLES GLOBALES------------------------------

//LOGGER
t_log* loggerKernel;

//VARIABLES GLOBALES
int pidActual;
int socketMemoria;
int socketFS;
bool estadoPlanificacionesSistema;

//COLAS
t_queue* colaBloqueado;
t_queue* colaEjecutando;
t_queue* colaFinalizado;
t_queue* colaListo;
t_queue* colaNuevo;
t_queue* listaDeCPULibres;

//TABLAS FS
t_list* tablaAdminArchivos;
t_list* tablaAdminGlobal;


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
char** SEM_UNIT;
char** SHERED_VARS;
char** STACK_SIZE;

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
	SEM_UNIT = config_get_array_value(configuracion, "SEM_INIT");
	SHERED_VARS = config_get_array_value(configuracion, "SHARED_VARS");
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
	imprimirArrayDeInt(SEM_UNIT);
	printf("SHARED_VARS=");
	imprimirArrayDeChar(SHERED_VARS);
	printf("STACK_SIZE=%d\n", STACK_SIZE);
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

void inicializarStack(PCB *unPCB){
	unPCB->contextos = list_create();
	unPCB->contextos->args = list_create();
	unPCB->contextos->vars = list_create();
	unPCB->contextos->pos = 0;
	//HACE FALTA INICIALIZAR EL RETVAR?
}

char* cargarEtiquetas(t_metadata_program* metadata_program,int sizeEtiquetasIndex) {
	char* etiquetas = malloc(sizeEtiquetasIndex * sizeof(char));
	memcpy(etiquetas, metadata_program->etiquetas,sizeEtiquetasIndex * sizeof(char));
	return etiquetas;
}

//----------------------------------------PCB----------------------------------------------//




PCB* crearPCB(char* codigo,int tamanioCodigo,int cantPag, int socketConsola) {
	t_metadata_program* metadata_program = metadata_desde_literal(codigo);
	PCB* unPCB = malloc(sizeof(PCB));

	unPCB->pid=pidActual;
	unPCB->programCounter=0;
	unPCB->estaAbortado=false;
	unPCB->exitCode=0;
	unPCB->indiceCodigo=cargarCodeIndex(codigo,metadata_program);
	unPCB->cantidadPaginasCodigo=ceil((double)tamanioCodigo/(double)cantPag);
	inicializarStack(unPCB);
	unPCB->tamanioEtiquetas=metadata_program->instrucciones_size;
	unPCB->indice_etiquetas=cargarEtiquetas(metadata_program,unPCB->tamanioEtiquetas);
	unPCB->tablaHeap=malloc(sizeof(tablaHeap));
	unPCB->socket=socketConsola;
	unPCB->estado="NUEVO";

	pidActual++;

	log_debug(loggerKernel, "PCB creada para el pid: %i ",unPCB->pid);
	metadata_destruir(metadata_program);

	queue_push(colaNuevo,unPCB);
	log_debug(loggerKernel, "El pid: %i ha ingresado a la cola de nuevo.",unPCB->pid);

	return unPCB;
}

PCB *iniciarPCB(char *codigoDePrograma, int socketConsolaDuenio){
	int cantidadPaginasCodigo = obtenerCantidadPaginas(codigoDePrograma);
	void *inicioDePrograma = malloc(sizeof(int)*2);
	memcpy(inicioDePrograma, &contadorDePids, sizeof(int));
	memcpy(inicioDePrograma+sizeof(int), &cantidadPaginasCodigo, sizeof(int));
	int sePudo = mandarInicioAMemoria(inicioDePrograma);
	if(sePudo == OPERACION_CON_MEMORIA_EXITOSA){
		t_metadata_program *metadataDelPrograma = metadata_desde_literal(codigoDePrograma);
		PCB *pcbNuevo = malloc(sizeof(PCB));
		pcbNuevo->pid = contadorDePids;
		pcbNuevo->estado = NUEVO;
		pcbNuevo->programCounter = 0;
		pcbNuevo->rafagas = 0;
		pcbNuevo->cantidadPaginasCodigo = cantidadPaginasCodigo;
		pcbNuevo->cantidadDeIntructions = metadataDelPrograma->;
		pcbNuevo->indiceCodigo = cargarCodeIndex(codigoDePrograma, metadataDelPrograma);
		pcbNuevo->tamEtiquetas = metadataDelPrograma->etiquetas_size;
		pcbNuevo->indiceEtiquetas = cargarEtiquetas(metadataDelPrograma, pcbNuevo->tamEtiquetas);
		inicializarStack(pcbNuevo);
		pcbNuevo->tamContextos = 1;
		pcbNuevo->socket = socketConsolaDuenio;
		pcbNuevo->estaAbortado = false;
		pcbNuevo->tablaHeap = malloc(sizeof(tablaHeap));

		metadata_destruir(metadataDelPrograma);
		free(codigoDePrograma);
		contadorDePids++;
    	return pcbNuevo;
	}else{
		free(codigoDePrograma);
		return NULL;
	}
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
        memcpy(&contextoAAgregarAPCB->cod, paqueteConContexto->mensaje+dondeEstoy, sizeof(int));
        dondeEstoy +=sizeof(int);
        desserializarVariables(contextoAAgregarAPCB->args, paqueteConContexto, &dondeEstoy);
        desserializarVariables(contextoAAgregarAPCB->vars, paqueteConContexto, &dondeEstoy);
        memcpy(&contextoAAgregarAPCB->retPos, paqueteConContexto->mensaje + dondeEstoy, sizeof(int));
        memcpy(&contextoAAgregarAPCB->retVar, paqueteConContexto->mensaje + dondeEstoy + sizeof(int), sizeof(direccion));
        list_add(pcbConContexto->contextos, contextoAAgregarAPCB);
        dondeEstoy += sizeof(int);
    }
}

int desserializarTIntructions(PCB* pcbDesserializada, paquete *paqueteConPCB){
    int i, auxiliar = 0;
    memcpy(&pcbDesserializada->cantidadTIntructions, paqueteConPCB->mensaje+(sizeof(int)*3), sizeof(int));
    pcbDesserializada->indiceCodigo = malloc(pcbDesserializada->cantidadTIntructions*sizeof(t_intructions));
    for(i = 0; i<pcbDesserializada->cantidadTIntructions; i++){
        memcpy(&pcbDesserializada->indiceCodigo[auxiliar].start, paqueteConPCB->mensaje+sizeof(int)*(4+auxiliar), sizeof(int));
        auxiliar++;
        memcpy(&pcbDesserializada->indiceCodigo[auxiliar].offset, paqueteConPCB->mensaje+sizeof(int)*(4+auxiliar), sizeof(int));
        auxiliar++;
    }
    int retorno = sizeof(int)*(4+auxiliar);
    return retorno;
}

PCB* deserializarPCB(paquete* paqueteConPCB){
    PCB* pcbDesserializada = malloc(paqueteConPCB->tamMsj);
    memcpy(&pcbDesserializada->pid, paqueteConPCB->mensaje, sizeof(int));
    memcpy(&pcbDesserializada->programCounter, paqueteConPCB->mensaje+sizeof(int), sizeof(int));
    int dondeEstoy = desserializarTIntructions(pcbDesserializada, paqueteConPCB);
    memcpy(&pcbDesserializada->tamEtiquetas, paqueteConPCB->mensaje+dondeEstoy, sizeof(int));
    pcbDesserializada->etiquetas = string_new();
    memcpy(pcbDesserializada->etiquetas, paqueteConPCB->mensaje+sizeof(int)+dondeEstoy, pcbDesserializada->tamEtiquetas);
    memcpy(&pcbDesserializada->exitCode, paqueteConPCB->mensaje+sizeof(int)+dondeEstoy+pcbDesserializada->tamEtiquetas, sizeof(int));
    memcpy(&pcbDesserializada->tamContextos, paqueteConPCB->mensaje+sizeof(int)*2+dondeEstoy+pcbDesserializada->tamEtiquetas, sizeof(int));
    dondeEstoy +=sizeof(int)*3+pcbDesserializada->tamEtiquetas;
    pcbDesserializada->contextos = list_create();
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
    void* variableSerializada = malloc(list_size(variables)*sizeof(variable)+sizeof(int));
    int i;
    int cantidadDeVariables = list_size(variables);
    memcpy(variableSerializada, &cantidadDeVariables, sizeof(int));
    for(i = 0; i<cantidadDeVariables; i++){ //Antes i=1
        variable *variableObtenida = list_get(variables, i);
        memcpy(variableSerializada+sizeof(variable)*i, variableObtenida, sizeof(variable));
    }
    return variableSerializada;
}

int sacarTamanioDeLista(t_list* contexto){
    int i, tamanioDeContexto = 0;
    for(i = 0; i<list_size(contexto); i++){
        stack *contextoAObtener = list_get(contexto, i);
        tamanioDeContexto += sizeof(int)*4+sizeof(direccion)+list_size(contextoAObtener->args)*sizeof(variable)+list_size(contextoAObtener->vars)*sizeof(variable);
        //free(contextoAObtener);
    }
    return tamanioDeContexto;
}

void *serializarContextoActual(PCB* pcbConContextos){
    void* contextoSerializado = malloc(sacarTamanioDeLista(pcbConContextos->indiceStack));
  int i;
  for(i = 0; i<pcbConContextos->tamanioContexto; i++){
    stack* contexto = list_get(pcbConContextos->indiceStack, i);
    memcpy(contextoSerializado, &contexto->posicion, sizeof(int));
    void *argsSerializadas = serializarVariable(contexto->args);
    void *varsSerializadas = serializarVariable(contexto->vars);
    memcpy(contextoSerializado+sizeof(int), argsSerializadas, list_size(contexto->args)*sizeof(variable));
    memcpy(contextoSerializado+sizeof(int)+list_size(contexto->args)*sizeof(variable), varsSerializadas, list_size(contexto->vars)*sizeof(variable));
    memcpy(contextoSerializado+list_size(contexto->args)*sizeof(variable)+list_size(contexto->vars)*sizeof(variable), &contexto->retPos, sizeof(int));
  }
  return contextoSerializado;
}

int sacarTamanioPCB(PCB* pcbASerializar){
    int tamaniosDeContexto = sacarTamanioDeLista(pcbASerializar->indiceStack);
    int tamanio= sizeof(int)*7+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+tamaniosDeContexto+pcbASerializar->tamanioEtiquetas;
    return tamanio;
}

void *serializarPCB(PCB* pcbASerializar){
    int tamanioPCB = sacarTamanioPCB(pcbASerializar);
    void* pcbSerializada = malloc(tamanioPCB);
    memcpy(pcbSerializada, &pcbASerializar->pid, sizeof(int));
    memcpy(pcbSerializada+sizeof(int), &pcbASerializar->programCounter, sizeof(int));
    void* indiceDeCodigoSerializado = serializarT_Intructions(pcbASerializar);
    memcpy(pcbSerializada+sizeof(int)*2, &pcbASerializar->cantidadTIntructions, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*3, indiceDeCodigoSerializado, sizeof(t_intructions)*pcbASerializar->cantidadTIntructions);
    memcpy(pcbSerializada+sizeof(int)*3+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions, &pcbASerializar->tamanioEtiquetas, sizeof(int));
    memcpy(pcbSerializada+ sizeof(int)*4+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions, pcbASerializar->indice_etiquetas, pcbASerializar->tamanioEtiquetas); //CHEQUEAR QUE ES TAMETIQUETAS
    memcpy(pcbSerializada+sizeof(int)*4+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->rafagas, sizeof(int));
    void* contextoActualSerializado = serializarContextoActual(pcbASerializar);
    memcpy(pcbSerializada+sizeof(int)*5+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas, &pcbASerializar->tamanioContexto, sizeof(int));
    memcpy(pcbSerializada+sizeof(int)*6+sizeof(t_intructions)*pcbASerializar->cantidadTIntructions+pcbASerializar->tamanioEtiquetas+sacarTamanioDeLista(pcbASerializar->indiceStack), contextoActualSerializado, sacarTamanioDeLista(pcbASerializar->indiceStack));
    return pcbSerializada;
}



//--------------ADMINISTRACION DE PROGRAMAS---------------//
void enviarCodigoDeProgramaAMemoria(char* codigo){
	void *codigoParaMemoria;
	int i;
	int tamanioDeCodigoRestante = string_length(codigo);
	int cantidadDePaginasQueOcupa = tamanioDeCodigoRestante/TAMANIO_PAGINA;
	if((tamanioDeCodigoRestante % TAMANIO_PAGINA) != 0){
		cantidadDePaginasQueOcupa++;
	}
	for(i = 0; i<cantidadDePaginasQueOcupa; i++){
		int tamanioMensaje;
		codigoParaMemoria = malloc(tamanioDeCodigoRestante);
		memcpy(codigoParaMemoria, codigo+(i*TAMANIO_PAGINA), TAMANIO_PAGINA);
		if(tamanioDeCodigoRestante>TAMANIO_PAGINA){
			tamanioMensaje = TAMANIO_PAGINA;
		}else{
			tamanioMensaje = tamanioDeCodigoRestante;
		}
		sendRemasterizado(socketServidorMemoria, GUARDAR_BYTES, tamanioMensaje, codigoParaMemoria);
		free(codigoParaMemoria);
		tamanioDeCodigoRestante -= TAMANIO_PAGINA;
	}
}



//--------------------------------------HILOS-----------------------------------//
//------------------------------------MANEJADOR CONSOLA-------------------------------------------------/////////
void* manejadorConexionConsola(void* socketConsola){
	int socketDeConsola = *(int*)socketConsola;
}
//CONSOLA KERNEL
void modificarGradoMultiprogramacion(int grado){
	GRADO_MULTIPROG=grado;
}

void *manejadorTeclado(){
	char* comando = malloc(50*sizeof(char));
	size_t tamanioMaximo = 50;

	while(1){
		puts("Esperando comando...");

		getline(&comando, &tamanioMaximo, stdin);

		int tamanioRecibido = strlen(comando);
		comando[tamanioRecibido-1] = '\0';
		char* posibleFinalizarPrograma = string_substring_until(comando, strlen("finalizarPrograma"));
		char* posibleConsultarEstado = string_substring_until(comando, strlen("consultarEstado"));

		if(string_equals_ignore_case(posibleFinalizarPrograma,"finalizarPrograma")){
			char* valorPid=string_substring_from(comando,strlen("finalizarPrograma")+1);
			finalizarPrograma((int)valorPid);
		}else{
			if(string_equals_ignore_case(comando, "consultarListado")){
				consultarListado();
			}else{
				if(string_equals_ignore_case(posibleConsultarEstado, "consultarEstado")){
					char* estado=string_substring_from(comando,strlen("consultarEstado")+1);
					consultarEstado(estado);
				}else{
					if(string_equals_ignore_case(comando, "detenerPlanificacion")){
						detenerPlanificacion();
					}else{
						if(string_equals_ignore_case(comando, "reanudarPlanificacion")){
							reanudarPlanificacion();
						}else{
							if(string_equals_ignore_case(comando, "modificarGradoDeMultiprogramacion")){
								modificarGradoMultiprogramacion(grado);
							}else{
								puts("Error de comando");
							}
							comando = realloc(comando,50*sizeof(char));
						}
					}
				}
			}
		}
	}

	free(comando);
}



//PLANIFICACION DEL SISTEMA

void manejarColaNueva(){
	if(queue_size(colaNuevo)<GRADO_MULTIPROG){
		PCB* pcbNueva=malloc(sizeof(PCB));
		pcbNueva=queue_pop(colaNuevo);
		queue_push(colaListo,pcbNueva);
		log_info(loggerKernel,"Se ha agregado la PCB del proceso %d a la cola de Listo", pcbNueva->pid);
	}else{
		log_info(loggerKernel,"No se ha podido agregar ningun proceso a la cola de listo por grado de multiprogramacion.");
	}

}


void planificarSistema(){
	while(1){
		while(estadoPlanificacionesSistema){
			if(!queue_is_empty(colaNuevo)){
				manejarColaNueva();
			}

		}
	}
}

//CPU
bool mismoPath(tablaGlobalFS* entradaGFS){
	return string_equals_ignore_case(entradaGFS->file,);
}


void agregarEntradaTG(char* path){
	tablaGlobalFS entradaGFS;
	if(list_find(tablaAdminGlobal,(void*)mismoPath){

	}
}


void abrirArchivo(pid,path,flags){
	if(validarArchivo(path)){
		agregarEntradaTG(path);
	}
}

void *manejadorCPU(void* socket){
  int socketCPU = *(int*)socket;
  bool estaOcupadaCPU = false;
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
  char* path=string_new();
  int tamanioPath;
  t_banderas* flags=malloc(sizeof(t_banderas));


  while(estaOcupadaCPU){
    paquete *paqueteRecibidoDeCPU;
    paqueteRecibidoDeCPU = recvRemasterizado(socketCPU);
    switch (paqueteRecibidoDeCPU->tipoMsj) {
      case WAIT_SEMAFORO:

        break;
      case SIGNAL_SEMAFORO:
        signalSemaforo((char*)paqueteRecibidoDeCPU->mensaje);
        break;
      case RESERVAR_HEAP:

        break;
      case LIBERAR_HEAP:

        break;
      case LEER_DATOS_ARCHIVO:
        leerDatos(procesoParaCPU->pcb->pid, paqueteRecibidoDeCPU->mensaje);
        break;
      case ESCRIBIR_DATOS_ARCHIVO:

        break;
      case PROCESO_ABORTADO:
        pcbAfectado = desserializarPCB(paqueteRecibidoDeCPU);
        pcbAfectado->estaAbortado=true;
        pcbAfectado->exitCode=-10;//DEFINIR EXIT CODE
        queue_push(colaFinalizado, pcbAfectado);
        sendRemasterizado(socketMemoria, FINALIZAR_PROCESO_MEMORIA, sizeof(int), &pcbAfectado->pid);// PROBAR SI FUNCIONA
        estaOcupadaCPU = false;
        break;
      case PROCESO_FINALIZADO:
    	pcbAfectado= desserializarPCB(paqueteRecibidoDeCPU);
    	pcbAfectado->estado=false;
    	pcbAfectado->exitCode=0;
        queue_push(colaFinalizado, pcbAfectado);
        estaOcupadaCPU = false;
        break;
      case PROCESO_BLOQUEADO:
    	pcbAfectado = desserializarPCB(paqueteRecibidoDeCPU);
    	pcbAfectado->estaAbortado=false;
        queue_push(colaBloqueado, pcbAfectado);
        estaOcupadaCPU = false;
        break;
      case ABRIR_ARCHIVO:
    	  paquete* paqueteAbrirArchivo;
    	  paqueteAbrirArchivo=recvRemasterizado(socketCPU);
    	  memcpy(pid,paqueteAbrirArchivo->mensaje+sizeof(int),sizeof(int));
    	  tamanioPath=paqueteAbrirArchivo->tamMsj-sizeof(int)-sizeof(t_banderas);
    	  memcpy(path,paqueteAbrirArchivo->mensaje+sizeof(int)+tamanioPath,tamanioPath);
    	  memcpy(flags,paqueteAbrirArchivo->mensaje+sizeof(int)+tamanioPath,sizeof(t_banderas),sizeof(t_banderas));
    	  abrirArchivo(pid,path,flags);
    	  break;
      case BORRAR_ARCHIVO:
        break;
      case CERRAR_ARCHIVO:
        break;
      case MOVER_CURSOR:
        break;
      case OBTENER_ARCHIVO:
        break;
      case GUARDAR_DATOS_ARCHIVO:
        break;
    }
  }
}



//--------------------------------------HANDSHAKES-----------------------------------//
void realizarHandshakeMemoria(int socket) {
	sendDeNotificacion(socket, SOY_KERNEL_MEMORIA);
	paquete* paqueteConPaginas = recvRemasterizado(socket);
	if(paqueteConPaginas->tipoMsj == TAMANIO_PAGINA){
		TAMANIO_PAGINA = *(int*)paqueteConPaginas->mensaje;
	}else{
		log_info(loggerKernel, "Error al recibir el tamanio de pagina de memoria.");
		exit(-1);
	}
	free(paqueteConPaginas);
}

void realizarhandshakeCPU(int socket){
	void *buffer = malloc(string_length(ALGORITMO)+sizeof(int)*2);
	memcpy(buffer, ALGORITMO, string_length(ALGORITMO));
	if(string_equals_ignore_case(ALGORITMO, "RR")){
		memcpy(buffer+string_length(ALGORITMO), &QUANTUM, sizeof(int));
	}else{
		int quantumDeMentira = -1;
		memcpy(buffer+string_length(ALGORITMO), &quantumDeMentira, sizeof(int));
	}
	memcpy(buffer+string_length(ALGORITMO)+sizeof(int), &QUANTUM_SLEEP, sizeof(int));
	sendRemasterizado(socket, HANDSHAKE_CPU, string_length(ALGORITMO)+sizeof(int)*2, buffer);
	free(buffer);
}

//--------------------------------------MAIN-----------------------------------//

int main (int argc, char *argv[]){
	loggerKernel = log_create("Kernel.log", "Kernel", 0, 0);
	//PUESTA EN MARCHA
	verificarParametrosInicio(argc);
	inicilizarKernel(argv[1]);
	char *path = "Debug/kernel.config";
	inicializarKernel(path);

	//INICIALIZANDO VARIABLES
	pidActual = 0;
	int socketEscuchaCPU, socketEscuchaConsolas, socketMaxCliente, socketAChequear, socketQueAceptaClientes;
	int *socketConsola, socketCPU;
	pthread_t hiloManejadorTeclado, hiloManejadorConsola, hiloManejadorCPU, hiloManejadorPlanificacion;
	fd_set socketsCliente, socketsAuxiliaresCliente;
	FD_ZERO(&socketsCliente);
	FD_ZERO(&socketsAuxiliaresCliente);
	mostrarConfiguracionesKernel();
	estadoPlanificacionesSistema=true;

	//INICIALIZANDO LISTAS FS
	tablaAdminArchivos=list_create();
	tablaAdminGlobal=list_create();

	//INICIALIZANDO COLAS
	colaBloqueado = queue_create();
	colaEjecutando = queue_create();
	colaFinalizado = queue_create();
	colaListo = queue_create();
	colaNuevo = queue_create();
	listaDeCPULibres=queue_create();

	//CONEXIONES
	socketEscuchaCPU = ponerseAEscucharClientes(PUERTO_CPU, 0);
	socketEscuchaConsolas = ponerseAEscucharClientes(PUERTO_CONSOLA, 0);
	socketMemoria = conectarAServer(IP_MEMORIA, PUERTO_MEMORIA);
	socketFS = conectarAServer(IP_FS, PUERTO_FS);
	FD_SET(socketEscuchaCPU, &socketsCliente);
	FD_SET(socketEscuchaConsolas, &socketsCliente);
	socketMaxCliente = calcularSocketMaximo(socketEscuchaConsolas, socketEscuchaCPU);
	realizarHandshakeMemoria(socketMemoria);
	realizarHandshakeFS(socketFS);

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
							pthread_create(&hiloManejadorCPU, NULL, manejadorCPU, (void*)socketCPU);
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
