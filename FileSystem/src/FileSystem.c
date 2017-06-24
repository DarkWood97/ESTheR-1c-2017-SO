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
#include <commons/bitarray.h>

#define ES_KERNEL 1
#define CORTO_KERNEL 2
#define GUARDAR_DATOS 3
#define OBTENER_DATOS 4
#define CREAR_ARCHIVO 6
#define BORRAR_ARCHIVO 7
#define VALIDAR_ARCHIVO 8
#define HANDSHAKE_ACEPTADO 9
//--TYPEDEF--------------------------------------------------------------

typedef struct __attribute__((packed)) {
	int tamanio;
	int *bloques;
} archivo;

t_log* loggerFS;
bool hayKernelConectado = false;
int puerto;
char *punto_Montaje;
int tamanio_bloque;
int cantidad_bloque;
t_bitarray 	*bitmap;
int socketKernel;

//Lo que recibo
char* pathDeArchivo;
char modoDeArchivo;
int	offset;
int size;
char* buffer;

//------CONFIGURACION FILESYSTEM-----------------------------------------------
char* concatenarPath(char* path){
	char* cadenaAux=strlen(punto_Montaje);
	cadenaAux= punto_Montaje;
	string_append(cadenaAux,"/Archivos/");
	string_append(cadenaAux,path);
	return cadenaAux;
}

char* concatenarBloque(char* bloque){
	char* cadenaAux=strlen(punto_Montaje);
	cadenaAux= punto_Montaje;
	string_append(cadenaAux,"/Archivos/");
	string_append(bloque,".bin");
	string_append(cadenaAux,bloque);
	return cadenaAux;
}

void fileSystemCrear(t_config *configuracion) {
	verificarParametrosCrear(configuracion,2);
	puerto = config_get_int_value(configuracion, "PUERTO");
	punto_Montaje = config_get_string_value(configuracion,"PUNTO_MONTAJE");
}

void inicializarFileSystem(char *path) {
	t_config *configuracionFileSystem = (t_config*)malloc(sizeof(t_config));
	*configuracionFileSystem = generarT_ConfigParaCargar(path);
	fileSystemCrear(configuracionFileSystem);
	free(configuracionFileSystem);
}
void mostrarConfiguracionesFileSystem() {
	printf("PUERTO=%d\n", puerto);
	printf("PUNTO_MONTAJE=%s\n", punto_Montaje);
}

void metadataFileSystem(){
	char* cadenaAux=strlen(punto_Montaje);
	cadenaAux=punto_Montaje;
	string_append(cadenaAux,"/Metadata/Metadata.bin");
	void* bufferDeInts;
	bufferDeInts = malloc(sizeof(int)*2);
	FILE *archivoFileSystem = fopen(cadenaAux, "r+w");
	fread(bufferDeInts, sizeof(int), 2, archivoFileSystem);
	int cantBloques, tamanioBloques;
	memcpy(&cantBloques, bufferDeInts, sizeof(int));
	memcpy(&tamanioBloques, bufferDeInts+sizeof(int), sizeof(int));
	fclose(archivoFileSystem);
	free(bufferDeInts);
}

long int obtenerTamanioArchivo(FILE* archivo){
	 long int nTamArch;
	 fseek (archivo, 0, SEEK_END);
	 nTamArch = ftell (archivo);
	 return nTamArch;
}

void cargarBitmap(){
	//Hacer malloc
	char* bitmapAux;
	char* cadenaAux=strlen(punto_Montaje);
	cadenaAux=punto_Montaje;
	string_append(cadenaAux,"/Metadata/Bitmap.bin");
	FILE *archivo = fopen(cadenaAux, "r+w");
	int tamanio=obtenerTamanioArchivo(archivo)-1;
	fread(bitmapAux, tamanio, 1, archivo);//Que va?
	size_t size=strlen(bitmapAux);
	bitmap = bitarray_create_with_mode(bitmapAux, size , MSB_FIRST);
}
//----------------------------FUNCIONES ARCHIVOS------------------------------------------------
void leerBlqoues(int tamanioArchivo, int* bloques){
	int bloque=offset/tamanio_bloque;
	int resto = (tamanio_bloque*bloque)-(offset+size);//Chequear, sino modulo
	//Hacer malloc
	char* datosDelBloque;

	char* cadenaBloque=malloc(strlen(punto_Montaje)+strlen(bloques[bloque])+strlen("/Archivos/")+strlen);
	cadenaBloque=concatenarBloque(bloques[bloque]);
	if((offset+size) < tamanioArchivo){
		fseek(cadenaBloque,offset,SEEK_SET);
		if(resto>0){
			fread(datosDelBloque,sizeof(datosDelBloque),size,cadenaBloque);
			if(send(socketKernel,&datosDelBloque,sizeof(datosDelBloque),0)==-1){
				log_info(loggerFS, "", cadenaBloque);
				exit(-1);
			}
		}else{
			fread(datosDelBloque,sizeof(datosDelBloque),(size-resto),cadenaBloque);

			//resto=resto-tamanio_bloque;
			//Solucionar el tema del send
			//if(send(socket,&datosDelBloque,sizeof(datosDelBloque),0)==-1){
				//log_info(loggerFS, "Error al enviar existencia de archivo.", cadena);
				//exit(-1);
			//}
			int sobrante=(offset+size)-(tamanio_bloque*bloque);
			while(sobrante>tamanio_bloque){
				bloque++;
				cadenaBloque=concatenarBloque(bloques[bloque]);
				fseek(cadenaBloque,0,SEEK_SET);
				if(sobrante<tamanio_bloque){
					fread(datosDelBloque,sizeof(datosDelBloque),sobrante,cadenaBloque);
				}else{
					fread(datosDelBloque,sizeof(datosDelBloque),tamanio_bloque,cadenaBloque);
				}
				sobrante=sobrante-tamanio_bloque;
			}
		}
	}

//	int i=0;
//	for(;i<string_length(bloques);i++){
//
//
//		fread(datosDelBloque,sizeof(bloques),1,cadenaBloque);//Que size le pongo
//		if(send(socket,&datosDelBloque,sizeof(datosDelBloque),0)==-1){
//			log_info(loggerFS, "", cadenaBloque);
//			exit(-1);
//		}
//	}

}

void guardarBloques(int tamanioArchivo,int*bloques){
	int bloque=offset/tamanio_bloque;
	int resto = (tamanio_bloque*bloque)-(offset+size);//Chequear, sino modulo
	char* cadenaBloque=malloc(strlen(punto_Montaje)+strlen(bloques[bloque])+strlen("/Archivos/")+strlen);
	//Arreglar concatenacion de bloques--usar atoi
	cadenaBloque=concatenarBloque(bloques[bloque]);
	if((offset+size) < tamanioArchivo){
		fseek(cadenaBloque,offset,SEEK_SET);
		if(resto>0){
			fwrite(buffer,sizeof(buffer),size,cadenaBloque);
		}else{
			fwrite(buffer,sizeof(buffer),(size-resto),cadenaBloque);
			int sobrante=(offset+size)-(tamanio_bloque*bloque);
			while(sobrante>tamanio_bloque){
				bloque++;
				cadenaBloque=concatenarBloque(bloques[bloque]);
				fseek(cadenaBloque,0,SEEK_SET);
				if(sobrante<tamanio_bloque){
					fwrite(buffer,sizeof(buffer),sobrante,cadenaBloque);
				}else{
					fwrite(buffer,sizeof(buffer),tamanio_bloque,cadenaBloque);
				}
				sobrante=sobrante-tamanio_bloque;
			}
		}
	}
//
//	int i=0;
//	for(;i<string_length(bloques);i++){
//		fread(datosDelBloque,sizeof(bloques),1,cadenaBloque);//Que size le pongo
//		if(send(socket,&datosDelBloque,sizeof(datosDelBloque),0)==-1){
//			log_info(loggerFS, "", cadenaBloque);
//			exit(-1);
//		}
//	}
}

//Chequear
int bloqueVacio(){
	int i=0;
	int tamanioBitmap=bitarray_get_max_bit(bitmap);
	int aux = bitarray_get_max_bit(bitmap);
	int ocupado;
	for(;i<tamanioBitmap ;i++){
		ocupado= bitarray_test_bit(bitmap,aux);
		if(ocupado==0){
			bitarray_set_bit(bitmap, aux);
			return aux;
		}
		aux--;
	}
}

void liberarBloques(int* bloques){
	int i=0;
	for(;i<string_length(bloques);i++){
		bitarray_clean_bit(bitmap, bloques[i]);//Uso este a set?
	}
}

bool crearArchivoDeFS(){
		int bloque;
		int tamanio=0;
		char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
		cadena=concatenarPath(pathDeArchivo);
		if(modoDeArchivo=="c"){
			FILE *archivoRecibido = fopen(cadena, "r+w");
			bloque=bloqueVacio();
			fwrite(&tamanio,sizeof(int),1,archivoRecibido);
			fwrite(&bloque,sizeof(int),1,archivoRecibido);
			return true;
		}
		return false;
}

int cantidadBloques(int tamanio){
	return tamanio/tamanio_bloque;
}

bool seBorroArchivoDeFS(){
	  bool removidoCorrectamente = false;
	  char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	  cadena=concatenarPath(pathDeArchivo);
	  int tamanio;
	  int* bloques;
	  fread(&tamanio,sizeof(int),1,cadena);
	  int largo= cantidadBloques(tamanio);
	  fread(&bloques,sizeof(int),largo-1,cadena);
	  liberarBloques(bloques);
	  if(remove(cadena)==0){
		removidoCorrectamente = true;
		return removidoCorrectamente;
	  }
	  return removidoCorrectamente;
}


bool chequearArchivo(){
	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo);
	if(access(cadena, F_OK) == 0){
		return true;
	}else{
		return false;
	}
}

void verificarArchivo (){
	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo);
	if(!chequearArchivo()){
		crearArchivoDeFS(cadena);
		log_info(loggerFS, "Se creo el archivo %s exitosamente.",cadena);
	}else{
		 log_info(loggerFS, "No se pudo crear el archivo %s.", cadena);
	}
}

void obtenerDatosDelArchivo(){
	int tamanioArchivo;
	int* bloques;
	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo);
	if(modoDeArchivo=="r"){
		fread(&tamanioArchivo,sizeof(int),1,cadena);
		int largo= cantidadBloques(tamanioArchivo);
		fread(&bloques,sizeof(int),largo,cadena);
		leerBlqoues(tamanioArchivo,bloques);
		log_info(loggerFS, "Se obtuvieron los datos del archivo %s exitosamente.",cadena);
	}else{
		log_info(loggerFS, "Modo incorrecto del archivo %s .",cadena);
	}
}

void guardarDatosArchivo(){
	int tamanioArchivo;
	int* bloques;
	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo);
	if(modoDeArchivo=="w"){
		fread(&tamanioArchivo,sizeof(int),1,cadena);
		int largo= cantidadBloques(tamanioArchivo);
		fread(&bloques,sizeof(int),largo,cadena);
		guardarBloques(tamanioArchivo,bloques);
		log_info(loggerFS, "Se guardaron los datos del archivo %s exitosamente.",cadena);
	}else{
		log_info(loggerFS, "Modo incorrecto del archivo %s .",cadena);
	}
}

//---------------------------------------------------------------------------------
void recibirMensajeCrear(void* mensaje){
	int tamanioPath;
	memcpy(modoDeArchivo,mensaje,sizeof(char));
	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
}

void recibirMensajeObtenerDatos(void* mensaje){
	int tamanioPath;
	memcpy(modoDeArchivo,mensaje,sizeof(char));
	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
	memcpy(offset,mensaje+sizeof(char)+sizeof(int)+tamanioPath,sizeof(int));
	memcpy(size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int),sizeof(int));
}

void recibirMensajeGuardarDatos(void* mensaje){
	int tamanioPath;
	int tamanioBuffer;
	memcpy(modoDeArchivo,mensaje,sizeof(char));
	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
	memcpy(offset,mensaje+sizeof(char)+sizeof(int)+tamanioPath,sizeof(int));
	memcpy(size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int),sizeof(int));
	memcpy(tamanioBuffer,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int)+sizeof(int),sizeof(int));
	memcpy(size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int)+sizeof(int)+sizeof(int),tamanioBuffer);
}


int recibirEntero(){
  int enteroRecibido;
  if(recv(socketKernel,&enteroRecibido, sizeof(int), 0)==-1){
    log_info(loggerFS, "Error al recibir mensaje desde kernel.");
    exit(-1);
  }
  return enteroRecibido;
}

//char* recibirChar(int tamMsj){
//  char* path = malloc(tamMsj);
//  if(recv(socketKernel,path,tamMsj,0)==-1){
//    log_info(loggerFS,"Error al recibir el path de archivo.");
//    exit(-1);
//  }
//  return path;
//	//free(path);
//}
//---------------------------------HANDSHAKE-----------------------------------------
void recibirHandshakeDeKernel(){
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

//--------------------------------- MAIN -----------------------------------------

int main() {
	loggerFS = log_create("FileSystem.log", "FileSystem", 0, 0);
	char *path="FileSystem/fileSystem.config";
	inicializarFileSystem(path);
	mostrarConfiguracionesFileSystem();
	int socketEscuchaFS;
	socketEscuchaFS = ponerseAEscucharClientes(puerto, 0);
	metadataFileSystem();
	cargarBitmap();
	bool existeArchivo;
	  while(!hayKernelConectado){
	    int socketKernel = aceptarConexionDeCliente(socketEscuchaFS);
	    recibirHandshakeDeKernel(socketKernel);
	  }
	  while(1){
	    paquete paqueteRecibidoDeKernel;
	    paqueteRecibidoDeKernel.tipoMsj = recibirEntero();
	    paqueteRecibidoDeKernel.tamMsj = recibirEntero();
	    paqueteRecibidoDeKernel.mensaje=malloc(paqueteRecibidoDeKernel.tamMsj);
	    if(recv(socketKernel,&paqueteRecibidoDeKernel.mensaje,paqueteRecibidoDeKernel.tamMsj,0)==-1){
	        log_info(loggerFS,"Error al recibir el tamanio paquete.");
	        exit(-1);
	    }
	    switch (paqueteRecibidoDeKernel.tipoMsj) {
	      case VALIDAR_ARCHIVO:
	        existeArchivo = chequearArchivo(paqueteRecibidoDeKernel.mensaje);
	        if(send(socketKernel,&existeArchivo,sizeof(bool),0)==-1){
	          log_info(loggerFS, "Error al enviar existencia de archivo.", paqueteRecibidoDeKernel.mensaje);
	          exit(-1);
	        }
	        break;
	      case BORRAR_ARCHIVO:
	        if(seBorroArchivoDeFS(paqueteRecibidoDeKernel.mensaje)){
	          log_info(loggerFS, "Se borro el archivo %s exitosamente.",paqueteRecibidoDeKernel.mensaje);
	        }else{
	          log_info(loggerFS, "No se pudo borrar el archivo %s.", paqueteRecibidoDeKernel.mensaje);
	        }
	        break;
	      case CREAR_ARCHIVO:
	    	recibirMensajeCrear(paqueteRecibidoDeKernel.mensaje);
			verificarArchivo();
	        break;
	      case OBTENER_DATOS:
	    	recibirMensajeObtenerDatos(paqueteRecibidoDeKernel.mensaje);
			obtenerDatosDelArchivo();
	        break;
	      case GUARDAR_DATOS:
	    	recibirMensajeGuardarDatos(paqueteRecibidoDeKernel.mensaje);
			guardarDatosArchivo();
	        break;
	      case CORTO_KERNEL:
	        close(socketKernel);
	        break;
	      default:
			log_info(loggerFS, "Error de operacion");
				}

	    }
}

