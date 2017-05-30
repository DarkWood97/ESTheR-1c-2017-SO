/*
 ============================================================================
 Name        : Consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include <pthread.h>
#include "funcionesGenericas.h"
#include "socket.h"
//--TYPEDEF-------------------------------------------------------------------
typedef struct __attribute__((__packed__)){
	int tamMsj;
	int tipoMsj;
	void* mensaje;
}paquete;
pthread_t a;
typedef struct {
	_ip ip_Kernel;
	int puerto_kernel;
} consola;
typedef struct
{
	int PID;
	struct timeval inicio;
	struct timeval fin;
	struct timeval duracion;
	pthread_t hilo;
	int impresiones;
}programa;
time_t horario;
int sizePaquete=sizeof(paquete);
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define MENSAJE_PID   105
#define HANDSHAKE_CONSOLA 1005
#define ESPACIO_INSUFICIENTE -2
#define INICIAR 1
#define FINALIZAR 503
#define DESCONECTAR 3
#define LIMPIAR 4

int socketKernel;
t_list * listaDeProgramas;
pthread_t usuario;
//No puede inicializar mas de 20 prog
//----FUNCIONES CONSOLA-------------------------------------------------------
consola consola_crear(t_config* configuracion) { //Chequear al abrir el archivo si no tiene error
	consola consola_auxiliar;
	consola_auxiliar.ip_Kernel.numero = config_get_string_value(configuracion,"IP_KERNEL");
	consola_auxiliar.puerto_kernel = config_get_int_value(configuracion,"PUERTO_KERNEL");
	return consola_auxiliar;
}

consola inicializarPrograma(char* path) {
	t_config *configuracion = (t_config*)malloc(sizeof(t_config));
	*configuracion = generarT_ConfigParaCargar(path);
	consola nueva_consola = consola_crear(configuracion);
	free(configuracion);
	return nueva_consola;
}
void mostrar_consola(consola aMostrar) {
	printf("IP=%s\n", aMostrar.ip_Kernel.numero);
	printf("PUERTO=%i\n", aMostrar.puerto_kernel);
}
//-------------------------------------------------------------------
//----FUNCIONES DE ARCHIVOS RECIBIDOS---------------------------------
void verificarArchivoAEnviar(char * archivo)
{
	if(strlen(archivo)==0)
	{
		perror("Error de archivo");
	}
}
//------FUNCIONES MENSAJES--------------------------------------------
char * recibirArchivoPorTeclado(){
	char* archivoARecibir=malloc(sizeof(char)*16);//buscar la manera que aparezca vacio
	printf("Por favor ingrese el path del archivo a continuacion\n");
	scanf("%s", archivoARecibir);
	//fgets(archivoARecibir,sizeof(char)*16,stdin); //Para poder leer una cadena con espacios
	verificarArchivoAEnviar(archivoARecibir);
	return archivoARecibir;
}
void verificarRecepcionMensaje(int socket, paquete  mensajeAEnviar)
{
	if(send(socket,&mensajeAEnviar,sizeof(mensajeAEnviar),0)){
			perror("No se pudo enviar el mensaje");
			free(mensajeAEnviar.mensaje);
			exit(-1);
		}
}
void realizarHandshake(int socket, paquete mensaje) { //Socket que envia mensaje

	int longitud = sizeof(mensaje); //sino no lee \0 (ACA POR QUE NO ES PAQUETE?)
		if (send(socket, &mensaje, longitud, 0) == -1) {
			perror("Error de send");
			close(socket);
			exit(-1);
	}
//
}
//---------------FUNCIONES DE SERIALIZACION--------------------------------

paquete  serializar(int tipoMensaje, char* bufferDeData) {
  paquete aEnviar;

  aEnviar.tamMsj = string_length(bufferDeData)+1;
  aEnviar.mensaje=malloc(aEnviar.tamMsj);
  aEnviar.tipoMsj= tipoMensaje;
  aEnviar.mensaje = bufferDeData;

  //free(aEnviar.mensaje);
  return aEnviar;
}

void deserealizarMensaje(int socket, void * retornar, void* impresiones)
{
	int *tamanio = 0;
	recv(socket,tamanio,16,0);
	paquete recibido;
	recv(socket,&recibido,(int)tamanio,0);
	int caso=recibido.tipoMsj;
	switch(caso)
	{
	case MENSAJE_IMPRIMIR:
		printf("Mensaje recibido: %p",recibido.mensaje);
		impresiones++;
		break;
	case MENSAJE_PID:
		retornar=recibido.mensaje;
	}
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
//-----------FUNCION PROGRAMA------------------------------------------
void nuevoPrograma (programa * programaNuevo,int  pid)
{
	struct timeval inicioAux;
	programaNuevo->PID=pid;
	gettimeofday(&inicioAux, NULL);
	programaNuevo->inicio=inicioAux;
}

void imprimirInformacion(programa *programaAImprimir)
{
	puts("Fecha y hora de inicio: \n ");
	timeval_print(&programaAImprimir->inicio);
	puts("Fecha y hora de finalizacion: \n");
	timeval_print(&programaAImprimir->fin);
	printf("Cantidad de impresiones por pantalla: %d \n", programaAImprimir->impresiones);
	puts("Tiempo total de ejecucion: ");
	timeval_print(&programaAImprimir->duracion);
}

//---------------------FUNCIONES DE INTERFAZ----------------------------
void * iniciarProgramaAnsisop(char * pathPrograma, programa * programaEjecutando)
{
	paquete  auxiliar;
	paquete paquetePath;
	int PIDPrograma=0;
	auxiliar=serializar(MENSAJE_PATH,pathPrograma);
	memcpy(&paquetePath,&auxiliar,sizeof(auxiliar));
	verificarRecepcionMensaje(socketKernel,paquetePath);
	free(auxiliar.mensaje);
	free(paquetePath.mensaje);
	deserealizarMensaje(socketKernel,&PIDPrograma,NULL);
	nuevoPrograma(programaEjecutando,PIDPrograma);
	printf("El programa ejecutando tiene el PID: %d",PIDPrograma);
	deserealizarMensaje(socketKernel, NULL,&programaEjecutando->impresiones); //queda a la espera de mensajes para imprimir
	list_add(listaDeProgramas,programaEjecutando);
	free(pathPrograma);
	return NULL;
}
programa * buscoUnPid(int pidBuscado)
{
        bool _pidEquivalentes_(programa* unPrograma) //aca recibis cada elemento de la listaDeProgramas, los va pasando automaticamente uno por uno
        {
 	       return unPrograma->PID == pidBuscado;
        }
       programa * encontrado = list_find(listaDeProgramas, (void *)_pidEquivalentes_);
       return encontrado;
}
void finalizarPrograma(int pid)
{
	programa * auxiliar;
	auxiliar=buscoUnPid(pid);
	if(auxiliar!=NULL)
	{
		pthread_join(auxiliar->hilo, NULL);
		gettimeofday(&auxiliar->fin, NULL);
		timeval_subtract(&auxiliar->duracion, &auxiliar->fin, &auxiliar->inicio);
		imprimirInformacion(auxiliar);
		free(auxiliar);
	}
	else
	{
		perror("PID no encontrado");
	}

}

void desconectarConsola()
{
	list_destroy_and_destroy_elements(listaDeProgramas,NULL);
	pthread_join(usuario, NULL);
	puts("Se desconecto la consola \n");
	puts("Se abortaron todos los programas");

}
void mostrarAyuda()
{
	printf("Bienvenido al menu de opciones de la consola\n");
	printf("Ingrese el numero de la opcion deseada\n");
	printf("-1. Iniciar programa Ansisop\n");
	printf("-2 Finalizar programa\n");
	printf("-3 Desconectar consola\n");
	printf("-4 Limpiar consola\n");
}
void buscarComandoCorrespondiente (int comando)
{
	char * path;
	programa * nuevo =malloc(sizeof(programa));
	switch(comando)
		{
		case INICIAR:
			path=recibirArchivoPorTeclado();
			pthread_create(&nuevo->hilo,NULL,iniciarProgramaAnsisop(path,nuevo),NULL);
			free(path);
			free(nuevo);
			break;
		case FINALIZAR:
			puts("Ingresar PID del programa a finalizar");
			int pid;
			scanf("%d",&pid);
			finalizarPrograma(pid);
			break;
		case DESCONECTAR:
			desconectarConsola();
			break;
		case LIMPIAR:
			system("clear");
			break;
		default:
			perror("Comando no reconocido");
		}
}
void * solicitarComando()
{
	while(1)
	{
		int comando;
		mostrarAyuda();
		scanf("%d",&comando);
		buscarComandoCorrespondiente(comando);
	}
	return NULL;
}
//------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	//char* prueba="Debug/consola.config";
	//consola nuevaConsola = inicializarPrograma(prueba);
	consola nuevaConsola = inicializarPrograma(argv[1]);
	mostrar_consola(nuevaConsola);
	socketKernel = conectarAServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	/* HANDSHAKE CON KERNEL */
	paquete paqueteAEnviar;
	paquete auxiliar;
	auxiliar=serializar(HANDSHAKE_CONSOLA, "0");
	memcpy(&paqueteAEnviar, &auxiliar, sizeof(auxiliar)); /*no se si es sizeof(paquete)*/
	realizarHandshake(socketKernel,paqueteAEnviar);
	free(paqueteAEnviar.mensaje);
	free(auxiliar.mensaje);
	/*fin handshake*/
	/*hilo usuario*/

	listaDeProgramas=list_create();
	pthread_create(&usuario, NULL, solicitarComando(),NULL);
	pthread_join(usuario,NULL);

	close(socketKernel);
	return EXIT_SUCCESS;
}
