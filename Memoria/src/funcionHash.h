/*
 * funcionHash.h
 *
 *  Created on: 12/7/2017
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

typedef struct __attribute__((packed)){
    int frame;
    int pid;
    int pagina;
}entradaTabla;

typedef struct{
    int pid;
    int numPagina;
    void* contenido;
}entradaDeCache;

typedef struct{
    t_list* entradasCache;
}cache;

typedef struct{
    int pid;
    int contadorDePaginasPedidas;
    int cantidadDePaginasAsignadas;
}paginasPorProceso;

#ifndef FUNCIONHASH_H_
#define FUNCIONHASH_H_

//HANDSHAKES
#define TAMANIO_PAGINA_PARA_KERNEL 1020
#define TAMANIO_PAGINA_PARA_CPU 1020
#define HANG_UP_KERNEL 0
#define ES_CPU 1001
#define HANG_UP_CPU 0
#define ES_KERNEL 1002
//
//#define FALTA_RECURSOS_MEMORIA -50
//#define ESPACIO_INSUFICIENTE -2
//
//#define INICIO_EXITOSO 50
//#define SE_PUDO_COPIAR 2001
//#define NO_SE_PUEDE_COPIAR -2001


//OPERACIONES CON MEMORIA
#define INICIALIZAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define LEER_DATOS 504
#define ESCRIBIR_DATOS 505
#define LIBERAR_PAGINA 506

//RESULTADO DE OPERACIONES
#define OPERACION_EXITOSA_FINALIZADA 1
#define OPERACION_FALLIDA -1
#define DATOS_DE_PAGINA 103






t_log * loggerMemoria;
void* memoriaSistema;
entradaTabla* entradasDeTabla;
//void* punteroAPrincipioDeDatos;
pthread_mutex_t mutexTablaInvertida;
t_list *paginasPorProcesos;
int cantidadPaginasDeTabla;
int cantidadPaginasLibres;
cache *cacheDeMemoria;
int socketKernel;

//-----VALORES DE MEMORIA--------//
int PUERTO;
int MARCOS;
int MARCOS_SIZE;
int ENTRADAS_CACHE;
char* REEMPLAZO_CACHE;
int CACHE_X_PROC;
int RETARDO_MEMORIA;


//-----BUSCADOR DE PAGINAS
bool esElFrameCorrecto(int, int, int);
bool esPaginaLibre(int, int, int);
int buscarFrameProceso(int, int, bool(*closure)(int, int, int));
#endif /* FUNCIONHASH_H_ */
