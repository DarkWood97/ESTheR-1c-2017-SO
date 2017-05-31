/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include "funcionesGenericas.h"
#include "socket.h"

#define ES_KERNEL 1
#define CORTO_KERNEL 2
#define GUARDAR_DATOS 3
#define OBTENER_DATOS 4
#define CREAR_ARCHIVO 6
#define BORRAR_ARCHIVO 7
#define VALIDAR_ARCHIVO 8
#define HANDSHAKE_ACEPTADO 9
//--TYPEDEF--------------------------------------------------------------
typedef struct {
	int puerto;
	char *punto_montaje;
} fileSystem;

t_log* loggerFS;
bool hayKernelConectado = false;


//------FUNCIONES FILESYSTEM-----------------------------------------------
fileSystem fileSystemCrear(t_config *configuracion) {
	fileSystem fileSystemAuxiliar;
	verificarParametrosCrear(configuracion,2);
	fileSystemAuxiliar.puerto = config_get_int_value(configuracion, "PUERTO");
	fileSystemAuxiliar.punto_montaje = config_get_string_value(configuracion,"PUNTO_MONTAJE");
	return fileSystemAuxiliar;
}

fileSystem inicializarFileSystem(char *path) {
	t_config *configuracionFileSystem = (t_config*)malloc(sizeof(t_config));
	*configuracionFileSystem = generarT_ConfigParaCargar(path);
	fileSystem fileSystemSistema = fileSystemCrear(configuracionFileSystem);
	return fileSystemSistema;
	free(configuracionFileSystem);
}
void mostrarConfiguracionesFileSystem(fileSystem fileSystem) {
	printf("PUERTO=%d\n", fileSystem.puerto);
	printf("PUNTO_MONTAJE=%s\n", fileSystem.punto_montaje);
}
//-----FUNCIONES ARCHIVOS------------------------------------------------
bool seBorroArchivoDeFS(char* pathDeArchivo){
  bool removidoCorrectamente = false;
  if(remove(pathDeArchivo)!=-1){
    removidoCorrectamente = true;
    return removidoCorrectamente;
  }
  return removidoCorrectamente;

}

int recibirEntero(int socketKernel){
  int enteroRecibido;
  if(recv(socketKernel,&enteroRecibido, sizeof(int), 0)==-1){
    log_info(loggerFS, "Error al recibir mensaje desde kernel.");
    exit(-1);
  }
  return enteroRecibido;
}

void recibirHandshakeDeKernel(int socketKernel){
  int quienEs;
  int handshakeRecibido = HANDSHAKE_ACEPTADO;
  if(recv(socketKernel, &quienEs, sizeof(int), 0) == -1){
    log_info(loggerFS, "Error al recibir handshake");
    exit(-1);
  }
  switch(quienEs){
    case ES_KERNEL:
      hayKernelConectado = true;
      if(send(socketKernel, &handshakeRecibido, sizeof(int),0) == -1){
        log_info(loggerFS, "Error al enviar aceptacion de handshake");
        exit(-1);
      }
      break;
    default:
      log_info(loggerFS, "Se recibio conexion erronea de %d", socketKernel);
      close(socketKernel);
  }
}

bool chequearArchivo(char* pathDeArchivo){
  if(access(pathDeArchivo, F_OK) != -1){
    return true;
  }else{
    return false;
  }
}

char* recibirPath(int tamMsj, int socketKernel){
  char* path = malloc(tamMsj);
  if(recv(socketKernel,path,tamMsj,0)==-1){
    log_info(loggerFS,"Error al recibir el path de archivo.");
    exit(-1);
  }
  return path;
}


int main() {
	loggerFS = log_create("FileSystem.log", "FileSystem", 0, 0);
	char *path="FileSystem/fileSystem.config";
	fileSystem fileSystem = inicializarFileSystem(path);
	mostrarConfiguracionesFileSystem(fileSystem);
	int socketClienteParaKernel, socketEscuchaFS;
	socketEscuchaFS = ponerseAEscucharClientes(fileSystem.puerto, 0);
	char* pathDeArchivo;
	bool existeArchivo;
	  while(!hayKernelConectado){
	    int socketClienteParaKernel = aceptarConexionDeCliente(socketEscuchaFS);
	    recibirHandshakeDeKernel(socketClienteParaKernel);
	  }
	  while(1){
	    paquete paqueteRecibidoDeKernel;
	    paqueteRecibidoDeKernel.tipoMsj = recibirEntero(socketClienteParaKernel);
	    paqueteRecibidoDeKernel.tamMsj = recibirEntero(socketClienteParaKernel);
	    switch (paqueteRecibidoDeKernel.tipoMsj) {
	      case VALIDAR_ARCHIVO:
	        pathDeArchivo = malloc(paqueteRecibidoDeKernel.tamMsj);
	        pathDeArchivo = recibirPath(paqueteRecibidoDeKernel.tamMsj,socketClienteParaKernel);
	        existeArchivo = chequearArchivo(pathDeArchivo);
	        if(send(socketClienteParaKernel,&existeArchivo,sizeof(bool),0)==-1){
	          log_info(loggerFS, "Error al enviar existencia de archivo.");
	          exit(-1);
	        }
	        break;
	      case BORRAR_ARCHIVO:
	        pathDeArchivo = recibirPath(paqueteRecibidoDeKernel.tamMsj,socketClienteParaKernel);
	        if(seBorroArchivoDeFS(pathDeArchivo)){
	          log_info(loggerFS, "Se borro el archivo %s exitosamente.",pathDeArchivo);
	        }else{
	          log_info(loggerFS, "No se pudo borrar el archivo %s.", pathDeArchivo);
	        }
	        break;
	      case CREAR_ARCHIVO:
	        pathDeArchivo = recibirPath(paqueteRecibidoDeKernel.tamMsj,socketClienteParaKernel);

	        break;
	      case OBTENER_DATOS:
	        pathDeArchivo = recibirPath(paqueteRecibidoDeKernel.tamMsj,socketClienteParaKernel);

	        break;
	      case GUARDAR_DATOS:
	        pathDeArchivo = recibirPath(paqueteRecibidoDeKernel.tamMsj,socketClienteParaKernel);

	        break;
	      case CORTO_KERNEL:
	        close(socketClienteParaKernel);
	        break;

	    }
	  }
	  return 0;
}
