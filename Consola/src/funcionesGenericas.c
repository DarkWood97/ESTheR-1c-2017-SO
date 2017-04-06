/*
 * funcionesGenericas.c
 *
 *  Created on: 6/4/2017
 *      Author: utnso
 */

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

