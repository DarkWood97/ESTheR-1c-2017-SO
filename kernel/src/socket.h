/*
 * socket.h
 *
 *  Created on: 8/4/2017
 *      Author: utnso
 */
#include "funcionesSockets.h"
#ifndef SRC_SOCKET_H_
#define SRC_SOCKET_H_

int ponerseAEscuchar(int, int);
int aceptarConexion(int);
void seleccionarYAceptarSockets(int);
bool enviarMensaje(int, char*);
int conectarServer(char *, int);
void verificarErrorAccept(int, struct sockaddr_in);
void verificarErrorConexion(int, int );
void verificarErrorSend(int, int );
void verificarErrorServer(struct hostent *, char*);

#endif /* SRC_SOCKET_H_ */
