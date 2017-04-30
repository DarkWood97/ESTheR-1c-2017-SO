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

	int tamPaquete=sizeof mensaje.mensaje+sizeof mensaje.tamMsj+sizeof mensaje.tipoMsj;
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



//------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	consola nuevaConsola = inicializarPrograma(argv[1]);
	mostrar_consola(nuevaConsola);
	int socketParaKernel;
	socketParaKernel = conectarAServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	char* archivo;
	archivo = recibirArchivoPorTeclado();
	paquete archivoAEnviarAKernel;
	archivoAEnviarAKernel=serializar(archivo);
	verificarRecepcionMensaje(socketParaKernel,archivoAEnviarAKernel);
	free(archivoAEnviarAKernel.mensaje);
	recibirMensajeDeKernel(socketParaKernel);
	close(socketParaKernel);
	return EXIT_SUCCESS;
}
