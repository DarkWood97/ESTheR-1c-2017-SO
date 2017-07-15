/*
 * consola.c
 *
 *  Created on: 14/6/2017
 *      Author: utnso
 */

#include <pthread.h>
#include "funcionesGenericas.h"
#include "socket.h"
#include <stdio.h>

#define MENSAJE_CODIGO  103
#define MENSAJE_PID   105
#define HANDSHAKE_CONSOLA 1005
#define FINALIZAR_PROGRAMA 503
#define ESPACIO_INSUFICIENTE -2

typedef struct {
	_ip ip_Kernel;
	int puerto_kernel;
} consola;

typedef struct{
	int hora;
	int minuto;
	int segundo;
}tiempo;

typedef struct
{
	int pid;
	tiempo inicio;
	pthread_t hilo;
	int impresiones;
}programa;

t_log* loggerConsola;
t_list* listaProcesos;
paquete* paqueteHiloCorrecto;
pthread_mutex_t mutexImpresiones;
pthread_mutex_t mutexPaquete;
bool paqueteNoUsado;
int socketKernel;

//----FUNCIONES CONSOLA-------------------------------------------------------
consola consola_crear(t_config* configuracion) { //Chequear al abrir el archivo si no tiene error
	consola consola_auxiliar;
	consola_auxiliar.puerto_kernel = config_get_int_value(configuracion,"PUERTO_KERNEL");
	consola_auxiliar.ip_Kernel.numero = string_new();
	string_append(&(consola_auxiliar.ip_Kernel.numero),config_get_string_value(configuracion,"IP_KERNEL"));
	return consola_auxiliar;
}

consola inicializarPrograma(char* path) {
	t_config *configuracion = generarT_ConfigParaCargar(path);
	consola nueva_consola = consola_crear(configuracion);
	config_destroy(configuracion);
	return nueva_consola;
}
void mostrar_consola(consola aMostrar) {
	printf("IP=%s\n", aMostrar.ip_Kernel.numero);
	printf("PUERTO=%i\n", aMostrar.puerto_kernel);
}

//------FUNCIONES MENSAJES--------------------------------------------
void realizarHandshake()
{
	sendDeNotificacion(socketKernel, HANDSHAKE_CONSOLA);
}
//-------------------FUNCIONES DEL TIEMPO------------------------------
tiempo obtenerTiempo(char* unTiempo)
{
	char** tiempov;
	tiempo t;

	tiempov = malloc(15);
	tiempov = string_split(unTiempo, ":");

	t.hora = atoi(tiempov[0]);
	t.minuto = atoi(tiempov[1]);
	t.segundo = atoi(tiempov[2]);

	free(tiempov);
	return t;
}

tiempo get_tiempo_total(tiempo in, tiempo fin)
{
	tiempo aux;
	aux.hora = fin.hora - in.hora;
	aux.minuto = fin.minuto - in.minuto;
	aux.segundo = fin.segundo - in.segundo;
	aux.segundo=fabs(aux.segundo);

	return aux;
}
//---------------------FUNCIONES DE INTERFAZ----------------------------
long int obtenerTamanioArchivo(FILE* archivo){
	long int nTamArch;
	fseek (archivo, 0, SEEK_END);
	nTamArch = ftell (archivo);
	return nTamArch;
}

int obtenerPosicionHilo(){
	int i = 0;
	for(;i<list_size(listaProcesos);i++){
		programa* unPrograma = list_get(listaProcesos, i);
		if((unPrograma->hilo)==(pthread_self())){
			free(unPrograma);
			return i;
		}
	}
	return i;
}

char* leerArchivo(FILE *archivo, long int tamanio)
{
	fseek(archivo, 0, SEEK_SET);
	char* codigo = malloc(tamanio);
	fread(codigo, tamanio-1, 1, archivo);
	codigo[tamanio-1] = '\0';
	return codigo;
}

void eliminarPrograma(int pid){
	int i = 0;
	if(!(list_is_empty(listaProcesos))){
		for(;i<list_size(listaProcesos);i++){
			programa* unPrograma = list_get(listaProcesos,i);
			if(unPrograma->pid==pid){
				list_remove(listaProcesos, i);
			}
			free(unPrograma);
		}
	}
}

programa* buscarPrograma(int pid){

	bool chequearProgramaCorrecto(programa* unPrograma){
		if(unPrograma->pid == pid){
			return true;
		}
		return false;
	}

	if(list_any_satisfy(listaProcesos, (void*)chequearProgramaCorrecto)){
		programa* programaEncontrado = list_find(listaProcesos, (void*)chequearProgramaCorrecto);
		return programaEncontrado;
	}else{
		log_info(loggerConsola, "El proceso %d no se ha encontrado.", pid);
		return NULL;
	}
}

void imprimirInformacion(int pid,int impresiones,tiempo inicio,tiempo final){
	tiempo tiempoDuracion = get_tiempo_total(inicio,final);

	printf("Programa Finalizado: %d \n", pid);
	printf(">>El tiempo inicial del programa con pid: %i es %i:%i:%i\n",pid, inicio.hora, inicio.minuto,inicio.segundo);
	log_info(loggerConsola,">>El tiempo inicial del programa con pid: %i es %i:%i:%i\n",pid, inicio.hora, inicio.minuto,inicio.segundo);
	printf(">>El tiempo final del programa con pid:%i es %i:%i:%i\n", pid,final.hora, final.minuto,final.segundo);
	log_info(loggerConsola,">>El tiempo final del programa con pid:%i es %i:%i:%i\n", pid,final.hora, final.minuto,final.segundo);
	printf("Cantidad de impresiones por pantalla: %d \n", impresiones);
	printf(">>El tiempo total del programa con pid:%i es %i:%i:%i\n\n", pid,tiempoDuracion.hora, tiempoDuracion.minuto,tiempoDuracion.segundo);
	log_info(loggerConsola,">>el tiempo total del programa con pid:%i es %i:%i:%i\n", pid,tiempoDuracion.hora, tiempoDuracion.minuto,tiempoDuracion.segundo);
}

void finalizarHilo(programa* programaAFinalizar)
{
	if(pthread_cancel(programaAFinalizar->hilo) == 0)
	{
		log_debug(loggerConsola,"Se finalizo correctamente el hilo programa con PID= %d", programaAFinalizar->pid);
		pthread_join(programaAFinalizar->hilo, (void**) NULL);
	}else{
		log_error(loggerConsola,"No se pudo matar el hilo con PID= %d", programaAFinalizar->pid);
	}
}



void finalizarPrograma(int pid)
{
	sendRemasterizado(socketKernel, FINALIZAR_PROGRAMA, sizeof(int), &pid);

	programa* unPrograma = buscarPrograma(pid);
	if(unPrograma != NULL){
		finalizarHilo(unPrograma);
		char* final = malloc(1000);
		final = temporal_get_string_time();
		tiempo tiempoF = obtenerTiempo(final);
		imprimirInformacion(pid,unPrograma->impresiones,unPrograma->inicio,tiempoF);
		free(final);
		eliminarPrograma(pid);
		free(unPrograma);
	}
	else{
		printf("PID Ingresado: Incorrecto\n");
	}
}

void* iniciarPrograma(void* unPrograma)
{
	while(1){
		if(paqueteNoUsado){
			if((((programa*)unPrograma)->pid==paqueteHiloCorrecto->tipoMsj)){
				pthread_mutex_lock(&mutexPaquete);
				printf("Mensaje recibido: %p",paqueteHiloCorrecto->mensaje);
				((programa*)unPrograma)->impresiones++;
				list_replace(listaProcesos, obtenerPosicionHilo(), unPrograma);
				paqueteNoUsado = false;
				free(paqueteHiloCorrecto);
				pthread_mutex_unlock(&mutexPaquete);
			}
		}

		paquete* paqueteRecibido = recvRemasterizado(socketKernel);

		if(((programa*)unPrograma)->pid==paqueteRecibido->tipoMsj){
			pthread_mutex_lock(&mutexImpresiones);
			printf("Mensaje recibido: %p",paqueteRecibido->mensaje);
			((programa*)unPrograma)->impresiones++;
			list_replace(listaProcesos, obtenerPosicionHilo(), unPrograma);
			pthread_mutex_unlock(&mutexImpresiones);
		} else if(FINALIZAR_PROGRAMA==paqueteRecibido->tipoMsj){
			int pidAFinalizar, tamanio;
			memcpy(&pidAFinalizar, paqueteRecibido->mensaje, sizeof(int));
			memcpy(&tamanio,paqueteRecibido->mensaje+sizeof(int),sizeof(int));
			char* mensaje = malloc(tamanio);
			memcpy(&mensaje,paqueteRecibido->mensaje+(sizeof(int)*2),tamanio);
			printf("Mensaje recibido: %p",mensaje);
			finalizarPrograma(pidAFinalizar);
			free(mensaje);
			pthread_exit(NULL);
		}
		else{
			pthread_mutex_lock(&mutexPaquete);
			paqueteHiloCorrecto = malloc(sizeof(paqueteRecibido->tamMsj)+(sizeof(int)*2));
			int tamMsj = paqueteRecibido->tamMsj;
			int tipoMsj = paqueteRecibido->tipoMsj;
			char* mensaje = string_new();
			string_append(&mensaje,(char*)paqueteRecibido->mensaje);
			paqueteHiloCorrecto->tamMsj = tamMsj;
			paqueteHiloCorrecto->tipoMsj = tipoMsj;
			paqueteHiloCorrecto->mensaje = mensaje;
			paqueteNoUsado = true;
			pthread_mutex_unlock(&mutexPaquete);
		}

		free(paqueteRecibido);
	}
}

void recibirPID(long int tamanio,char* mensaje)
{
	char* mensajeAEnviar = string_new();
	string_append(&mensajeAEnviar,mensaje);

	sendRemasterizado(socketKernel, MENSAJE_CODIGO, tamanio, mensajeAEnviar);
	programa* unPrograma = malloc(sizeof(programa));
	bool PIDNoRecibido = true;

	while(PIDNoRecibido){
		paquete* paqueteRecibido = recvRemasterizado(socketKernel);
		if(paqueteRecibido->tipoMsj==MENSAJE_PID){
			pthread_mutex_unlock(&mutexImpresiones);
			char* tiempo=malloc(1000);
			tiempo = temporal_get_string_time();
			pthread_t hiloPrograma;
			int pid = *(int*)paqueteRecibido->mensaje;
			unPrograma->pid = pid;
			unPrograma->hilo = hiloPrograma;
			unPrograma->impresiones = 0;
			unPrograma->inicio = obtenerTiempo(tiempo);
			list_add(listaProcesos,unPrograma);
			pthread_create(&hiloPrograma,NULL,iniciarPrograma,(void*)unPrograma);
			PIDNoRecibido = false;
			free(tiempo);
			log_info(loggerConsola, "Se recibio correctamente el pid %d de kernel...", *(int*)paqueteRecibido->mensaje);
		} else if(paqueteRecibido->tipoMsj==ESPACIO_INSUFICIENTE){
			pthread_mutex_unlock(&mutexImpresiones);
			log_error(loggerConsola, "No hay espacio suficiente...");
			printf("No hay espacio suficiente para iniciar el programa\n");
			PIDNoRecibido = false;
		}
		else{
			pthread_mutex_lock(&mutexPaquete);
			paqueteHiloCorrecto = malloc(sizeof(paqueteRecibido->tamMsj)+(sizeof(int)*2));
			int tamMsj = paqueteRecibido->tamMsj;
			int tipoMsj = paqueteRecibido->tipoMsj;
			char* mensaje = string_new();
			string_append(&mensaje,(char*)paqueteRecibido->mensaje);
			paqueteHiloCorrecto->tamMsj = tamMsj;
			paqueteHiloCorrecto->tipoMsj = tipoMsj;
			paqueteHiloCorrecto->mensaje = mensaje;
			paqueteNoUsado = true;
			pthread_mutex_unlock(&mutexPaquete);
		}

	free(paqueteRecibido);
	}

	free(mensajeAEnviar);
}

void prepararPrograma(char* path){
	pthread_mutex_lock(&mutexImpresiones);
	FILE *archivoRecibido = fopen(path, "r");
	log_info(loggerConsola,"Archivo recibido correctamente...\n");

	long int tamanio = obtenerTamanioArchivo(archivoRecibido)+1;
	char* mensaje = leerArchivo(archivoRecibido,tamanio);

	fclose(archivoRecibido);

	recibirPID(tamanio,mensaje);
	free(mensaje);
}

void desconectarConsola()
{
	int i = 0;
	if(!(list_is_empty(listaProcesos))){
		int tamanio = list_size(listaProcesos);
		for(;i<tamanio;i++){
			programa* unPrograma  = list_get(listaProcesos, 0);
			int pid = unPrograma->pid;
			finalizarPrograma(pid);
			free(unPrograma);
		}
	}
	puts("Se desconecto la consola \n");
	puts("Se abortaron todos los programas");
}

void mostrarAyuda(bool programaIniciado)
{
	printf("Bienvenido al menu de opciones de la consola\n");
	printf("Escriba la opcion deseada: \n");
	printf("Iniciar Programa\n");
	if(programaIniciado){
		printf("Mostrar Programas Iniciados\n");
		printf("Finalizar Programa\n");
		printf("Desconectar Consola\n");
		printf("Limpiar Consola\n");
	}
}

void mostrarProgramasIniciados(){
	int i = 0;
	if(!(list_is_empty(listaProcesos))){
		for(;i<list_size(listaProcesos);i++){
			programa* unPrograma = list_get(listaProcesos,i);
			printf("Los programas son: \n");
			printf("PID-PROGRAMA: %d \n", unPrograma->pid);
			free(unPrograma);
		}
	}
}

void* manejadorInterfaz()
{
	char* comando = malloc(50*sizeof(char));
	size_t tamanioMaximo = 50;
	bool programaIniciado = false;
	paqueteNoUsado = false;

	while(1)
	{
		mostrarAyuda(programaIniciado);
		puts("Esperando comando...");

		getline(&comando, &tamanioMaximo, stdin);

		int tamanioRecibido = strlen(comando);
		comando[tamanioRecibido-1] = '\0';
		if(string_equals_ignore_case(comando,"Iniciar Programa"))
		{
			puts("Por favor ingrese la direccion del archivo a continuacion\n");
			comando = realloc(comando,50*sizeof(char));
			getline(&comando,&tamanioMaximo,stdin);
			int tamanioPathRecibido = strlen(comando);
			comando[tamanioPathRecibido-1] = '\0';
			prepararPrograma(comando);
			if(!(list_is_empty(listaProcesos))){
				programaIniciado = true;
			}
		}else if((string_equals_ignore_case(comando, "Mostrar Programas Iniciados"))&&(programaIniciado)){
			mostrarProgramasIniciados();
		}else if((string_equals_ignore_case(comando, "Finalizar Programa"))&&(programaIniciado)){
			puts("Ingrese el pid a finalizar\n");
			size_t tamanioComando = 10;
			comando = realloc(comando,10*sizeof(char));
			getline(&comando,&tamanioComando,stdin);
			int tamanioPidRecibido = strlen(comando);
			comando[tamanioPidRecibido-1] = '\0';
			int pid = atoi(comando);
			finalizarPrograma(pid);
			programaIniciado = !(list_is_empty(listaProcesos));
		}else if((string_equals_ignore_case(comando, "Desconectar Consola"))&&(programaIniciado)){
			desconectarConsola();
			programaIniciado = false;
		}else if((string_equals_ignore_case(comando, "Limpiar Consola"))){
			system("/usr/bin/clear");
		} else{
			puts("Error de comando");
			log_error(loggerConsola, "Error en el comando ingresado...");
		}
		comando = realloc(comando,50*sizeof(char));
	}
}
//------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	verificarParametrosInicio(argc);
	consola nuevaConsola = inicializarPrograma(argv[1]);
	//consola nuevaConsola = inicializarPrograma("Debug/consola.config");
	mostrar_consola(nuevaConsola);
	loggerConsola = log_create("consola.log", "consola", 0, 0);
	pthread_mutex_init(&mutexImpresiones,NULL);
	pthread_mutex_init(&mutexPaquete,NULL);
	listaProcesos = list_create();

	socketKernel = conectarAServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	realizarHandshake();
	log_info(loggerConsola,"Se conecto con el kernel...\n");

	pthread_t interfazUsuario;
	pthread_create(&interfazUsuario, NULL, manejadorInterfaz,NULL);

	pthread_join(interfazUsuario,(void**) NULL);

	pthread_mutex_destroy(&mutexImpresiones);
	pthread_mutex_destroy(&mutexPaquete);
	list_destroy(listaProcesos);
	log_destroy(loggerConsola);
	close(socketKernel);

	return EXIT_SUCCESS;
}
