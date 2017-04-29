/*
 * funcionesGenericas.c
 *
 *  Created on: 6/4/2017
 *      Author: utnso
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <commons/log.h>
#include <commons/string.h>
#include <commons/config.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
typedef struct{
  int tamMsj;
  int tipoMsj;
} header;

typedef struct{
  header header;
  void* mensaje;
}paquete;
t_config generarT_ConfigParaCargar(char *path) {
	t_config *configuracionDelComponente = (t_config*)malloc(sizeof(t_config));
	if((configuracionDelComponente = config_create(path)) == NULL){
		perror("Error de ruta de archivo de configuracion");
		free(configuracionDelComponente);
		exit(-1);
	}
	return *configuracionDelComponente;
	free(configuracionDelComponente);
}

void recibirMensajeDeKernel(int socketKernel){
	char *buff = malloc(sizeof(char)*16);
//	int tamanioBuff = strlen(buff)+1;
	if(recv(socketKernel,buff,16,0) == -1){ //Implementar protocolo donde esta el 16 Tamano de mensaje
		perror("Error de receive");
		free(buff);
		exit(-1);
	}
	printf("%s\n",buff);
	free(buff);
}
void verificarParametrosInicio(int argc)
{
	if(argc!=2){
			perror("Faltan parametros para inicializar la consola");
			exit(-1);
		}
}

