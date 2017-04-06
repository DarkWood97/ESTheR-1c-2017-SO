/*
 ============================================================================
 Name        : Sockets.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>

int ponerseAEscuchar(int puerto, int protocolo){
	int socketListener;
	struct sockaddr_in mySocket;
	int  yes = 1;
	if((socketListener = socket(AF_INET,SOCK_STREAM,protocolo)) == -1){
		perror("Error de socket");
		return -1;
	}
	if(setsokcopt(socketListener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
		perror("Error de setsockopt");
		return -1;
	}
	mySocket.sin_family = AF_INET;
	mySocket.sin_port = htons(puerto);
	mySocket.sin_addr.s_addr = htonl(INADDR_ANY); //Ver htonl
	memset(&(addr_server.sin_zero),'\0',8);

	if(bind(socketListenner,(struct sockaddr *)mySocket,sizeof(mySocket)) == -1){
		perror("Error de bind");
		return -1;
	}


}
