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


typedef struct {
	_ip ip_Kernel;
	int puerto_kernel;
} consola;

consola consola_crear(t_config* configuracion) { //Chequear al abrir el archivo si no tiene error
	consola consola_auxiliar;
	consola_auxiliar.ip_Kernel.numero = config_get_string_value(configuracion,"IP_KERNEL");
	consola_auxiliar.puerto_kernel = config_get_int_value(configuracion,"PUERTO_KERNEL");
	return consola_auxiliar;

}

consola iniciarConsola(char *path) {
	t_config *configuracion = malloc(sizeof(t_config));
	*configuracion = generarT_ConfigParaCargar(path);
	consola nueva_consola = consola_crear(configuracion);
	free(configuracion);
	return nueva_consola;
}
void mostrar_consola(consola aMostrar) {
	printf("IP=%s\n", aMostrar.ip_Kernel.numero);
	printf("PUERTO=%i\n", aMostrar.puerto_kernel);
}

char * recibirMensaje(){
	char *mensajeARecibir=malloc(16);//buscar la manera que aparezca vacio
	puts("Mensaje:");
	scanf("%s", mensajeARecibir);
	return mensajeARecibir;
	free(mensajeARecibir);
}

int main(int argc, char *argv[]) {
	if(argc!=2){
		perror("Faltan parametros");
		exit(-1);
	}
	consola nuevaConsola = iniciarConsola(argv[1]);
	mostrar_consola(nuevaConsola);
	int socketKernel;
	bool llegoMensaje;
	socketKernel = conectarServer(nuevaConsola.ip_Kernel.numero, nuevaConsola.puerto_kernel);
	char *mensajeAEnviar = recibirMensaje();
	if(!(llegoMensaje = enviarMensaje (socketKernel, mensajeAEnviar))){
		perror("No se pudo enviar el mensaje");
		exit(-1);
	}
	recibirMensajeDeKernel(socketKernel);
	return EXIT_SUCCESS;

}
