/*
 * socket.h
 *
 *  Created on: 8/4/2017
 *      Author: utnso
 */
#include "funcionesSockets.h"
#ifndef SRC_SOCKET_H_
#define SRC_SOCKET_H_

//typedef struct __attribute__((__packed__)){
//	int tipoMsj;
//	int tamMsj;
//	void* mensaje;
//}paquete;

int ponerseAEscucharClientes(int, int);
int aceptarConexionDeCliente(int);
void seleccionarYAceptarSockets(int);
bool enviarMensaje(int, char*);
int conectarServer(char *, int);
void verificarErrorAccept(int, struct sockaddr_in);
void verificarErrorConexion(int, int );
void verificarErrorSend(int, int );
void verificarErrorServer(struct hostent *, char*);
int calcularSocketMaximo(int, int);
int recvDeNotificacion(int);
void sendDeNotificacion(int , int );
paquete *recvRemasterizado(int);
void sendRemasterizado(int, int, int, void*);
void destruirPaquete(paquete*);
#endif /* SRC_SOCKET_H_ */
