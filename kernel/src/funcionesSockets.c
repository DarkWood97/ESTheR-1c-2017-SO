/*
 * funcionesGenericas.c
 *
 *  Created on: 7/4/2017
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

#define backlog 10 //cantidad de conexiones en la cola

int verificarErrorSocket(int socket) {
	if (socket == -1) {
		perror("Error de socket");
		exit(-1);
	} else {
		return 0;
	}
}
int verificarErrorSetsockopt(int socket) {
	int yes = 1;
	if (setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		perror("Error de setsockopt");
		exit(-1);
	} else {
		return 0;
	}
}
int verificarErrorBind(int socket, struct sockaddr_in mySocket) {
	if (bind(socket, (struct sockaddr *) &mySocket, sizeof(mySocket)) == -1) {
		perror("Error de bind");
		exit(-1);
	} else {
		return 0;
	}
}
int verificarErrorListen(int socket) {
	if (listen(socket, backlog) == -1) {
		perror("Error de listen");
		exit(-1);
	} else {
		return 0;
	}
}

//---VERIFICACION DE ERRORES-----------------------------------------
void verificarErrorAccept(int verificacion, struct sockaddr_in su_addr)
{
	if(verificacion==-1)
	{
			perror("Error de accept");
			exit(-1);
	}
	else
	{
			printf("Se ha conectado a: %s\n", inet_ntoa(su_addr.sin_addr));
	}

}
void verificarErrorConexion(int verificador, int socket_server)
{
	if (verificador == -1) {
			perror("Error al conectar con el servidor.");
			close(socket_server);
			//exit(-1);		Sacarlo cuando la conexion a memoria este hecha
		}
}
void verificarErrorSelect(int verificadorSelect)
{
	if (verificadorSelect == -1) {
				perror("Error de select");
				exit(1);
			}
}
void verificarErrorSend(int verificacion, int socket)
{
	if (verificacion == -1) {
				perror("Error de send");
				close(socket);
				exit(-1);
			}
}
void verificarErrorServer(struct hostent *infoDelServer, char* ip)
{
	if ((infoDelServer = gethostbyname(ip)) == NULL) {
			perror("Error al obtener datos del server.");
			exit(-1);
		}
}
