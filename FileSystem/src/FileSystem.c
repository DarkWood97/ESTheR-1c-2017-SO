/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesGenericas.h"
#include "socket.h"

typedef struct {
	int port;
	char *punto_montaje;
} fileSystem;

fileSystem fileSystemCrear(t_config *configuracion) {
	fileSystem fileSystemAuxiliar;
	if (dictionary_size(configuracion->properties) != 2) {
		perror("Faltan parametros para inicializar el fileSystem");
	}
	fileSystemAuxiliar.port = config_get_int_value(configuracion, "PORT");
	fileSystemAuxiliar.punto_montaje = config_get_string_value(configuracion,"PUNTO_MONTAJE");
	return fileSystemAuxiliar;
}

fileSystem inicializarFileSystem(char *path) {
	t_config *configuracionFileSystem;
	*configuracionFileSystem = generarT_ConfigParaCargar(path);
	fileSystem fileSystemSistema = fileSystemCrear(configuracionFileSystem);
	return fileSystemSistema;
	free(configuracionFileSystem);
}

bool validarArchivo(char* path) {
	FILE* f = fopen(path, " r ");
	if (!f) {
		return true;
	} else {
		return false;
	}
}

void crearArchivo(char* path) {
	bool permisoCreacion = true; //Bitmap?
	if (validarArchivo(path) && permisoCreacion) {
		FILE* f = fopen(path, "w");
	}
}

void borraArchivo(char* path) {
	remove(path); //Bitmap?
}

void mostrarConfiguracionesFileSystem(fileSystem fileSystem) {
	printf("PORT=%d\n", fileSystem.port);
	printf("PUNTO_MONTAJE=%s\n", fileSystem.punto_montaje);
}

int main(int argc, char *argv[]) {
	if (argc != 1) {
		perror("Faltan parametros");
	}
	fileSystem fileSystem = inicializarFileSystem(argv[0]);
	mostrarConfiguracionesFileSystem(fileSystem);
	int socketAceptadorFS, socketListenerFS;
	socketListenerFS = ponerseAEscuchar(fileSystem.port, 0);
	socketAceptadorFS = aceptarConexion(socketListenerFS);
	recibirMensajeDeKernel(socketAceptadorFS);
	return EXIT_SUCCESS;
}
