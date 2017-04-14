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

typedef struct {
	_ip ip_Kernel;
	int puerto_kernel;
} consola;
//----FUNCIONES CONSOLA-------------------------------------------------------
consola consola_crear(t_config* configuracion) { //Chequear al abrir el archivo si no tiene error
	consola consola_auxiliar;
	consola_auxiliar.ip_Kernel.numero = config_get_string_value(configuracion,"IP_KERNEL");
	consola_auxiliar.puerto_kernel = config_get_int_value(configuracion,"PUERTO_KERNEL");
	return consola_auxiliar;

}

consola inicializarConsola(char *path) {
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
//------FUNCIONES MENSAJES--------------------------------------------
char * recibirMensajePorTeclado(){
	char* mensajeARecibir=malloc(sizeof(char)*16);//buscar la manera que aparezca vacio
	puts("Mensaje:");
	scanf("%s", mensajeARecibir);
	return mensajeARecibir;
}
void verificarRecepcionMensaje(int socket, char* mensajeAEnviar)
{
	bool llegoMensaje;
	if(!(llegoMensaje = enviarMensaje (socket, mensajeAEnviar))){
			perror("No se pudo enviar el mensaje");
			free(mensajeAEnviar);
			exit(-1);
		}
}

int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	consola nuevaConsola = inicializarConsola(argv[1]);
	mostrar_consola(nuevaConsola);
	int socketParaKernel;
	socketParaKernel = conectarAServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	char* mensajeAEnviarAKernel;
	mensajeAEnviarAKernel = recibirMensajePorTeclado();
	verificarRecepcionMensaje(socketParaKernel,mensajeAEnviarAKernel);
	free(mensajeAEnviarAKernel);
	recibirMensajeDeKernel(socketParaKernel);
	return EXIT_SUCCESS;

}
