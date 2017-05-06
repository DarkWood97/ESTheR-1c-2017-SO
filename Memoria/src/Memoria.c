#include "funcionesGenericas.h"
#include "../Socket/src/socket.h"
//#include "socket.h"
#include <ctype.h>


#define FALTA_RECURSOS_MEMORIA -50
#define ESPACIO_INSUFICIENTE -1
#define ES_CPU 1001
#define ES_KERNEL 1002
#define INICIALIZAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define INICIO_EXITOSO
#define TAMANIO_PAGINA_PARA_KERNEL

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

//-----FUNCIONES MEMORIA-------------------------------------------------
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
	int cantidadDePaginasQueOcupa = tamanio_tabla/MARCOS_SIZE;
	if((tamanio_tabla%MARCOS_SIZE)!=0){
	    cantidadDePaginasQueOcupa++;
	  }
	return cantidadDePaginasQueOcupa;
}

void crearEstructuraAdministrativa(){ //Tabla de paginas invertidas
	int cantidadDePAginasQueOcupa = calcularTamanioDeTabla();
	punteroAPrincipioDeDatos = memoriaSistema + cantidadDePAginasQueOcupa*MARCOS_SIZE;
	int paginaChequeada;
	for(paginaChequeada = 0; paginaChequeada<cantidadDePAginasQueOcupa;paginaChequeada++){
		entradasDeTabla[paginaChequeada].frame = paginaChequeada;
		entradasDeTabla[paginaChequeada].pid = -1;
		entradasDeTabla[paginaChequeada].pagina = paginaChequeada;
	}
	for(paginaChequeada = cantidadDePAginasQueOcupa; paginaChequeada<MARCOS; paginaChequeada++){
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





//-------------------------FUNCIONES AUXILIARES PARA LOS MANEJADORES DE HILOS---------------------------//

//------------------------FUNCIONES AUXILIARES PARA HILO DE KERNEL--------------------------------------//

bool hayEspacioParaNuevoProceso(int cantidadDePaginasNecesarias){
	int paginaChequeada;
	int paginasDisponiblesEncontradas = 0;
	bool hayEspacio = false;
	for(paginaChequeada = 0 ; paginaChequeada<=MARCOS;paginaChequeada++){
		if(ocupaCeroPaginas(paginaChequeada)&&noHayNingunProceso(paginaChequeada)){
			paginasDisponiblesEncontradas++;
		}
	}
	if(paginasDisponiblesEncontradas>=cantidadDePaginasNecesarias){
		hayEspacio = true;
	}
	return hayEspacio;
}

//-------------------------------------ASIGNAR PAGINAS PARA PROCESO---------------------------------//

void asignarPaginasAProceso(int pid, int cantidadDePaginas, int socketKernel){
  //Aca va un semaforo
	if(hayEspacioParaNuevoProceso(cantidadDePaginas)){
		int espacioInsuficiente = ESPACIO_INSUFICIENTE;
		if(send(socketKernel,&espacioInsuficiente,sizeof(int),0)==-1){
			perror("Erro de send");
			exit(-1);
		}
	}else{
		int paginaChequeada, cantidadDePaginasAsignadas = 0;
		for(paginaChequeada = 0; paginaChequeada<=MARCOS && cantidadDePaginasAsignadas<=cantidadDePaginas; paginaChequeada++){
			if(ocupaCeroPaginas(paginaChequeada)&&noHayNingunProceso(paginaChequeada)){
				entradasDeTabla[paginaChequeada].pid = pid;
				entradasDeTabla[paginaChequeada].pagina = cantidadDePaginasAsignadas;
				cantidadDePaginasAsignadas++;
			}
		}
	}
  //Fin del semaforo
}

//-----------------------------------INICIAR PROCESO----------------------------------------//

void inicializarProceso(int pid, int cantidadDePaginas, int socketKernel){
  //Aca va un semaforo

	log_info(loggerMemoria,"Se ha encontrado espacio suficiente para inicializar el programa %d",pid);
	usleep(RETARDO_MEMORIA);
	log_info(loggerMemoria,"Retardo...");
	asignarPaginasAProceso(pid, cantidadDePaginas,socketKernel);
}

void finalizarProceso(int pidProceso){
	int numeroDePagina;
	for(numeroDePagina = 0; numeroDePagina<=MARCOS; numeroDePagina++){
		if(entradasDeTabla[numeroDePagina].pid == pidProceso){
			entradasDeTabla[numeroDePagina].pid = -1;
			entradasDeTabla[numeroDePagina].pagina = -1;
		}
	}
	log_info(loggerMemoria,"El proceso %d se ha finalizado.",pidProceso);
}


//------------------------------------------------HILOS MEMORIA-----------------------------------------//
//--------------------------------------------MANEJADOR TECLADO-----------------------------------------//

void *manejadorTeclado(){
  //Leo de consola, con un maximo de 50 caracteres Usar getline
	char* comando = malloc(50*sizeof(char));
	size_t tamanioMaximo = 50;


	while(1){
		puts("Esperando comando...");
		size_t bytesLeidos = getline(&comando, &tamanioMaximo, stdin);

		int tamanioRecibido = strlen(comando);
		comando[tamanioRecibido-1] = '\0';
		if(string_equals_ignore_case(comando,"dump")){
			dumpDeTodosLosDatos();
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

			}else{
				puts("Error de comando");
			}
		}
	}
	free(comando);
}
//-------------------------------------------MANEJADOR KERNEL------------------------------------//
void *manejadorConexionKernel(void* socketKernel){
	while(1){
		int pid,paginasRequeridas;
		paquete paqueteRecibidoDeKernel;
    //Chequear y recibir datos de kernel
		if(recv(*(int*)socketKernel, &paqueteRecibidoDeKernel.tipoMsj, sizeof(int),0)==-1){   //*(int*) porque lo que yo tengo es un puntero de cualquier cosa que paso a int y necesito el valor de eso
			perror("Error al recibir el tipo de mensaje\n");
			exit(-1);
		}
		if(recv(*(int*)socketKernel,&paqueteRecibidoDeKernel.tamMsj, sizeof(int),0)==-1){
			perror("Error al recibir el tamanio del mensaje\n");
		}
		if(recv(*(int*)socketKernel,&pid,sizeof(int),0)==-1){ //Lo recibo aca, ya que todas los comandos enviados por kernel incluyen el pid
			perror("Error de recv");
			exit(-1);
		}

		switch (paqueteRecibidoDeKernel.tipoMsj){
		case INICIALIZAR_PROGRAMA:
			paqueteRecibidoDeKernel.mensaje = malloc(paqueteRecibidoDeKernel.tamMsj);
			if(recv(*(int*)socketKernel,&paginasRequeridas,sizeof(int),0)==-1){
				perror("Error al recibir cantidad de paginas requeridas\n");
				exit(-1);
			}
			inicializarProceso(pid,paginasRequeridas,*(int*)socketKernel);
			break;
		case ASIGNAR_PAGINAS:
			if(recv(*(int*)socketKernel,&paginasRequeridas,sizeof(int),0)==-1){
				perror("Error al recibir cantidad de paginas requeridas\n");
				exit(-1);
			}
			asignarPaginasAProceso(pid, paginasRequeridas, *(int*)socketKernel);
			break;
		case FINALIZAR_PROGRAMA:
			finalizarProceso(pid);
			break;
		default:
			perror("No se recibio correctamente el mensaje");
		}
		free(paqueteRecibidoDeKernel.mensaje);
	}
}
//--------------------------------------------MANEJADOR CPU--------------------------------------//
void *manejadorConexionCPU (void *socket){
  //Para la proxima entrega queda ver que hace memoria y cpu en su conexion
}
//------------------------------------------FIN DE FUNCIONES MANEJADORAS DE HILOS------------------------//



//----------------------------------------MAIN--------------------------------------------//
int main(int argc, char *argv[]) {
	loggerMemoria = log_create("Memoria.log","Memoria",0,0);
	pthread_mutex_init(&mutexTablaInvertida,NULL);
	verificarParametrosInicio(argc);
	//char* path = "Debug/memoria.config";
	inicializarMemoria(argv[1]);
	paquete paqueteDeRecepcion;
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

	aceptarConexiones = fd_setAuxiliar;
	pthread_create(&hiloManejadorTeclado,NULL,manejadorTeclado,NULL); //Creo un hilo para atender las peticiones provenientes de la consola de memoria
	log_info(loggerMemoria,"Conexion a consola memoria exitosa...\n");
	while (1) { //Chequeo nuevas conexiones y las voy separando a partir del handshake recibido (CPU o Kernel).
		fd_setAuxiliar = aceptarConexiones;
		if(select(socketEscuchaMemoria,&fd_setAuxiliar,NULL,NULL,NULL)==-1){
			perror("Error de select en memoria.");
			exit(-1);
		}
		for(socketClienteChequeado = 0; socketClienteChequeado<=socketMaximo; socketClienteChequeado++){
			if(socketClienteChequeado == socketMaximo){
				socketAceptaClientes = aceptarConexionDeCliente(socketEscuchaMemoria);
				FD_SET(socketAceptaClientes,&aceptarConexiones);
				socketMaximo = calcularSocketMaximo(socketAceptaClientes,socketMaximo);
				log_info(loggerMemoria,"Registro nueva conexion de socket: %d\n",socketAceptaClientes);
			}else{
				if(recv(socketClienteChequeado,&paqueteDeRecepcion.tipoMsj,sizeof(int),0)==-1){
					perror("Error de send en memoria");
					exit(-1);
				}else{
					switch(paqueteDeRecepcion.tipoMsj){
		  				case ES_CPU:
		  					pthread_create(&hiloManejadorCPU,NULL,manejadorConexionCPU,(void *)socketClienteChequeado);
		  					log_info(loggerMemoria,"Se registro nueva CPU...\n");
		  					FD_CLR(socketClienteChequeado,&aceptarConexiones);
		  					break;
		  				case ES_KERNEL:
		  					pthread_create(&hiloManejadorKernel,NULL,manejadorConexionKernel,(void *)socketClienteChequeado);
		  					if(send(socketClienteChequeado,&MARCOS_SIZE,sizeof(int),0)==-1){
		  						perror("Erro al enviar el tamanio de pagina");
		  						exit(-1);
		  					}
		  					log_info(loggerMemoria,"Se registro conexion de Kernel...\n");
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
