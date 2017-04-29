/*
 * funcionesGenericas.h
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

#ifndef FUNCIONESGENERICAS_H_
#define FUNCIONESGENERICAS_H_

typedef struct {
	char *numero;
}_ip;

typedef struct{
  int tamMsj;
  int tipoMsj;
} header;

typedef struct{
  header header;
  void* mensaje;
}paquete;

t_config generarT_ConfigParaCargar(char *);
void recibirMensajeDeKernel(int);
void verificarParametrosInicio(int);
#endif /* FUNCIONESGENERICAS_H_ */
