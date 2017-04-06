#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <string.h>

t_config generarT_ConfigParaCargar(char *path){
	t_config *configuracionDelComponente = malloc (sizeof(t_config));
	configuracionDelComponente = config_create(path);
	return *configuracionDelComponente;
	free(configuracionDelComponente);
}

int cantidadElementosArray (unsigned char *array){
	int i=0;
	for(i;i;i++){

	}
}
