/*
 ============================================================================
 Name        : Consola.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "src/funcionesGenericas.h"
#include "src/socket.h"
//Como hacer include sin necesidad de importar las carpetas de src del otro proyecto

typedef struct {
	char *numero;
} ip;

typedef struct {
	ip ip_Kernel;
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
	*configuracion = *config_create(path);
	consola nueva_consola = consola_crear(configuracion);
	free(configuracion);
	return nueva_consola;
}
void mostrar_consola(consola aMostrar) {
	printf("IP=%s\n", aMostrar.ip_Kernel.numero);
	printf("Puerto=%i\n", aMostrar.puerto_kernel);
	system("pause");
}

char * recibirMensaje(){
	char *mensajeARecibir;
	puts("Mensaje:");
	scanf("%s", &mensajeARecibir);
	return mensajeARecibir;
}

int main(int argc, char *argv[]) {
	if(argc!= 1){
		perror("Faltan parametros");
	}
	consola nuevaConsola = iniciarConsola(*argv);
	mostrar_consola(nuevaConsola);
	int socketKernel;
	bool llegoMensaje;
	socketKernel = conectarServer(nuevaConsola.ip_Kernel, nuevaConsola.puerto_kernel); //Revisar implementacion de ip
	char *mensajeAEnviar = recibirMensaje();
	llegoMensaje = enviarMensaje (socketKernel, mensajeAEnviar);
	return EXIT_SUCCESS;

}
