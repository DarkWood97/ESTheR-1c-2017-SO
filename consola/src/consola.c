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

typedef struct {
	_ip ip_Kernel;
	int puerto_kernel;
} Consola;

typedef struct
{
	int pid;
	struct timeval inicio;
	pthread_t hilo;
	int impresiones;
}Programa;

t_log* loggerConsola;
t_list* listaProcesos;
paquete* paqueteHiloCorrecto;
pthread_mutex_t mutexImpresiones;
pthread_mutex_t mutexPaquete;
int socketKernel;

//----FUNCIONES CONSOLA-------------------------------------------------------
Consola consola_crear(t_config* configuracion) { //Chequear al abrir el archivo si no tiene error
	Consola consola_auxiliar;
	consola_auxiliar.ip_Kernel.numero = config_get_string_value(configuracion,"IP_KERNEL");
	consola_auxiliar.puerto_kernel = config_get_int_value(configuracion,"PUERTO_KERNEL");
	return consola_auxiliar;
}

Consola inicializarPrograma(char* path) {
	t_config *configuracion = (t_config*)malloc(sizeof(t_config));
	*configuracion = generarT_ConfigParaCargar(path);
	Consola nueva_consola = consola_crear(configuracion);
	free(configuracion);
	return nueva_consola;
}
void mostrar_consola(Consola aMostrar) {
	printf("IP=%s\n", aMostrar.ip_Kernel.numero);
	printf("PUERTO=%i\n", aMostrar.puerto_kernel);
}

//------FUNCIONES MENSAJES--------------------------------------------
void realizarHandshake()
{
	int soyLeyenda = HANDSHAKE_CONSOLA;
	sendRemasterizado(socketKernel, HANDSHAKE_CONSOLA, sizeof(int), &soyLeyenda);
}
//-------------------FUNCIONES DEL TIEMPO------------------------------
int timeval_subtract(struct timeval *result, struct timeval *t2, struct timeval *t1)
{
    long int diff = (t2->tv_usec + 1000000 * t2->tv_sec) - (t1->tv_usec + 1000000 * t1->tv_sec);
    result->tv_sec = diff / 1000000;
    result->tv_usec = diff % 1000000;
    return (diff<0);
}
void timeval_print(struct timeval *tv)
{
    char buffer[30];
    time_t curtime;

    printf("%ld.%06ld", tv->tv_sec, tv->tv_usec);
    curtime = tv->tv_sec;
    strftime(buffer, 30, "%m-%d-%Y  %T", localtime(&curtime));
    printf(" = %s.%06ld\n", buffer, tv->tv_usec);
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
	for(;i<=list_size(listaProcesos);i++){
		if(list_get(listaProcesos, i)==(pthread_self())){
			return i;
		}
	}
}

char* leerArchivo(FILE *archivo, long int tamanio)
{
	fseek(archivo, 0, SEEK_SET);
	char *codigo = malloc(tamanio);
	fread(codigo, tamanio-1, 1, archivo);
	codigo[tamanio-1] = '\0';
	return codigo;
}

Programa* recibirPID(long int tamanio,Programa* programa,char* mensaje)
{
	sendRemasterizado(socketKernel, MENSAJE_CODIGO, tamanio, &mensaje);

	struct timeval inicio;
	bool PIDNoRecibido = true;
	paquete* paqueteRecibido;

	while(PIDNoRecibido){
		paqueteRecibido = recvRemasterizado(socketKernel);

		if(paqueteRecibido->tipoMsj==MENSAJE_PID){
			programa->pid = paqueteRecibido->mensaje;
			gettimeofday(&inicio, NULL);
			programa->inicio = inicio;
			programa->hilo = pthread_self();
			programa->impresiones = 0;
			list_add(listaProcesos,programa);
			PIDNoRecibido = false;
		}
	}
	free(paqueteRecibido);
	return programa;
}

void* iniciarPrograma(void* path)
{
	pthread_mutex_lock(&mutexImpresiones);
	FILE *archivoRecibido = fopen((char*) path, "r");
	log_info(loggerConsola,"Archivo recibido correctamente...\n");

	long int tamanio = obtenerTamanioArchivo(archivoRecibido)+1;
	char* mensaje = malloc(tamanio);
	mensaje = leerArchivo(archivoRecibido,tamanio);

	Programa* programa;
	programa = (Programa*) malloc(sizeof(Programa));
	programa = recibirPID(tamanio,programa,mensaje);
	pthread_mutex_unlock(&mutexImpresiones);

	paquete* paqueteRecibido;

	bool paqueteNoUsado = false;

	fclose(archivoRecibido);
	free((char*)path);

	while(1){
		if((programa->pid==paqueteHiloCorrecto->tipoMsj)&&(paqueteNoUsado)){
			pthread_mutex_lock(&mutexPaquete);
			printf("Mensaje recibido: %p",paqueteHiloCorrecto->mensaje);
			programa->impresiones++;
			list_replace(listaProcesos, obtenerPosicionHilo(), programa);
			paqueteNoUsado = false;
			free(paqueteHiloCorrecto);
			pthread_mutex_unlock(&mutexPaquete);
		}

		paqueteRecibido = recvRemasterizado(socketKernel);

		if(programa->pid==paqueteRecibido->tipoMsj){
			pthread_mutex_lock(&mutexImpresiones);
			printf("Mensaje recibido: %p",paqueteRecibido->mensaje);
			programa->impresiones++;
			list_replace(listaProcesos, obtenerPosicionHilo(), programa);
			free(paqueteRecibido);
			pthread_mutex_unlock(&mutexImpresiones);
		}
		else{
			pthread_mutex_lock(&mutexPaquete);
			paqueteHiloCorrecto = malloc(sizeof(paqueteRecibido));
			paqueteHiloCorrecto = paqueteRecibido;
			paqueteNoUsado = true;
			free(paqueteRecibido);
			pthread_mutex_unlock(&mutexPaquete);
			}
	}
}

Programa* buscarPrograma(int pid){
	int i = 0;
	for(;list_size(listaProcesos);i++){
		Programa* programa = list_get(listaProcesos, i);
		if(programa->pid == pid){
			list_remove(listaProcesos, i);
			return programa;
		}
		else{
			perror("No existe el programa con el pid indicado");
		}
	}
}

void imprimirInformacion(int pid,int impresiones,struct timeval *inicio,struct timeval *fin,struct timeval *duracion){
	printf("Programa finalizado: %d \n", pid);
	puts("Fecha y hora de inicio: \n ");
	timeval_print(&inicio);
	puts("Fecha y hora de finalizacion: \n");
	timeval_print(&fin);
	printf("Cantidad de impresiones por pantalla: %d \n", impresiones);
	puts("Tiempo total de ejecucion: ");
	timeval_print(&duracion);
}

void finalizarPrograma(int pid)
{
	sendRemasterizado(socketKernel, FINALIZAR_PROGRAMA, sizeof(int), &pid);

	Programa* programa = buscarPrograma(pid);
	pthread_join(programa->hilo, NULL);

	struct timeval fin;
	struct timeval duracion;
	gettimeofday(&fin, NULL);
	timeval_subtract(&duracion, &fin,&(programa->inicio));

	imprimirInformacion(pid,programa->impresiones,&(programa->inicio),&fin,&duracion);

	free(programa);
}

void desconectarConsola()
{
	int i = 0;
	int tamanio = list_size(listaProcesos);
	for(;i<=tamanio;i++){
		Programa* programa  = list_get(listaProcesos, 0);
		finalizarPrograma(programa->pid);
		free(programa);
	}
	puts("Se desconecto la consola \n");
	puts("Se abortaron todos los programas");
}

void mostrarAyuda(bool programaIniciado)
{
	printf("Bienvenido al menu de opciones de la consola\n");
	printf("Escriba la opcion deseada\n");
	printf("Iniciar programa\n");
	if(programaIniciado){
		printf("Mostrar Programas iniciados\n");
		printf("Finalizar programa\n");
		printf("Desconectar consola\n");
		printf("Limpiar consola\n");
	}
}

void mostrarProgramasIniciados(){
	int i = 0;
	for(;i<=list_size(listaProcesos);i++){
		Programa* programa = list_get(listaProcesos,i);
		printf("Programa: %d \n", programa->pid);
		free(programa);
	}
}

void* manejadorInterfaz()
{
	char* comando = malloc(50*sizeof(char));
	size_t tamanioMaximo = 50;
	bool programaIniciado = false;

	while(1)
	{
		mostrarAyuda(programaIniciado);
		puts("Esperando comando...");

		getline(&comando, &tamanioMaximo, stdin);

		int tamanioRecibido = strlen(comando);
		comando[tamanioRecibido-1] = '\0';
		if(string_equals_ignore_case(comando,"Iniciar Programa"))
		{
			comando = realloc(comando,50*sizeof(char));
			puts("Por favor ingrese la direccion del archivo a continuacion\n");
			getline(&comando,&tamanioMaximo,stdin);
			int tamanioPathRecibido = strlen(comando);
			comando[tamanioPathRecibido-1] = '\0';
			pthread_t hiloPrograma;
			programaIniciado = true;
			pthread_create(&hiloPrograma,NULL,iniciarPrograma,(void*)comando);
		}else if((string_equals_ignore_case(comando, "Mostrar Programas iniciados"))&&(programaIniciado)){
			mostrarProgramasIniciados();
		}else if((string_equals_ignore_case(comando, "Finalizar Programa"))&&(programaIniciado)){
			comando = realloc(comando,sizeof(int));
			puts("Ingrese el pid a finalizar\n");
			getline(&comando,sizeof(int),stdin);
			int tamanioPidRecibido = strlen(comando);
			comando[tamanioPidRecibido-1] = '\0';
			int pid = atoi(comando);
			finalizarPrograma(pid);
			programaIniciado = list_is_empty(listaProcesos);
		}else if((string_equals_ignore_case(comando, "Desconectar Consola"))&&(programaIniciado)){
			desconectarConsola();
			programaIniciado = false;
		}else if((string_equals_ignore_case(comando, "Limpiar Mensajes"))){
			system("clear");
		} else{
			puts("Error de comando");
		}
		comando = realloc(comando,50*sizeof(char));
	}
}
//------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[])
{
	verificarParametrosInicio(argc);
	Consola nuevaConsola = inicializarPrograma(argv[1]);
	//Consola nuevaConsola = inicializarPrograma("Debug/consola.config");
	mostrar_consola(nuevaConsola);
	loggerConsola = log_create("Consola.log", "Consola", 0, 0);
	pthread_mutex_init(&mutexImpresiones,NULL);
	pthread_mutex_init(&mutexPaquete,NULL);
	listaProcesos = list_create();

	socketKernel = conectarAServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	realizarHandshake();
	log_info(loggerConsola,"Se conecto con el kernel...\n");

	pthread_t interfazUsuario;
	pthread_create(&interfazUsuario, NULL, manejadorInterfaz,NULL);
	pthread_join(interfazUsuario,NULL);

	close(socketKernel);
	return EXIT_SUCCESS;
}
