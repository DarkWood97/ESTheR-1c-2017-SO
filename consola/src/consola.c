#include <pthread.h>
#include "funcionesGenericas.h"
#include "socket.h"
#include <stdio.h>

#define MENSAJE_CODIGO  103
#define MENSAJE_PID   105
#define HANDSHAKE_CONSOLA 1005
#define FINALIZAR_PROGRAMA 503
#define ESPACIO_INSUFICIENTE -2
#define IMPRIMIR_INFO 605

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
t_list* listaHilosCorrectos;
pthread_mutex_t mutexProcesos;
pthread_mutex_t mutexHilosCorrectos;
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
		int tamanio = list_size(listaProcesos);
		for(;i<tamanio;i++){
			programa* unPrograma = list_get(listaProcesos,i);
			if(unPrograma->pid==pid){
				list_remove(listaProcesos, i);
			}
			free(unPrograma);
			tamanio = list_size(listaProcesos);
			i--;
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
	programa* unPrograma = buscarPrograma(pid);
	if(unPrograma != NULL){
		sendRemasterizado(socketKernel, FINALIZAR_PROGRAMA, sizeof(int), &pid);
		finalizarHilo(unPrograma);
		char* final = malloc(1000);
		final = temporal_get_string_time();
		tiempo tiempoF = obtenerTiempo(final);
		imprimirInformacion(pid,unPrograma->impresiones,unPrograma->inicio,tiempoF);
		free(final);
		eliminarPrograma(pid);
	}
	else{
		printf("PID Ingresado: Incorrecto\n");
	}
}

void aniadirPaqueteALaLista(paquete* unPaqueteRecibido){
	paquete* unPaqueteAAniadir = malloc(sizeof(int)*2+unPaqueteRecibido->tamMsj);
	int tipoDeMsj = unPaqueteRecibido->tipoMsj;
	int tamanioDeMsj = unPaqueteRecibido->tamMsj;
	void* mensaje = malloc(tamanioDeMsj);
	memcpy(mensaje,unPaqueteRecibido->mensaje,tamanioDeMsj);
	unPaqueteAAniadir->tipoMsj = tipoDeMsj;
	unPaqueteAAniadir->tamMsj = tamanioDeMsj;
	unPaqueteAAniadir->mensaje = mensaje;
	list_add(listaHilosCorrectos,unPaqueteAAniadir);
}

void imprimirMensajeEspacioInsuficiente(){
	log_error(loggerConsola, "No hay espacio suficiente...");
	printf("No hay espacio suficiente para iniciar el programa\n");
}

bool imprimirInformacionPaquete(paquete* paqueteRecibido, programa* unPrograma){
	int pidPrograma = ((programa*)unPrograma)->pid;
	int pidImpresion;
	memcpy(&pidImpresion,paqueteRecibido->mensaje,sizeof(int));
	if(pidImpresion==pidPrograma){
		int tamanioInfo;
		memcpy(&tamanioInfo,paqueteRecibido->mensaje+sizeof(int),sizeof(int));
		char* infoAImprimir = malloc(tamanioInfo);
		memcpy(infoAImprimir,paqueteRecibido->mensaje+(sizeof(int)*2),tamanioInfo);
		printf("Mensaje recibido: %s",infoAImprimir);
		free(infoAImprimir);
		pthread_mutex_lock(&mutexProcesos);
		((programa*)unPrograma)->impresiones++;
		list_replace(listaProcesos, obtenerPosicionHilo(), unPrograma);
		pthread_mutex_unlock(&mutexProcesos);
		return true;
	}
	else{
		return false;
	}
}

void manejarHiloCorrespondiente(programa* unPrograma){
	while(!list_is_empty(listaHilosCorrectos)){
		paquete* paqueteHiloAdecuado = list_get(listaHilosCorrectos, 0);
		if(IMPRIMIR_INFO==paqueteHiloAdecuado->tipoMsj){
			if(imprimirInformacionPaquete(paqueteHiloAdecuado,unPrograma)){
				list_remove(listaHilosCorrectos, 0);
				destruirPaquete(paqueteHiloAdecuado);
			}
		}
		else if(FINALIZAR_PROGRAMA==paqueteHiloAdecuado->tipoMsj){
			int pidAFinalizar;
			int pidPrograma = ((programa*)unPrograma)->pid;
			memcpy(&pidAFinalizar,paqueteHiloAdecuado->mensaje,sizeof(int));
			if(pidPrograma==pidAFinalizar){
				int tamanioInfo;
				memcpy(&tamanioInfo,paqueteHiloAdecuado->mensaje+sizeof(int),sizeof(int));
				char* infoAImprimir = malloc(tamanioInfo);
				memcpy(infoAImprimir,paqueteHiloAdecuado->mensaje+(sizeof(int)*2),tamanioInfo);
				printf("Mensaje recibido: %s",infoAImprimir);
				free(infoAImprimir);
				list_remove(listaHilosCorrectos, 0);
				destruirPaquete(paqueteHiloAdecuado);
				pthread_mutex_lock(&mutexProcesos);
				((programa*)unPrograma)->impresiones++;
				list_replace(listaProcesos, obtenerPosicionHilo(), unPrograma);
				pthread_mutex_unlock(&mutexProcesos);
				finalizarPrograma(pidAFinalizar);
				pthread_exit(NULL);
			}
		}
	}
}

void* administrarPrograma(void* unPrograma)
{
	while(1){

		manejarHiloCorrespondiente(unPrograma);

		paquete* paqueteRecibido = recvRemasterizado(socketKernel);

		if(IMPRIMIR_INFO==paqueteRecibido->tipoMsj){
			if(!imprimirInformacionPaquete(paqueteRecibido, unPrograma)){
				aniadirPaqueteALaLista(paqueteRecibido);
			}
		} else if(FINALIZAR_PROGRAMA==paqueteRecibido->tipoMsj){
			int pidAFinalizar;
			int pidPrograma = ((programa*)unPrograma)->pid;
			memcpy(&pidAFinalizar,paqueteRecibido->mensaje,sizeof(int));
				if(pidPrograma==pidAFinalizar){
					int tamanioInfo;
					memcpy(&tamanioInfo,paqueteRecibido->mensaje+sizeof(int),sizeof(int));
					char* infoAImprimir = malloc(tamanioInfo);
					memcpy(infoAImprimir,paqueteRecibido->mensaje+(sizeof(int)*2),tamanioInfo);
					printf("Mensaje recibido: %s",infoAImprimir);
					free(infoAImprimir);
					pthread_mutex_lock(&mutexProcesos);
					((programa*)unPrograma)->impresiones++;
					list_replace(listaProcesos, obtenerPosicionHilo(), unPrograma);
					pthread_mutex_unlock(&mutexProcesos);
					finalizarPrograma(pidAFinalizar);
					pthread_exit(NULL);
				}
				else{
					aniadirPaqueteALaLista(paqueteRecibido);
				}
		}
		else{
			aniadirPaqueteALaLista(paqueteRecibido);
		}

		destruirPaquete(paqueteRecibido);
	}
}

void crearPrograma(paquete* unPaqueteRecibido){
	programa* unPrograma = malloc(sizeof(programa));
	char* tiempo=malloc(1000);
	tiempo = temporal_get_string_time();
	pthread_t hiloPrograma;
	int pid =*(int*)unPaqueteRecibido->mensaje;
	unPrograma->pid = pid;
	unPrograma->hilo = hiloPrograma;
	unPrograma->impresiones = 0;
	unPrograma->inicio = obtenerTiempo(tiempo);
	pthread_mutex_lock(&mutexProcesos);
	list_add(listaProcesos,unPrograma);
	pthread_mutex_unlock(&mutexProcesos);
	pthread_create(&hiloPrograma,NULL,administrarPrograma,(void*)unPrograma);
	free(tiempo);
	log_info(loggerConsola, "Se recibio correctamente el pid %d de kernel...", pid);
}

void* manejadorPaquetes(){
	while(1){
		int i;
		pthread_mutex_lock(&mutexHilosCorrectos);
		if(!list_is_empty(listaHilosCorrectos)){
			int tamanio = list_size(listaHilosCorrectos);
			for(i=0;i<tamanio;i++){
				paquete* paqueteHiloAdecuado = list_get(listaHilosCorrectos, i);
					if((MENSAJE_PID==paqueteHiloAdecuado->tipoMsj)){
						crearPrograma(paqueteHiloAdecuado);
						list_remove(listaHilosCorrectos, i);
						destruirPaquete(paqueteHiloAdecuado);
						tamanio = list_size(listaHilosCorrectos);
						i--;
					}
					else if(ESPACIO_INSUFICIENTE==paqueteHiloAdecuado->tipoMsj){
						imprimirMensajeEspacioInsuficiente();
						list_remove(listaHilosCorrectos, i);
						destruirPaquete(paqueteHiloAdecuado);
						tamanio = list_size(listaHilosCorrectos);
						i--;
					}
			}
		}
		pthread_mutex_unlock(&mutexHilosCorrectos);
	}
}

void recibirPID(long int tamanio,char* mensaje)
{
	char* mensajeAEnviar = string_new();
	string_append(&mensajeAEnviar,mensaje);

	sendRemasterizado(socketKernel, MENSAJE_CODIGO, tamanio, mensajeAEnviar);
	free(mensaje);
	free(mensajeAEnviar);

	paquete* paqueteRecibido = recvRemasterizado(socketKernel);

	if(paqueteRecibido->tipoMsj==MENSAJE_PID){
		crearPrograma(paqueteRecibido);
	} else if(paqueteRecibido->tipoMsj==ESPACIO_INSUFICIENTE){
		imprimirMensajeEspacioInsuficiente();
	}
	else{
		aniadirPaqueteALaLista(paqueteRecibido);
	}

	destruirPaquete(paqueteRecibido);
}

void prepararPrograma(char* path){
	FILE *archivoRecibido = fopen(path, "r");
	log_info(loggerConsola,"Archivo recibido correctamente...\n");

	long int tamanio = obtenerTamanioArchivo(archivoRecibido)+1;
	char* mensaje = leerArchivo(archivoRecibido,tamanio);

	fclose(archivoRecibido);

	recibirPID(tamanio,mensaje);
}

void desconectarConsola()
{
	int i = 0;
	pthread_mutex_lock(&mutexProcesos);
	if(!(list_is_empty(listaProcesos))){
		int tamanio = list_size(listaProcesos);
		for(;i<tamanio;i++){
			programa* unPrograma  = list_get(listaProcesos, i);
			int pid = unPrograma->pid;
			finalizarPrograma(pid);
			tamanio = list_size(listaProcesos);
			i--;
		}
	}
	pthread_mutex_unlock(&mutexProcesos);
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
	pthread_mutex_lock(&mutexProcesos);
	if(!(list_is_empty(listaProcesos))){
		for(;i<list_size(listaProcesos);i++){
			programa* unPrograma = list_get(listaProcesos,i);
			printf("Los programas son: \n");
			printf("PID-PROGRAMA: %d \n", unPrograma->pid);
			free(unPrograma);
		}
	}
	pthread_mutex_unlock(&mutexProcesos);
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
			pthread_mutex_lock(&mutexProcesos);
			finalizarPrograma(pid);
			pthread_mutex_unlock(&mutexProcesos);
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
	pthread_mutex_init(&mutexProcesos,NULL);
	pthread_mutex_init(&mutexHilosCorrectos,NULL);
	listaProcesos = list_create();
	listaHilosCorrectos = list_create();

	socketKernel = conectarAServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	realizarHandshake();
	log_info(loggerConsola,"Se conecto con el kernel...\n");

	pthread_t interfazUsuario;
	pthread_t hiloManejadorPaquetes;

	pthread_create(&hiloManejadorPaquetes,NULL,manejadorPaquetes,NULL);
	pthread_create(&interfazUsuario, NULL, manejadorInterfaz,NULL);

	pthread_join(interfazUsuario,(void**) NULL);

	pthread_mutex_destroy(&mutexProcesos);
	pthread_mutex_destroy(&mutexHilosCorrectos);
	list_destroy_and_destroy_elements(listaHilosCorrectos,(void*)destruirPaquete);
	list_destroy_and_destroy_elements(listaProcesos,free);
	log_destroy(loggerConsola);
	close(socketKernel);

	return EXIT_SUCCESS;
}
