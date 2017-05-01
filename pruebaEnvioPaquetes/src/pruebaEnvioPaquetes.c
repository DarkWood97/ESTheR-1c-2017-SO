/*
 ============================================================================
 Name        : pruebaEnvioPaquetes.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "socket.h"
#include "funcionesGenericas.h"

#define PORT 5000
#define IP "127.0.0.1"
#define ES_CPU 1001
#define ES_KERNEL 1002
#define INICIALIZAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503


typedef struct __attribute__((__packed__)){
	int tamMsj;
	int tipoMsj;
	void* mensaje;
}paquete;


//-----------------------HILO KERNEL-----------------------------------------------------//
void *manejadorConexionKernel(void* socketKernel){
  while(1){
    paquete paqueteRecibidoDeKernel;
    //Chequear y recibir datos de kernel
    if(recv(*(int*)socketKernel, &paqueteRecibidoDeKernel.tipoMsj, sizeof(int),0)==-1){   //*(int*) porque lo que yo tengo es un puntero de cualquier cosa que paso a int y necesito el valor de eso
    	perror("Error al recibir el tipo de mensaje\n");
    	exit(-1);
    }

    switch (paqueteRecibidoDeKernel.tipoMsj) {
      case INICIALIZAR_PROGRAMA:
    	  if(recv(*(int*)socketKernel,&paqueteRecibidoDeKernel.tamMsj,sizeof(int),0)==-1){
    		  perror("Error al recibir tamanio de mensaje\n");
    		  exit(-1);
    	  }
    	  paqueteRecibidoDeKernel.mensaje = malloc(paqueteRecibidoDeKernel.tamMsj);
      //Chequeo al recibir con la funcion ya hecha
        int idPrograma,paginasRequeridas;
        if(recv(*(int*)socketKernel,&idPrograma,sizeof(int),0) == -1){
        	perror("Error al recibir id de programa\n");
        	exit(-1);
        }
        if(recv(*(int*)socketKernel,&paginasRequeridas,sizeof(int),0)==-1){
        	perror("Error al recibir cantidad de paginas requeridas\n");
        	exit(-1);
        }
        // inicializarPrograma(idPrograma,paginasRequeridas);
        break;
      case ASIGNAR_PAGINAS:
        //blablabla todo lo que haya que hacer busco paginas libres, y asigno las que me pida,
        //Si no alcanzan perror
    	  break;
      case FINALIZAR_PROGRAMA:
        //Busco todas las paginas del proceso y las desasigno (-1 al pid)
    	  break;
      default:
    	  perror("No se recibio correctamente el mensaje");
    }
    free(paqueteRecibidoDeKernel.mensaje);
  }
}

//-----------------------------------HILOS CPU---------------------------------------//
void *manejadorConexionCPU (void *socket){
  //Para la proxima entrega queda ver que hace memoria y cpu en su conexion
}
//------------------------------------HILO TECLADO---------------------------------//
void *manejadorTeclado(){
  //Leo de consola, con un maximo de 50 caracteres Usar getline
  char* comando = malloc(50*sizeof(char));
  size_t tamanioMaximo = 50;


  while(1){
  puts("Esperando comando...");
  getline(&comando, &tamanioMaximo, stdin);

  int tamanioRecibido = strlen(comando);
  comando[tamanioRecibido-1] = '\0';

  char* posibleRetardo = string_substring_until(comando, 7);
  char* posibleFlush = string_substring_until(comando, 5);
  char* posibleDump = string_substring_until(comando, 4);
  char* posibleSize = string_substring_until(comando, 4);

  if(string_equals_ignore_case(posibleRetardo, "retardo")){  //
    char* valorARetardar = string_substring_from(comando, 8);

    if(!isdigit(valorARetardar)){  //Hay que hacer una funcion que chequee todos los valores, porque esta solo chequea caracteres, necesita chequear strings
      perror("Error de valor de retardo");
    }else{
//      int reatardoRequerido = atoi(valorARetardar);
//      retardo(retardoRequerido);
    }

  }else if(string_equals_ignore_case(posibleFlush, "flush")){
    //Limpiar cache, queda para las proximas entregas
  }else if(string_equals_ignore_case(posibleDump, "dump")){
      puts("dumppppp");
  }else if(string_equals_ignore_case(posibleSize, "size")){

  }

  }
}

//----------------------------------MAIN---------------------------------------//

int main(void) {

	t_log * loggerMemoria = log_create("Memoria.log","Memoria",0,0);

	int socketQueEscucha, socketChequeado, socketQueAcepta;
	paquete paqueteDeRecepcion;

	pthread_t hiloManejadorKernel, hiloManejadorTeclado,hiloManejadorCPU; //Declaro hilos para manejar las conexiones


	fd_set aceptarConexiones, fd_setAuxiliar;
	FD_ZERO(&aceptarConexiones);
	FD_ZERO(&fd_setAuxiliar);
	//Socket escucha es el que va con el listen
	int socketMaximo = socketQueEscucha;
	aceptarConexiones = fd_setAuxiliar;
	pthread_create(&hiloManejadorTeclado,NULL,manejadorTeclado,NULL); //Creo un hilo para atender las peticiones provenientes de la consola de memoria
	log_info(loggerMemoria,"Conexion a consola memoira exitosa...\n");
	while (1) { //Chequeo nuevas conexiones y las voy separando a partir del handshake recibido (CPU o Kernel).
	  fd_setAuxiliar = aceptarConexiones;
	  if(select(socketQueEscucha,&fd_setAuxiliar,NULL,NULL,NULL)==-1){
		  perror("Error de select en memoria.");
		  exit(-1);
	  }
	  for(socketChequeado = 0; socketChequeado<=socketMaximo; socketChequeado++){
		  if(socketChequeado == socketMaximo){
			  socketQueAcepta = aceptarConexionDeCliente(socketQueEscucha);
			  FD_SET(socketQueAcepta,&aceptarConexiones);
			  socketMaximo = calcularSocketMaximo(socketQueAcepta,socketMaximo);
			  log_info(loggerMemoria,"Registro nueva conexion de socket: %d\n",socketQueAcepta);
		  }else{
			  if(recv(socketChequeado,&paqueteDeRecepcion.tipoMsj,sizeof(int),0)==-1){
				  perror("Error de send en memoria");
				  exit(-1);
			  }else{
				  switch(paqueteDeRecepcion.tipoMsj){
				  	  case ES_CPU:
				  		  pthread_create(&hiloManejadorCPU,NULL,manejadorConexionCPU,(void *)socketChequeado);
				  		  log_info(loggerMemoria,"Se registro nueva CPU...\n");
				  		  break;
				  	  case ES_KERNEL:
				  		  pthread_create(&hiloManejadorKernel,NULL,manejadorConexionKernel,(void *)socketChequeado);
				  		  log_info(loggerMemoria,"Se registro conexion de Kernel...\n");
				  		  break;
				  	  default:
				  		  puts("Conexion erronea");
				  		  FD_CLR(socketChequeado,&aceptarConexiones);
				  		  close(socketChequeado);
				  }
			  }
		  }
	  }
	}
}
