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
	int puerto;
	char *punto_montaje;
} fileSystem;

fileSystem fileSystemCrear(t_config *configuracion) {
	fileSystem fileSystemAuxiliar;
	if (dictionary_size(configuracion->properties) != 2) {
		perror("Faltan parametros para inicializar el fileSystem");
		exit(-1);
	}
	fileSystemAuxiliar.puerto = config_get_int_value(configuracion, "PUERTO");
	fileSystemAuxiliar.punto_montaje = config_get_string_value(configuracion,"PUNTO_MONTAJE");
	return fileSystemAuxiliar;
}

fileSystem inicializarFileSystem(char *path) {
	t_config *configuracionFileSystem = malloc(sizeof(t_config));
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
	printf("PUERTO=%d\n", fileSystem.puerto);
	printf("PUNTO_MONTAJE=%s\n", fileSystem.punto_montaje);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		perror("Faltan parametros");
		exit(-1);
	}
	fileSystem fileSystem = inicializarFileSystem(argv[1]);
	mostrarConfiguracionesFileSystem(fileSystem);
	int socketAceptadorFS, socketListenerFS;
	socketListenerFS = ponerseAEscuchar(fileSystem.puerto, 0);
	socketAceptadorFS = aceptarConexion(socketListenerFS);
	recibirMensajeDeKernel(socketAceptadorFS);
	return EXIT_SUCCESS;
}
