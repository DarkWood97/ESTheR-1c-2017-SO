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

typedef struct __attribute__((__packed__)){
	int tipoMsj;
	int tamMsj;
	void* mensaje;
}paquete;

#define backlog 10 //cantidad de conexiones en la cola

int verificarErrorSocket(int);

int verificarErrorSetsockopt(int);

int verificarErrorBind(int, struct sockaddr_in);

int verificarErrorListen(int);
void verificarErrorServer(struct hostent *, char*);
void verificarErrorSend(int, int );
void verificarErrorSelect(int );
void verificarErrorConexion(int, int);
void verificarErrorAccept(int, struct sockaddr_in);
#endif /* FUNCIONESGENERICAS_H_ */
