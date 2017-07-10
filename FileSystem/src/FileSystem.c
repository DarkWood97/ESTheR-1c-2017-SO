#include "funcionesGenericas.h"
#include "socket.h"
#include <commons/bitarray.h>
#include <commons/txt.h>
#include <sys/mman.h>

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
char* mmapBitmap;

//Lo que recibo
char* pathDeArchivo;
char modoDeArchivo;
int	offset;
int size;
char* buffer;

//------CONFIGURACION FILESYSTEM-----------------------------------------------
void inicializarMmap(){
	int size;
	struct stat infoArchivo;
	int file= open("../Metadata/Bitmap.bin",O_RDWR);
	int status = fstat (file,&infoArchivo);
	size=infoArchivo.st_size;
	mmapBitmap = mmap(0,size,PROT_READ,PROT_WRITE|MAP_SHARED,file,0);
}



char* concatenarPath(char* path){
	char* cadenaAux=strlen(punto_Montaje);
	cadenaAux= punto_Montaje;
	string_append(cadenaAux,"/Archivos/");
	string_append(cadenaAux,path);
	return cadenaAux;
}

char* armarNombreArchivo(int bloque){
	char* bloqueAux=malloc(strlen(bloque));
	bloqueAux=string_itoa(bloque);
	string_append(bloqueAux,".bin");
	return bloqueAux;
}

void fileSystemCrear(t_config *configuracion) {
	verificarParametrosCrear(configuracion,2);
	puerto = config_get_int_value(configuracion, "PUERTO");
	punto_Montaje = string_new();
	string_append(&punto_Montaje, config_get_string_value(configuracion,"PUNTO_MONTAJE"));
}

void inicializarFileSystem(char *path) {
	t_config *configuracionFileSystem = generarT_ConfigParaCargar(path);
	fileSystemCrear(configuracionFileSystem);
	config_destroy(configuracionFileSystem);
}
void mostrarConfiguracionesFileSystem() {
	printf("PUERTO=%d\n", puerto);
	printf("PUNTO_MONTAJE=%s\n", punto_Montaje);
}

void metadataFileSystem(){
	//char* cadenaAux=malloc(strlen(punto_Montaje));
	//cadenaAux=punto_Montaje;
	//string_append(&cadenaAux,"Metadata/Metadata.bin");
	void* bufferDeInts;
	bufferDeInts = malloc(sizeof(int)*2);
	FILE *archivoFileSystem = txt_open_for_append("Metadata.txt");
	fread(&bufferDeInts, sizeof(int), 3, archivoFileSystem);
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

//----------------------------FUNCIONES ARCHIVOS------------------------------------------------

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
		int* bloque;
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
	  char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	  cadena=concatenarPath(pathDeArchivo);
	  int tamanio;
	  int* bloques;
	  fread(&tamanio,sizeof(int),1,cadena);
	  int largo= cantidadBloques(tamanio);
	  fread(&bloques,sizeof(int),largo-1,cadena);
	  liberarBloques(bloques);
	  if(remove(cadena)==0){
		return true;
	  }
	  return false;
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

void leerBlqoues(int tamanioArchivo, int* bloques){
	int bloque=offset/tamanio_bloque;
	int resto = (tamanio_bloque*bloque)-(offset+size);//Chequear, sino modulo
	//Hacer malloc
	char* datosDelBloque=malloc(size);
	char* cadenaBloque=malloc(strlen(bloques[bloque])+strlen(".bin"));
	cadenaBloque=armarNombreArchivo(bloques[bloque]);
	char* path=malloc(strlen(punto_Montaje)+strlen(cadenaBloque));
	path=concatenarPath(cadenaBloque);
	if((offset+size) < tamanioArchivo){
		fseek(path,offset,SEEK_SET);
		if(resto>0){
			//Moridificar size
			fread(&datosDelBloque,sizeof(datosDelBloque),size,path);
			if(send(socketKernel,&datosDelBloque,sizeof(datosDelBloque),0)==-1){
				log_info(loggerFS, "", cadenaBloque);
				exit(-1);
			}
		}else{
			fread(&datosDelBloque,sizeof(datosDelBloque),(size-resto),path);
			int sobrante=(offset+size)-(tamanio_bloque*bloque);
			while(sobrante>tamanio_bloque){
				bloque++;
				//Abrir archivo
				cadenaBloque=armarNombreArchivo(bloques[bloque]);
				path=concatenarPath(cadenaBloque);
				fseek(path,0,SEEK_SET);
				if(sobrante<tamanio_bloque){
					fread(&datosDelBloque,sizeof(datosDelBloque),sobrante,path);
				}else{
					fread(&datosDelBloque,sizeof(datosDelBloque),tamanio_bloque,path);
				}
				sobrante=sobrante-tamanio_bloque;
			}
		}
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

void guardarBloques(int tamanioArchivo,int*bloques){
	int bloque=offset/tamanio_bloque;
	int resto = (tamanio_bloque*bloque)-(offset+size);//Chequear, sino modulo

	char* datosDelBloque=malloc(size);
	char* cadenaBloque=malloc(strlen(bloques[bloque])+strlen(".bin"));
	cadenaBloque=armarNombreArchivo(bloques[bloque]);
	char* path=malloc(strlen(punto_Montaje)+strlen(cadenaBloque));
	path=concatenarPath(cadenaBloque);

	if((offset+size) < tamanioArchivo){
		fseek(path,offset,SEEK_SET);
		if(resto>0){
			fwrite(buffer,sizeof(buffer),size,path);
		}else{
			fwrite(buffer,sizeof(buffer),(size-resto),path);
			int sobrante=(offset+size)-(tamanio_bloque*bloque);
			while(sobrante>tamanio_bloque){
				bloque++;
				cadenaBloque=armarNombreArchivo(bloques[bloque]);
				path=concatenarPath(cadenaBloque);
				fseek(path,0,SEEK_SET);
				if(sobrante<tamanio_bloque){
					fwrite(buffer,sizeof(buffer),sobrante,path);
				}else{
					fwrite(buffer,sizeof(buffer),tamanio_bloque,path);
				}
				sobrante=sobrante-tamanio_bloque;
			}
		}
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

int main(int argc, char *argv[]) {
	loggerFS = log_create("FileSystem.log", "FileSystem", 0, 0);
	char *path="Debug/fileSystem.config";
	inicializarFileSystem(path);
	//verificarParametrosInicio(argc);
	//inicializarFileSystem(argv[1]);
	mostrarConfiguracionesFileSystem();
	int socketEscuchaFS;
	socketEscuchaFS = ponerseAEscucharClientes(puerto, 0);
	metadataFileSystem();
	inicializarMmap();
	bitmap = bitarray_create(mmapBitmap, string_length(mmapBitmap));
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
