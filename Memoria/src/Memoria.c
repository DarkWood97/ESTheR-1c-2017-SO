#include "funcionesGenericas.h"

#include "../Socket/src/socket.h"

typedef struct __attribute__((__packed__)){
	int tipoMsj;
	int tamMsj;
	void* mensaje;
}paquete;

//#include "socket.h"

#include <ctype.h>


#define FALTA_RECURSOS_MEMORIA -50
#define ESPACIO_INSUFICIENTE -2
#define ES_CPU 1001
#define HANG_UP_CPU -1001
#define ES_KERNEL 1002
#define INICIALIZAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define LEER_DATOS 504
#define ESCRIBIR_DATOS 505
#define INICIO_EXITOSO 50
#define SE_PUDO_COPIAR 2001
#define NO_SE_PUEDE_COPIAR -2001
#define DATOS_DE_PAGINA 103
#define TAMANIO_PAGINA_PARA_KERNEL 102
#define HANG_UP_KERNEL 0
#define LIBERAR_PAGINA 506

typedef struct __attribute__((packed)){
	int frame;
	int pid;
	int pagina;
}entradaTabla;


t_log * loggerMemoria;
void* memoriaSistema;
entradaTabla* entradasDeTabla;
void* punteroAPrincipioDeDatos;
pthread_mutex_t mutexTablaInvertida;

//-----VALORES DE MEMORIA--------//
int PUERTO;
int MARCOS;
int MARCOS_SIZE;
int ENTRADAS_CACHE;
char* REEMPLAZO_CACHE;
int CACHE_X_PROC;
int RETARDO_MEMORIA;
int cantidadPaginasDeTabla;

//-----FUNCIONES MEMORIA------------------------------------------------
void crearMemoria(t_config *configuracion) {
	verificarParametrosCrear(configuracion,7);
	PUERTO = config_get_int_value(configuracion, "PUERTO");
	MARCOS = config_get_int_value(configuracion, "MARCOS");
	MARCOS_SIZE = config_get_int_value(configuracion,"MARCO_SIZE");
	ENTRADAS_CACHE = config_get_int_value(configuracion,"ENTRADAS_CACHE");
	CACHE_X_PROC = config_get_int_value(configuracion,"CACHE_X_PROC");
	REEMPLAZO_CACHE = config_get_string_value(configuracion,"REEMPLAZO_CACHE");
	RETARDO_MEMORIA = config_get_int_value(configuracion,"RETARDO_MEMORIA");
}

void inicializarMemoria(char *path) {
	t_config *configuracionMemoria = (t_config*)malloc(sizeof(t_config));
	*configuracionMemoria = generarT_ConfigParaCargar(path);
	crearMemoria(configuracionMemoria);
	free(configuracionMemoria);
}

void mostrarConfiguracionesMemoria() {
	printf("PUERTO=%d\n", PUERTO);
	printf("MARCOS=%d\n", MARCOS);
	printf("MARCO_SIZE=%d\n", MARCOS_SIZE);
	printf("ENTRADAS_CACHE=%d\n", ENTRADAS_CACHE);
	printf("CACHE_X_PROC=%d\n", CACHE_X_PROC);
	printf("REEMPLAZO_CACHE=%s\n", REEMPLAZO_CACHE);
	printf("RETARDO_MEMORIA=%d\n", RETARDO_MEMORIA);
}

int calcularTamanioDeTabla(){
	long int tamanio_tabla = MARCOS*sizeof(entradaTabla);
	int cantidadPaginasDeTabla = tamanio_tabla/MARCOS_SIZE;
	if((tamanio_tabla%MARCOS_SIZE)!=0){
	    cantidadPaginasDeTabla++;
	  }
	return cantidadPaginasDeTabla;
}

void crearEstructuraAdministrativa(){ //Tabla de paginas invertidas
	cantidadPaginasDeTabla = calcularTamanioDeTabla();
	punteroAPrincipioDeDatos = memoriaSistema + cantidadPaginasDeTabla*MARCOS_SIZE;
	int paginaChequeada;
	for(paginaChequeada = 0; paginaChequeada<cantidadPaginasDeTabla;paginaChequeada++){
		entradasDeTabla[paginaChequeada].frame = paginaChequeada;
		entradasDeTabla[paginaChequeada].pid = -1;
		entradasDeTabla[paginaChequeada].pagina = paginaChequeada;
	}
	for(paginaChequeada = cantidadPaginasDeTabla; paginaChequeada<MARCOS; paginaChequeada++){
		entradasDeTabla[paginaChequeada].frame = paginaChequeada;
		entradasDeTabla[paginaChequeada].pid = -1;
		entradasDeTabla[paginaChequeada].pagina = -1;
	}
}

//-----------------------------FUNCIONES AUXILIARES DE MEMORIA-----------------------------------------//
bool ocupaCeroPaginas(int pagina){
	if(entradasDeTabla[pagina].pagina == -1){
		return true;
	}
	else{
		return false;
	}
}

bool noHayNingunProceso(int pagina){
	if(entradasDeTabla[pagina].pid == -1){
		return true;
	}else{
		return false;
	}
}

bool esNumero(char* cadenaAChequear){
	bool esNumero = true;
	int numeroDeCaracter;
	int tamanioNumero = string_length(cadenaAChequear);
	for(numeroDeCaracter = 0; numeroDeCaracter<tamanioNumero;numeroDeCaracter++){
		if(!isdigit(cadenaAChequear[numeroDeCaracter])){
			esNumero = false;
			return esNumero;
		}
	}
	return esNumero;
}

void retardo(int retardoRequerido){
	RETARDO_MEMORIA = retardoRequerido;
}

int obtenerCantidadDePaginasDe(int pid){ //Funcion sirve para asignar y para size
	int cantidadDePaginasDelProceso = 0;
	int paginaChequeada;
	for(paginaChequeada = 0; paginaChequeada<MARCOS; paginaChequeada++){
		if(entradasDeTabla[paginaChequeada].pid == pid){
			cantidadDePaginasDelProceso++;
		}
	}
	return cantidadDePaginasDelProceso;
}

//-------------------------COMANDOS DE MEMORIA Y FUNCIONES AUXILIARES PARA ELLOS------------------------//
//--------------------------------------DUMP-------------------------------------------------------------//

void copiarAArchivoDump(char* mensajeACopiar){
	int tamanioAEscribir = string_length(mensajeACopiar);
	FILE* dump = fopen("./dump.txt","a");
	fwrite(mensajeACopiar,tamanioAEscribir,1,dump);
	fclose(dump);
}

void copiarDatosProceso (int numeroDeFrame){
	char* cadenaDeCopiadoDeDatos = string_new();
	string_append(&cadenaDeCopiadoDeDatos,string_from_format("Datos pertenecientes al proceso %d,",entradasDeTabla[numeroDeFrame].pid));
	string_append(&cadenaDeCopiadoDeDatos,string_from_format(" numero de pagina %d,", entradasDeTabla[numeroDeFrame].pagina));
	string_append(&cadenaDeCopiadoDeDatos,string_from_format(" se encuentran en el frame %d y ",numeroDeFrame));
  //Obtengo los datos pertenecientes a la pagina
	long int comienzoDeDatos = numeroDeFrame*MARCOS_SIZE;
	void* datosDeProceso = malloc(MARCOS_SIZE);
	memcpy(datosDeProceso,entradasDeTabla+comienzoDeDatos,MARCOS_SIZE);
	string_append(&cadenaDeCopiadoDeDatos,string_from_format("posee los datos %p.\n", &datosDeProceso));
	printf("%s",cadenaDeCopiadoDeDatos);
	copiarAArchivoDump(cadenaDeCopiadoDeDatos);
	free(datosDeProceso);
	free(cadenaDeCopiadoDeDatos);
}

bool perteneceAlProceso(int pid, int numeroDeFrame){
	return entradasDeTabla[numeroDeFrame].pid == pid;
}

void copiarTituloContenidoMemoria(){
	char* cadenaDeEstructuracionDump = string_new();
	string_append(&cadenaDeEstructuracionDump,"\n----------DUMP CONTENIDO DE MEMORIA----------\n");
	printf("%s",cadenaDeEstructuracionDump);
	copiarAArchivoDump(cadenaDeEstructuracionDump);
	free(cadenaDeEstructuracionDump);
}

//------------------DUMP DE TODOS LOS DATOS

void dumpDeTodosLosDatos(){
	copiarTituloContenidoMemoria();
	int numeroDeFrame;
	pthread_mutex_lock(&mutexTablaInvertida);
	for(numeroDeFrame = 0; numeroDeFrame<MARCOS; numeroDeFrame++){
		if(!ocupaCeroPaginas(numeroDeFrame)){
			copiarDatosProceso(numeroDeFrame);
		}
	}
	pthread_mutex_unlock(&mutexTablaInvertida);
}

//-------------------------DUMP CONTENIDO DE PROCESO

void copiarTituloContenidoDeUnProceso(){
	char* cadenaDeEstructuracionDump = string_new();
	string_append(&cadenaDeEstructuracionDump,"\n----------DUMP CONTENIDO DE MEMORIA DE PROCESO----------\n");
	printf("%s",cadenaDeEstructuracionDump);
	copiarAArchivoDump(cadenaDeEstructuracionDump);
	free(cadenaDeEstructuracionDump);
}

void dumpDeUnProceso(int pid){
	copiarTituloContenidoDeUnProceso();
	int numeroDeFrame;
	pthread_mutex_lock(&mutexTablaInvertida);
	for(numeroDeFrame = 0; numeroDeFrame<MARCOS; numeroDeFrame++){
		if(perteneceAlProceso(pid, numeroDeFrame)){
			copiarDatosProceso(numeroDeFrame);
      //cadenaDeCopiadoDeDatos = string_from_vformat("Datos pertenecientes al proceso %d, numero de pagina %d son: %p",)
    }
  }
    pthread_mutex_unlock(&mutexTablaInvertida);
}


//-----------------------------DUMP ESTRUCTURAS DE MEMORIA

void copiarTituloDeEstructuras(){
	char* cadenaDeEstructuracionDump = string_new();
	string_append(&cadenaDeEstructuracionDump,"\n---------------DUMP ESTRUCTURAS--------------\n");
	printf("%s\n", cadenaDeEstructuracionDump);
	copiarAArchivoDump(cadenaDeEstructuracionDump);
	free(cadenaDeEstructuracionDump);
}

void copiarListaDeProcesosActivos(){
	int paginaDeProceso;
	char* tituloListaDeProcesos = string_new();
	string_append(&tituloListaDeProcesos,"Lista de procesos activos:\n");
	printf("%s",tituloListaDeProcesos);
	copiarAArchivoDump(tituloListaDeProcesos);
	free(tituloListaDeProcesos);
	for(paginaDeProceso = 0; paginaDeProceso<MARCOS; paginaDeProceso++){
		if (entradasDeTabla[paginaDeProceso].pid != -1) {
			char* procesoAImprimir = string_new();
			procesoAImprimir = string_itoa(entradasDeTabla[paginaDeProceso].pid);
			string_append(&procesoAImprimir,"\n");
			printf("%s", procesoAImprimir);
			copiarAArchivoDump(procesoAImprimir);
			free(procesoAImprimir);
    }
  }
}

void copiarTablaInvertidaADump(){
	char* cadenaAuxiliarDeCopiado = string_new();
	string_append(&cadenaAuxiliarDeCopiado,"\n|  #Frame  |  PID  |  #Pagina  |\n");
	printf("%s", cadenaAuxiliarDeCopiado);
	copiarAArchivoDump(cadenaAuxiliarDeCopiado);
	free(cadenaAuxiliarDeCopiado);
	int posicion;
	cadenaAuxiliarDeCopiado = string_new();
	for(posicion = 0; posicion<MARCOS; posicion++){
		string_append(&cadenaAuxiliarDeCopiado,string_from_format("|  %d  |  ",posicion));
		string_append(&cadenaAuxiliarDeCopiado,string_from_format("%d  |  ",entradasDeTabla[posicion].pid));
		string_append(&cadenaAuxiliarDeCopiado,string_from_format("%d  |\n",entradasDeTabla[posicion].pagina));
	}
	printf("%s", cadenaAuxiliarDeCopiado);
	copiarAArchivoDump(cadenaAuxiliarDeCopiado);
	free(cadenaAuxiliarDeCopiado);
	cadenaAuxiliarDeCopiado = string_new();
	string_append(&cadenaAuxiliarDeCopiado,"---------------------------------\n");
	printf("%s",cadenaAuxiliarDeCopiado);
	copiarAArchivoDump(cadenaAuxiliarDeCopiado);
	free(cadenaAuxiliarDeCopiado);
}


void dumpEstructurasDeMemoria(){
	copiarTituloDeEstructuras();
    pthread_mutex_lock(&mutexTablaInvertida);
    copiarTablaInvertidaADump();
    copiarListaDeProcesosActivos();
    pthread_mutex_unlock(&mutexTablaInvertida);
}

//--------------------------------SIZE--------------------------------------------------------//

int calcularCantFramesOcupados(){
	int cantidadDeFramesOcupados = 0;
	int frameChequeado;
	pthread_mutex_lock(&mutexTablaInvertida);
	for(frameChequeado = 0; frameChequeado<MARCOS; frameChequeado++){
		if(noHayNingunProceso(frameChequeado)){
			cantidadDeFramesOcupados++;
		}
	}
	pthread_mutex_unlock(&mutexTablaInvertida);
	return cantidadDeFramesOcupados;
}

void sizeMemoria(){
	char* tamanioDeMemoriaEnFrames = string_new();
	string_append(&tamanioDeMemoriaEnFrames,string_from_format("El tamanio de memoria en frames es %d,",MARCOS));
	string_append(&tamanioDeMemoriaEnFrames,string_from_format(" de tamanio %d.\n",MARCOS_SIZE));
	int cantidadDeFramesOcupados = calcularCantFramesOcupados();
	int cantidadDeFramesLibres = MARCOS - cantidadDeFramesOcupados;
	string_append(&tamanioDeMemoriaEnFrames,string_from_format("La cantidad de frames libres es %d,",cantidadDeFramesLibres));
	string_append(&tamanioDeMemoriaEnFrames,string_from_format(" y la cantidad de frames ocupados son %d.\n",cantidadDeFramesOcupados));
	printf("%s", tamanioDeMemoriaEnFrames);
	free(tamanioDeMemoriaEnFrames);
}

void sizeProceso(int pid){
	int cantidadDeFramesOcupados = obtenerCantidadDePaginasDe(pid);
	char* cadenaAImprimir = string_new();
	string_append(&cadenaAImprimir, string_from_format("La cantidad de paginas ocupadas por el proceso %d",pid));
	string_append(&cadenaAImprimir, string_from_format(" son %d",cantidadDeFramesOcupados));
	printf("%s\n", cadenaAImprimir);
	free(cadenaAImprimir);
}

//-------------------------FUNCIONES AUXILIARES PARA LOS MANEJADORES DE HILOS---------------------------//

//------------------------FUNCIONES AUXILIARES PARA HILO DE KERNEL--------------------------------------//

bool hayEspacioParaNuevoProceso(int cantidadDePaginasNecesarias){
	int paginaChequeada;
	int paginasDisponiblesEncontradas = 0;
	bool hayEspacio = false;
	usleep(RETARDO_MEMORIA);
	for(paginaChequeada = 0 ; paginaChequeada<MARCOS && paginasDisponiblesEncontradas<cantidadDePaginasNecesarias;paginaChequeada++){
		if(ocupaCeroPaginas(paginaChequeada)&&noHayNingunProceso(paginaChequeada)){
			paginasDisponiblesEncontradas++;
		}
	}
	if(paginasDisponiblesEncontradas>=cantidadDePaginasNecesarias){
		hayEspacio = true;
	}
	return hayEspacio;
}

bool espacioInsuficienteParaLaPagina(int tamanio, int offset){
	int tamanioRestanteEnPagina = MARCOS_SIZE-offset;
	if(tamanioRestanteEnPagina>tamanio){
		return false;
	}else{
		return true;
	}
}

int buscarFrameProceso(int pid, int numPagina){
	int numFrame, frameBuscado;
	usleep(RETARDO_MEMORIA);
	log_info(loggerMemoria, "Retardo...");
	log_info(loggerMemoria, "Buscando los datos en memoria...");
	for(numFrame = 0; numFrame<MARCOS; numFrame++){
		if(entradasDeTabla[numFrame].pagina == numPagina && entradasDeTabla[numFrame].pid == pid){
			log_info(loggerMemoria, "Datos encontrados.");
			frameBuscado = numFrame;
		}
	}
	return frameBuscado;
}

void almacenarBytesEnMemoria(int pid, int numPagina, int offset, int tamanio, void* buffer){
	int numFrameProceso = buscarFrameProceso(pid, numPagina);
	log_info(loggerMemoria, "Se encontro el frame asociado a la pagina numero %d del proceso %d.",numPagina, pid);
	long int aPartirDeDondeEscribir = numFrameProceso*MARCOS_SIZE + offset;
	memcpy(entradasDeTabla + aPartirDeDondeEscribir, buffer, tamanio);
}

void *leer(int pid, int pagina, int tamALeer, int offset){
	int numeroDeFrame;
	void* bufferDeLectura = malloc(tamALeer);
	numeroDeFrame = buscarFrameProceso(pid, pagina);
	long int comienzoDeLectura = numeroDeFrame*MARCOS_SIZE + offset;
	memcpy(bufferDeLectura, memoriaSistema + comienzoDeLectura, tamALeer);
  	return bufferDeLectura;
}

void reservarPaginasParaProceso(int pid, int cantidadDePaginas, int socketConPeticion){
	pthread_mutex_lock(&mutexTablaInvertida);
		if(!hayEspacioParaNuevoProceso(cantidadDePaginas)){
			sendDeNotificacion(socketConPeticion, ESPACIO_INSUFICIENTE);
		}else{
			int ultimaPaginaProceso = obtenerCantidadDePaginasDe(pid);
			int paginaChequeada, cantidadDePaginasAsignadas = 0;
			usleep(RETARDO_MEMORIA);
			for(paginaChequeada = 0; paginaChequeada<MARCOS && cantidadDePaginasAsignadas<cantidadDePaginas; paginaChequeada++){
				if(ocupaCeroPaginas(paginaChequeada)&&noHayNingunProceso(paginaChequeada)){
					entradasDeTabla[paginaChequeada].pid = pid;
					entradasDeTabla[paginaChequeada].pagina = ultimaPaginaProceso;
					ultimaPaginaProceso++;
					cantidadDePaginasAsignadas++;
				}
			}
			pthread_mutex_unlock(&mutexTablaInvertida);
			sendDeNotificacion(socketConPeticion, INICIO_EXITOSO);
		}
}

bool puedeSerLiberada(numeroDeFrame){
  if(cantidadPaginasDeTabla<numeroDeFrame<MARCOS){
    return true;
  }
  return false;
}
//-------------------------------------ASIGNAR PAGINAS PARA PROCESO---------------------------------//
void asignarPaginasAProceso(paquete* paqueteDeAsignacion, int socketConPeticionDeAsignacion){
	int pid, cantidadDePaginas;
	memcpy(&pid, paqueteDeAsignacion->mensaje, sizeof(int));
	memcpy(&cantidadDePaginas, paqueteDeAsignacion->mensaje + sizeof(int), sizeof(int));
	reservarPaginasParaProceso(pid,cantidadDePaginas,socketConPeticionDeAsignacion);
  //Fin del semaforo
}
//-----------------------------------INICIAR PROCESO----------------------------------------//
void inicializarProceso(paquete* paqueteParaIniciar, int socketConPeticionDeInicio){
	int pid, cantidadDePaginas;
	memcpy(&pid, paqueteParaIniciar->mensaje,sizeof(int));
	memcpy(&cantidadDePaginas, paqueteParaIniciar->mensaje+sizeof(int), sizeof(int));
	log_info(loggerMemoria,"Se ha encontrado espacio suficiente para inicializar el programa %d",pid);
	usleep(RETARDO_MEMORIA);
	log_info(loggerMemoria,"Retardo...");
	reservarPaginasParaProceso(pid, cantidadDePaginas,socketConPeticionDeInicio);
	free(paqueteParaIniciar);
}
//-----------------------------------FINALIZAR PROCESO---------------------------------------//
void finalizarProceso(paquete *paqueteDeFinalizacion, int socketConPeticionDeFinalizacion){
	int pidProceso;
	int numeroDePagina;
	memcpy(&pidProceso, paqueteDeFinalizacion->mensaje, sizeof(int));
	usleep(RETARDO_MEMORIA);
	pthread_mutex_lock(&mutexTablaInvertida);
	for(numeroDePagina = 0; numeroDePagina<MARCOS; numeroDePagina++){
		if(entradasDeTabla[numeroDePagina].pid == pidProceso){
			entradasDeTabla[numeroDePagina].pid = -1;
			entradasDeTabla[numeroDePagina].pagina = -1;
		}
	}
	pthread_mutex_unlock(&mutexTablaInvertida);
	log_info(loggerMemoria,"El proceso %d se ha finalizado.",pidProceso);
}
//---------------------------------ESCRIBIR DATOS EN MEMORIA--------------------------------//
void escribirDatos(paquete *paqueteDeEscritura ,int socketConPeticionDeEscritura){
	int pid, numPagina, offset, tamanio;
	void* bufferConDatos;
	memcpy(&pid, paqueteDeEscritura->mensaje, sizeof(int));
	memcpy(&numPagina,paqueteDeEscritura->mensaje + sizeof(int), sizeof(int));
	memcpy(&offset, paqueteDeEscritura->mensaje + 2*sizeof(int),sizeof(int));
	memcpy(&tamanio, paqueteDeEscritura->mensaje + 3*sizeof(int),sizeof(int));
	if(!espacioInsuficienteParaLaPagina(tamanio, offset)){
		bufferConDatos = malloc(tamanio);
		memcpy(bufferConDatos, paqueteDeEscritura->mensaje + 4*sizeof(int),tamanio);
		almacenarBytesEnMemoria(pid, numPagina, offset, tamanio, bufferConDatos);
		log_info(loggerMemoria, "Se ha copiado los datos en la pagina numero %d del proceso %d correctamente.",numPagina, pid);
		sendDeNotificacion(socketConPeticionDeEscritura, SE_PUDO_COPIAR);
	}else{
		sendDeNotificacion(socketConPeticionDeEscritura, NO_SE_PUEDE_COPIAR);
	}
}
//-----------------------------------------LEER DATOS DE MEMORIA------------------------------//

void leerDatos(paquete* paqueteDeLectura, int socketConPeticionDeLectura){ //Aca agrego el socket dependiendo el hilo que sea
	int pid, tamALeer, offset, pagina;
    memcpy(&pid, paqueteDeLectura->mensaje, sizeof(int));
    memcpy(&pagina, paqueteDeLectura->mensaje + sizeof(int), sizeof(int));
    memcpy(&tamALeer, paqueteDeLectura->mensaje + sizeof(int)*2, sizeof(int));
    memcpy(&offset, paqueteDeLectura->mensaje + sizeof(int)*3, sizeof(int));
    void* datosLeidos = leer(pid, pagina, offset, tamALeer);
    sendRemasterizado(socketConPeticionDeLectura, DATOS_DE_PAGINA, tamALeer, datosLeidos);
    //paquete paqueteAEnviar = leer(pid, pagina, offset, tamALeer); //SI ROMPE ES FACTIBLE QUE SEA ESTO
//    int tamPaquete = sizeof(int)*2 + tamALeer;
//    if(send(socketConPeticionDeLectura, &paqueteAEnviar, tamPaquete, 0) == -1){
//      perror("Error al enviar los datos leidos");
//      log_info(loggerMemoria, "Error al leer los datos solicitados por kernel");
//      exit(-1);
//    }
    free(datosLeidos);
}
//------------------------------LIBERAR PAGINA---------------------------------------//

/*
 * void liberarPagina(paquete *paqueteDeLiberacion, int socketConPeticionDeLiberacion){
  int pid, numPagina;
  memcpy(&pid, paqueteDeLiberacion->mensaje, sizeof(int));
  memcpy(&numPagina, paqueteDeLiberacion->mensaje + sizeof(int), sizeof(int));
  int numeroDeFrame = funcionHash(int pid, int numPagina);
  if(puedeSerLiberada(numeroDeFrame)){
    entradasDeTabla[numeroDeFrame].pid = -1;
    entradasDeTabla[numeroDeFrame].numPagina = -1;
    sendDeNotificacion(socketConPeticionDeLiberacion, SE_PUDO_LIBERAR);
  }else{
    sendDeNotificacion(socketConPeticionDeLiberacion, NO_SE_PUDO_LIBERAR);
  }
}
 *
 */
//------------------------------------------------HILOS MEMORIA-----------------------------------------//
//--------------------------------------------MANEJADOR TECLADO-----------------------------------------//

void *manejadorTeclado(){
  //Leo de consola, con un maximo de 50 caracteres Usar getline
	char* comando = malloc(50*sizeof(char));
	size_t tamanioMaximo = 50;
	while(1){
		puts("Esperando comando...");
		getline(&comando, &tamanioMaximo, stdin);
		int tamanioRecibido = strlen(comando);
		comando[tamanioRecibido-1] = '\0';
		if(string_equals_ignore_case(comando,"dump")){
			dumpDeTodosLosDatos();
		}else if(string_equals_ignore_case(comando, "size")){
      sizeMemoria();
    }else{
			char* posibleRetardo = string_substring_until(comando, 7);
			char* posibleFlush = string_substring_until(comando, 5);
			char* posibleDump = string_substring_until(comando, 4);
			char* posibleSize = string_substring_until(comando, 4);
			if(string_equals_ignore_case(posibleRetardo, "retardo")){  //
				char* valorARetardar = string_substring_from(comando, 8);
				if(!esNumero(valorARetardar)){
					perror("Error de valor de retardo");
				}else{
					int retardoRequerido = atoi(valorARetardar);
					retardo(retardoRequerido);
					log_info(loggerMemoria,"Se ha modificado el RETARDO_MEMORIA a %d",retardoRequerido);
				}
			}else if(string_equals_ignore_case(posibleFlush, "flush")){
    //Limpiar cache, queda para las proximas entregas
			}else if(string_equals_ignore_case(posibleDump, "dump")){
				char* queDumpear = string_new();
				queDumpear = string_substring_from(comando, 5);
				if(string_equals_ignore_case(queDumpear,"cache")){
				    //Dumpear cache
				}else if(string_equals_ignore_case(queDumpear,"estructuras")){
				    dumpEstructurasDeMemoria();
				}else if(esNumero(queDumpear)){
					int pid = atoi(queDumpear);
				    dumpDeUnProceso(pid);
				}else{
					perror("Error de segundo parametro de dump");
				}
				free(queDumpear);
			}else if(string_equals_ignore_case(posibleSize, "size")){
        char* procesoASizear = string_new();
        procesoASizear = string_substring_from(comando, 5);
        if(!esNumero(procesoASizear)){
          perror("Error del primer parametro en el size");
        }else{
          int pid = atoi(procesoASizear);
          free(procesoASizear);
          sizeProceso(pid);
        }
			}else{
				puts("Error de comando");
			}
			comando = realloc(comando,50*sizeof(char));
		}
	}
}
//-------------------------------------------MANEJADOR KERNEL------------------------------------//
void *manejadorConexionKernel(void* socketKernel){
	while(1){
		paquete *paqueteRecibidoDeKernel;
    //Chequear y recibir datos de kernel
		paqueteRecibidoDeKernel =  recvRemasterizado(*(int*)socketKernel);
		switch (paqueteRecibidoDeKernel->tipoMsj){
		case INICIALIZAR_PROGRAMA:
			log_info(loggerMemoria, "Ser recibio una peticion para inicializar un programa de kernel...");
			inicializarProceso(paqueteRecibidoDeKernel,*(int*)socketKernel);
			break;
		case ASIGNAR_PAGINAS:
			log_info(loggerMemoria, "Se recibio una peticion para asignar paginas de kernel...");
			asignarPaginasAProceso(paqueteRecibidoDeKernel, *(int*)socketKernel);
			break;
		case LEER_DATOS:
			log_info(loggerMemoria, "Se recibio una peticion de lectura por parte del kernel...");
			leerDatos(paqueteRecibidoDeKernel, *(int*)socketKernel);
		  break;
		case ESCRIBIR_DATOS:
			log_info(loggerMemoria, "Se recibio una peticion de escritura por parte del kernel...");
			escribirDatos(paqueteRecibidoDeKernel, *(int*)socketKernel);
			break;
		case LIBERAR_PAGINA:
			log_info(loggerMemoria, "Se recibio una peticion para liberar una pagina por parte de kernel...");
			//liberarPagina(paqueteRecibidoDeKernel, *(int*)socketKernel);
			break;
		case FINALIZAR_PROGRAMA:
			finalizarProceso(paqueteRecibidoDeKernel, *(int*)socketKernel);
			break;
		case HANG_UP_KERNEL:
			log_info(loggerMemoria, "Se ha cortado conexion con kernel.");
			log_info(loggerMemoria, "Cerrando memoria.");
			close(*(int*)socketKernel);
			exit(0);
		default:
			perror("No se recibio correctamente el mensaje");
		}
	}
}
//--------------------------------------------MANEJADOR CPU--------------------------------------//
void *manejadorConexionCPU (void *socketCPU){
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
	        case LEER_DATOS:
	          log_info(loggerMemoria,"Se recibio una peticion de lectura por parte de una CPU...");
	          leerDatos(paqueteRecibidoDeCPU.tamMsj, *(int*)socketCPU);
	          break;
	        case ESCRIBIR_DATOS:
	          log_info(loggerMemoria, "Se recibio una peticion de escritura por parte de una CPU...");
	          escribirDatos(paqueteRecibidoDeCPU.tamMsj, *(int*)socketCPU);
	          break;
	        case HANG_UP_CPU:
	        	log_info(loggerMemoria, "Se ha cortado conexion con el socket CPU %d ya que se cerro la CPU", socketCPU);
	        	close(*(int*)socketCPU);
	        	break;
	        default:
	        	perror("No se reconoce el mensaje enviado por CPU");
	      }
	  }
  }
//------------------------------------------FIN DE FUNCIONES MANEJADORAS DE HILOS------------------------//

//----------------------------------------MAIN--------------------------------------------//
int main(int argc, char *argv[]) {
	loggerMemoria = log_create("Memoria.log","Memoria",0,0);
	pthread_mutex_init(&mutexTablaInvertida,NULL);
	verificarParametrosInicio(argc);
	//char* path = "Debug/memoria.config";
	inicializarMemoria(argv[1]);
	paquete paqueteDeRecepcion, paqDePaginas;
	//inicializarMemoria(path);
	mostrarConfiguracionesMemoria();
	memoriaSistema = malloc(MARCOS*MARCOS_SIZE);
	entradasDeTabla= (entradaTabla*) memoriaSistema;
	crearEstructuraAdministrativa();
	log_info(loggerMemoria,"Se han inicializado las estructuras administrativas de memoria...");
	int socketEscuchaMemoria,socketClienteChequeado,socketAceptaClientes;
	pthread_t hiloManejadorKernel, hiloManejadorTeclado,hiloManejadorCPU; //Declaro hilos para manejar las conexiones

	fd_set aceptarConexiones, fd_setAuxiliar;
	FD_ZERO(&aceptarConexiones);
	FD_ZERO(&fd_setAuxiliar);
	socketEscuchaMemoria = ponerseAEscucharClientes(PUERTO, 0);
	FD_SET(socketEscuchaMemoria,&aceptarConexiones);
	int socketMaximo = socketEscuchaMemoria;

	//fd_setAuxiliar = aceptarConexiones;
	pthread_create(&hiloManejadorTeclado,NULL,manejadorTeclado,NULL); //Creo un hilo para atender las peticiones provenientes de la consola de memoria
	log_info(loggerMemoria,"Conexion a consola memoria exitosa...\n");
	while (1) { //Chequeo nuevas conexiones y las voy separando a partir del handshake recibido (CPU o Kernel).
		fd_setAuxiliar = aceptarConexiones;
		if(select(socketMaximo+1,&fd_setAuxiliar,NULL,NULL,NULL)==-1){
			perror("Error de select en memoria.");
			exit(-1);
		}
		for(socketClienteChequeado = 0; socketClienteChequeado<=socketMaximo; socketClienteChequeado++){
			if(FD_ISSET(socketClienteChequeado,&fd_setAuxiliar)){
				if(socketClienteChequeado == socketEscuchaMemoria){
					socketAceptaClientes = aceptarConexionDeCliente(socketEscuchaMemoria);
					FD_SET(socketAceptaClientes,&aceptarConexiones);
					socketMaximo = calcularSocketMaximo(socketAceptaClientes,socketMaximo);
					log_info(loggerMemoria,"Registro nueva conexion de socket: %d\n",socketAceptaClientes);
				}else{
						int quienEs = recvDeNotificacion(socketClienteChequeado);
						switch(quienEs){
							case ES_CPU:
								pthread_create(&hiloManejadorCPU,NULL,manejadorConexionCPU,(void *)socketClienteChequeado);
								log_info(loggerMemoria,"Se registro nueva CPU...\n");
								FD_CLR(socketClienteChequeado,&aceptarConexiones);
								break;
							case ES_KERNEL:
								sendRemasterizado(socketClienteChequeado, TAMANIO_PAGINA_PARA_KERNEL, sizeof(int), &MARCOS_SIZE);
								int* socketKernel = malloc(sizeof(int));
								*socketKernel = socketClienteChequeado;
								log_info(loggerMemoria,"Se registro conexion de Kernel...\n");
								log_info(loggerMemoria, "Handshake con kernel realizado.");
								pthread_create(&hiloManejadorKernel,NULL,manejadorConexionKernel,(void*)socketKernel);
								FD_CLR(socketClienteChequeado,&aceptarConexiones);
								break;
							default:
								puts("Conexion erronea");
								FD_CLR(socketClienteChequeado,&aceptarConexiones);
								close(socketClienteChequeado);
					}
				}
			}
		}
	}
}
