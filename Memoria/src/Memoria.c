#include "src/funcionesGenericas.h"
#include "src/socket.h"

typedef struct {
	int puerto;
	int marcos;
	int marco_size;
	int entradas_cache;
	int cache_x_proc;
	char *reemplazo_cache;
	int retardo_memoria;
} memoria;

memoria memoriaCrear(t_config *configuracion) {
	memoria memoriaAuxiliar;
	if (dictionary_size(configuracion->properties) != 7) {
		perror("Faltan parametros para inicializar memoria");
		exit(-1);
	}
	memoriaAuxiliar.puerto = config_get_int_value(configuracion, "PUERTO");
	memoriaAuxiliar.marcos = config_get_int_value(configuracion, "MARCOS");
	memoriaAuxiliar.marco_size = config_get_int_value(configuracion,"MARCO_SIZE");
	memoriaAuxiliar.entradas_cache = config_get_int_value(configuracion,"ENTRADAS_CACHE");
	memoriaAuxiliar.cache_x_proc = config_get_int_value(configuracion,"CACHE_X_PROC");
	memoriaAuxiliar.reemplazo_cache = config_get_string_value(configuracion,"REEMPLAZO_CACHE");
	memoriaAuxiliar.retardo_memoria = config_get_int_value(configuracion,"RETARDO_MEMORIA");
	return memoriaAuxiliar;
}

memoria inicializarMemoria(char *path) {
	t_config *configuracionMemoria = malloc(sizeof(t_config));
	*configuracionMemoria = generarT_ConfigParaCargar(path);
	memoria memoriaSistema = memoriaCrear(configuracionMemoria);
	return memoriaSistema;
	free(configuracionMemoria);
}

void mostrarConfiguracionesMemoria(memoria memoria) {
	printf("PUERTO=%d\n", memoria.puerto);
	printf("MARCOS=%d\n", memoria.marcos);
	printf("MARCO_SIZE=%d\n", memoria.marco_size);
	printf("ENTRADAS_CACHE=%d\n", memoria.entradas_cache);
	printf("CACHE_X_PROC=%d\n", memoria.cache_x_proc);
	printf("REEMPLAZO_CACHE=%s\n", memoria.reemplazo_cache);
	printf("RETARDO_MEMORIA=%d\n", memoria.retardo_memoria);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		perror("Faltan parametros");
		exit(-1);
	}
	memoria memoria = inicializarMemoria(argv[1]);
	mostrarConfiguracionesMemoria(memoria);
	int socketAceptadorMemoria, socketListenerMemoria;
	socketListenerMemoria = ponerseAEscuchar(memoria.puerto, 0);
	socketAceptadorMemoria = seleccionarYAceptarSockets(socketListenerMemoria);
	recibirMensajeDeKernel(socketAceptadorMemoria);
	return EXIT_SUCCESS;
}
