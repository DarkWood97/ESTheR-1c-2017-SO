/*
 * socket.h
 *
 *  Created on: 8/4/2017
 *      Author: utnso
 */
#include "funcionesSockets.h"
#ifndef SRC_SOCKET_H_
#define SRC_SOCKET_H_

typedef struct{
  int tamMsj;
  int tipoMsj;
} header;
typedef struct{
  header header;
  char* mensaje;
}paquete;

int ponerseAEscuchar(int, int);
int aceptarConexion(int);
void seleccionarYAceptarSockets(int);
bool enviarMensaje(int, paquete);
int conectarServer(char *, int);


#endif /* SRC_SOCKET_H_ */
