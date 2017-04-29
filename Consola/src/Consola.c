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
typedef struct{
  int tamMsj;
  int tipoMsj;
} header;
typedef struct __attribute__((__packed__)){
  header header;
  char* mensaje;
}paquete;

typedef struct {
	_ip ip_Kernel;
	int puerto_kernel;
} consola;

#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
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
		printf("Error de archivo: No se recibio ningun path de archivo%s\n");
	}
}
//------FUNCIONES MENSAJES--------------------------------------------
char * recibirArchivoPorTeclado(){
	char* archivoARecibir=malloc(sizeof(char)*16);//buscar la manera que aparezca vacio
	printf("Ingresar archivo:%s\n");
//	scanf("%s", mensajeARecibir);
	fgets(archivoARecibir,sizeof(char)*16,stdin); //Para poder leer una cadena con espacios
	verificarArchivoAEnviar(archivoARecibir);
	return archivoARecibir;
}
bool enviarMensaje(int socket, paquete mensaje) { //Socket que envia mensaje

	//int tamPaquete=sizeof(struct paquete);
		if (send(socket, mensaje, tamPaquete, 0) == -1) {
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
paquete serializar(header headerDeMensaje, void *bufferDeData) {
  paquete paqueteAEnviar;
  paqueteAEnviar.mensaje=malloc(sizeof(char)*16);
  paqueteAEnviar.header.tamMsj = headerDeMensaje.tamMsj;
  paqueteAEnviar.header.tipoMsj=headerDeMensaje.tipoMsj;
  paqueteAEnviar.mensaje = bufferDeData;
  return paqueteAEnviar;
}
void crearHeader(char* path, header enviar)
{
	int longitudPath=strlen(path)+1;
	enviar.tamMsj=longitudPath;
	enviar.tipoMsj = MENSAJE_PATH;
}


//------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	consola nuevaConsola = inicializarPrograma(argv[1]);
	mostrar_consola(nuevaConsola);
	int socketParaKernel;
	socketParaKernel = conectarAServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	char* archivo;
	header aEnviar;
	archivo = recibirArchivoPorTeclado();
	crearHeader(archivo, aEnviar);
	paquete archivoAEnviarAKernel;
	archivoAEnviarAKernel=serializar(aEnviar,archivo);
	verificarRecepcionMensaje(socketParaKernel,archivoAEnviarAKernel);
	free(archivoAEnviarAKernel.mensaje);
	recibirMensajeDeKernel(socketParaKernel);
	close(socketParaKernel);
	return EXIT_SUCCESS;
}
