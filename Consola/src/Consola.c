/*
 ============================================================================
 Name        : Consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesGenericas.h"
#include "socket.h"
//--TYPEDEF-------------------------------------------------------------------
typedef struct __attribute__((__packed__)){
	int tamMsj;
	int tipoMsj;
	void* mensaje;
}paquete;

typedef struct {
	_ip ip_Kernel;
	int puerto_kernel;
} consola;
int sizePaquete=sizeof(paquete);
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define INICIAR 1
#define FINALIZAR 2
#define DESCONECTAR 3
#define LIMPIAR 4

int socketKernel;
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
		printf("Error de archivo: No se recibio ningun path de archivos\n");
	}
}
//------FUNCIONES MENSAJES--------------------------------------------
char * recibirArchivoPorTeclado(){
	char* archivoARecibir=malloc(sizeof(char)*16);//buscar la manera que aparezca vacio
	printf("%s","Ingresar archivo");
//	scanf("%s", mensajeARecibir);
	fgets(archivoARecibir,sizeof(char)*16,stdin); //Para poder leer una cadena con espacios
	verificarArchivoAEnviar(archivoARecibir);
	return archivoARecibir;
}
bool enviarMensaje(int socket, paquete mensaje) { //Socket que envia mensaje

	int tamPaquete=(sizeof mensaje.tamMsj)+sizeof (mensaje.tipoMsj)+sizeof(mensaje.mensaje);
		if (send(socket, &mensaje, tamPaquete, 0) == -1) {
			perror("Error de send");
			close(socket);
			exit(-1);
	}
	return true;
}
void verificarRecepcionMensaje(int socket, paquete mensajeAEnviar)
{
	bool llegoMensaje;
	if(!(llegoMensaje=enviarMensaje(socket, mensajeAEnviar))){
			perror("No se pudo enviar el mensaje");
			free(mensajeAEnviar.mensaje);
			exit(-1);
		}
}
//---------------FUNCIONES DE SERIALIZACION--------------------------------
int obtenerLongitudPath(char* path)
{
	int longitudPath=strlen(path)+1;
	return longitudPath;

}
paquete serializar(void *bufferDeData) {
  paquete paqueteAEnviar;
  int longitud= obtenerLongitudPath(bufferDeData);
  paqueteAEnviar.mensaje=malloc(sizeof(char)*16);
  paqueteAEnviar.tamMsj = longitud;
  paqueteAEnviar.tipoMsj= MENSAJE_PATH;
  paqueteAEnviar.mensaje = bufferDeData;
  return paqueteAEnviar;
}
void destruirPaquete(paquete *paquete) {
	free(paquete->mensaje);
	free(paquete);
}
//---------------------FUNCIONES DE INTERFAZ----------------------------
void iniciarProgramaAnsisop()
{
	char * pathPrograma;
	paquete auxiliar,paquetePath;
	recibirArchivoPorTeclado(pathPrograma);
	auxiliar=serializar(pathPrograma);
	memcpy(&paquetePath,&auxiliar,sizeof(auxiliar));
	verificarRecepcionMensaje(socketKernel,paquetePath);
	destruirPaquete(&auxiliar);
	destruirPaquete(&paquetePath);
}
void finalizarPrograma()
{

}
void desconectarConsola()
{

}
void solicitarComando()
{
	int comando;
	printf("INgrese el numero de la opcion deseada\n");
	printf("-1. Iniciar programa Ansisop\n");
	printf("-2 Finalizar programa");
	printf("-3 Desconectar consola");
	printf("-4 Limpiar consola");
	printf("Esperando comando....\n");
	//fgets(comando,sizeof(int),stdin);
	scanf("%d",&comando);
	switch(comando)
	{
	case INICIAR:
		iniciarProgramaAnsisop();
		break;
	case FINALIZAR:
		finalizarPrograma();
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

//------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	consola nuevaConsola = inicializarPrograma(argv[1]);
	mostrar_consola(nuevaConsola);
	socketKernel = conectarAServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	while(1)
	{
		solicitarComando();
		recibirMensajeDeKernel(socketKernel);
	}
	close(socketKernel);
	return EXIT_SUCCESS;
}
