#include "funcionesGenericas.h"
#include "socket.h"
#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <sys/mman.h>
#include <math.h>

#define ES_KERNEL 1006
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
t_bitarray 	*bitarray;
int socketKernel;
char* mmapBitmap;

//FUNCION APARTE
t_config *generarT_Config(char *path) {
	t_config *configuracionDelComponente;
	if((configuracionDelComponente = config_create(path)) == NULL){
		perror("Error de ruta de archivo");
		exit(-1);
	}
	return configuracionDelComponente;
}

//------CONFIGURACION FILESYSTEM-----------------------------------------------
int abrirBitmap(){
	log_debug(loggerFS,"Se procede abrir el archivo Bitmap.bin");
	char* cadena=concatenarPath("Bitmap.bin","/Metadata/");
	int file= open(cadena,O_RDWR);
	return file;
}

int crearBitmap(){
	log_debug(loggerFS,"Se procede a crear el archivo Bitmap.bin");
	char* cadena=concatenarPath("Bitmap.bin","/Metadata/");
	int file = open(cadena,O_RDWR);
	int cantidad = ceil(((double)cantidad_bloque)/8.0);
	char* contenido=string_repeat('\0',cantidad);
	if(file<0){
		log_info(loggerFS,"No se pudo crear el archivo");
	}
	write(file,contenido,cantidad);
	return file;
}



void inicializarMmap(){
	int file=abrirBitmap();
	if(file<0){
		file=crearBitmap();
	}
	struct stat myStat;
	if (fstat(file, &myStat) < 0) {
	    printf("Error al establecer fstat\n");
	    close(file);
	}
	mmapBitmap = mmap(0,myStat.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,file,0);

	if (mmapBitmap == MAP_FAILED) {
			printf("Error al mapear a memoria: %s\n", strerror(errno));
			close(file);
	}

	bitarray = bitarray_create_with_mode(mmapBitmap,5200/8, MSB_FIRST);
}




void fileSystemCrear(t_config *configuracion) {
 verificarParametrosCrear(configuracion,2);
 puerto = config_get_int_value(configuracion, "PUERTO");
 punto_Montaje = string_new();
 string_append(&punto_Montaje, config_get_string_value(configuracion, "PUNTO_MONTAJE"));
}

void inicializarFileSystem(char *path) {
	t_config *configuracionFileSystem=generarT_ConfigParaCargar(path);
	fileSystemCrear(configuracionFileSystem);
	config_destroy(configuracionFileSystem);
}
void mostrarConfiguracionesFileSystem() {
	printf("PUERTO=%d\n", puerto);
	printf("PUNTO_MONTAJE=%s\n", punto_Montaje);
}

void mostrarMetadata(){
	printf("TAMANIO_BLOQUES=%d\n", tamanio_bloque);
	printf("CANTIDAD_BLOQUES=%d\n", cantidad_bloque);
}


void obtenerMetadata (t_config *metadata){
	//vereficarParametrosCrear(metadata,3);
	tamanio_bloque=config_get_int_value(metadata,"TAMANIO_BLOQUES");
	cantidad_bloque=config_get_int_value(metadata,"CANTIDAD_BLOQUES");
}


void metadataFileSystem(){
//	char* path=malloc(strlen(punto_Montaje)+strlen("/Metadata/Metadata.config")); // CREO QUE NO VA
//	string_append(&path,punto_Montaje);
//	string_append(&path,"/Metadata/Metadata.config");
	char *path="Debug/Metadata.config";
	t_config* metadata =generarT_Config(path);
	obtenerMetadata(metadata);
	config_destroy(metadata);
}

long int obtenerTamanioArchivo(FILE* archivo){
	 long int nTamArch;
	 fseek (archivo, 0, SEEK_END);
	 nTamArch = ftell (archivo);
	 return nTamArch;
}

//-------------------------------------------OPERACIONES CON RUTAS-------------------------------------
char* concatenarPath(char* path, char* carpeta){
	char* cadenaAux=malloc(string_length(punto_Montaje)+string_length(path)+string_length(carpeta));
	string_append(&cadenaAux, punto_Montaje);
	string_append(&cadenaAux,carpeta);
	string_append(&cadenaAux,path);
	return cadenaAux;
}

char* armarNombreArchivo(int bloque){
	char* bloqueAux=malloc(sizeof(int)+string_length(".bin"));
	string_append(&bloqueAux, string_itoa(bloque));
	string_append(&bloqueAux,".bin");
	return bloqueAux;
}



//----------------------------FUNCIONES ARCHIVOS------------------------------------------------

//Validar Archivo
bool validarArchivo(char* path){
	char* cadena=malloc(strlen(punto_Montaje)+strlen(path)+strlen("/Archivos/"));
	cadena=concatenarPath(path,"/Archivos/");
	FILE* archivo=fopen(cadena,"r");
	if (archivo == NULL) {
		log_info(loggerFS,"No existe el archivo");
		return false;
	} else {
		log_info(loggerFS,"Existe el archivo");
		return true;
	}
}

//Borrar Archivo
void liberarBloques(char** bloques){
	int i=0;
	char* pathBloque;
	char* pathArchivo;
	int bloquePosicion;
	for(;bloques[i]!=NULL;i++){
		bloquePosicion=atoi(bloques[i]);
		pathBloque=string_new();
		pathArchivo=string_new();
		pathBloque=armarNombreArchivo(atoi(bloques[i]));
		pathArchivo=concatenarPath(pathBloque,"/Bloques/");
		remove(pathArchivo);
		bitarray_clean_bit(bitarray, bloquePosicion);
		free(pathBloque);
		free(pathArchivo);
	}
}

bool seBorroArchivoDeFS(char* pathDeArchivo){
	  char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	  cadena=concatenarPath(pathDeArchivo,"/Archivos");
	  t_config* archivo=generarT_Config(cadena);
	  int tamanio=config_get_int_value(archivo,"TAMANIO");
	  char** bloques= config_get_array_value(archivo,"BLOQUES");
	  config_destroy(archivo);
	  if(remove(cadena)>=0){
		  liberarBloques(bloques);
		  log_info(loggerFS,"se borro el archivo %s correctamente",cadena);
		  return true;
	  }else{
		  log_info(loggerFS,"se borro el archivo %s correctamente",cadena);
		  return false;
	  }

}

//Crear archivo
int obtenerBloque(){
	int i=0;
	for(;i<bitarray_get_max_bit(bitarray);i++){
		if(!bitarray_test_bit(bitarray,i)){
			return i;
		}
	}
	return 0;
}

void crearPathBloque(int nuevoBloque){
	char* cadenaBloque=armarNombreArchivo(nuevoBloque);
	char* path=concatenarPath(cadenaBloque,"/Bloques/");
	//VER COMO CREAR DIRECTORIOS

}

void asignarBloqueArchivo(int nuevoBloque,char* cadena){
	t_config* datosArchivo=config_create(cadena);
	int tamanioArchivo=config_get_int_value(datosArchivo, "TAMANIO");
	char** bloques=config_get_array_value(datosArchivo,"BLOQUES");
	FILE* archivo=fopen(cadena,"w+");
	char* tamanio="TAMANIO";
	string_append(&tamanio,string_itoa(tamanioArchivo));
	fputs(tamanio,archivo);
	fputc('\n',archivo);
	fputs("BLOQUES=[",archivo);
	int i=0;
	while(bloques[i]!=NULL){
		fputs(bloques[i],archivo);
		fputc(',',archivo);
		i++;
	}
	bitarray_set_bit(bitarray,nuevoBloque);
	crearPathBloque(nuevoBloque);
	fputs(string_itoa(nuevoBloque),archivo);
	fputc(']',archivo);
	fputc('\n',archivo);
}

void crearArchivo (void* mensaje){
	char modoDeArchivo;
	int tamanioPath=0;
	char* pathDeArchivo=string_new();
	memcpy(modoDeArchivo,mensaje,sizeof(char));
	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
	int bloque;
	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo,"/Archivos/");
//	FILE* archivo=fopen(cadena,"w+");
//	if(archivo!=NULL){
//		fputs("TAMANIO=0",archivo);
//		fputc('\n',archivo);
//		fputs("BLOQUES=[",archivo);
		bloque=obtenerBloque();
		//NO TENDRIA QUE INICIALIZAR EL PATH BLOQUE
		if(bloque!=0){
//			fputs(string_itoa(bloque),archivo);
			asignarBloqueArchivo(bloque,cadena);
			log_info(loggerFS,"Se asigna a %s correctamente el bloque %d",cadena,bloque);
			log_info(loggerFS, "Se creo el archivo %s exitosamente.",cadena);
		}else{
			log_info(loggerFS,"No se pudo crear el archivo %s por falta de bloques disponibles",cadena);
		}
//		fputc(']',archivo);
//		fputc('\n',archivo);
//	}else{
//		log_info(loggerFS, "No se pudo crear el archivo %s.", cadena);
//	}
}


// OBTENER DATOS
void leerBloques(int tamanioArchivo, char** bloques, int size, int offset){
	char* datosDelBloque=string_new();
	int primerBloqueALeer=offset/tamanio_bloque;

	int faltante = (tamanio_bloque*(primerBloqueALeer+1))-(offset+size);//+1 por si cae en el primer bloque-->0

	if(tamanioArchivo>(size+offset)){
		//Creo el path del bloque
		int bloque=atoi(bloques[primerBloqueALeer]);
		char* cadenaBloque=string_new();
		cadenaBloque=armarNombreArchivo(bloque);
		char* path=string_new();
		path=concatenarPath(cadenaBloque,"/Bloques/");
		FILE* archivoBloque=fopen(path,"r");
		fseek(archivoBloque,offset,SEEK_SET);

		if(faltante>0){
			fgets(datosDelBloque,size,archivoBloque);

		}else{
			fgets(datosDelBloque,(tamanio_bloque-offset),archivoBloque);
			fclose(archivoBloque);
			primerBloqueALeer++;

			faltante=(size+offset)-tamanio_bloque;//Lo que falta leer del size (positivo)
			while(faltante>0){
				//Paso al siguiente bloque
				bloque=atoi(bloques[primerBloqueALeer]);
										//ESTA BIEN HACER OTRO STRIGN NEW Y FILE?
				char* cadenaBloque=string_new();
				cadenaBloque=armarNombreArchivo(bloque);
				char* path=string_new();
				path=concatenarPath(cadenaBloque,"/Bloques/");
				FILE* archivoBloque=fopen(path,"r");
				fseek(archivoBloque,0,SEEK_SET);

				char* leido=malloc(faltante);

				if(faltante<tamanioArchivo){
					fgets(leido,faltante,archivoBloque);
					faltante=faltante-faltante;
					string_append(&datosDelBloque,leido);
				}else{
					fgets(leido,tamanio_bloque,archivoBloque);
					faltante=faltante-tamanio_bloque;
					string_append(&datosDelBloque,leido);
				}
				primerBloqueALeer++;
			}
		}
		//MANDAR A KERNEL

	}else{
		log_info(loggerFS,"Lo pedido para leer supera el tamanio del bloque");
	}

}

void obtenerDatosDelArchivo(void* mensaje){
	int tamanioPath=0,size=0,offset=0;
	char* pathDeArchivo=string_new();
	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
	memcpy(offset,mensaje+sizeof(char)+sizeof(int)+tamanioPath,sizeof(int));
	memcpy(size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int),sizeof(int));

	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo,"/Archivos/");

	t_config* archivo=generarT_Config(cadena);
	int tamanio=config_get_int_value(archivo,"TAMANIO");
	char** bloques= config_get_array_value(archivo,"BLOQUES");
	log_info(loggerFS,"Se encontro el archivo %s y se procede a leer los datos del mismo",cadena);
	leerBloques(tamanio,bloques,size,offset);
}

// GUARDAR DATOS



bool guardarBloques(char**bloques,int tamanioArchivo, int size,int offset,char* buffer,char* cadena){
	int primerBloqueALeer=offset/tamanio_bloque;

	int faltante = (tamanio_bloque*(primerBloqueALeer+1))-(offset+size);//+1 por si cae en el primer bloque-->0
	//Creo el path del bloque
	int bloque=atoi(bloques[primerBloqueALeer]);
	char* cadenaBloque=string_new();
	cadenaBloque=armarNombreArchivo(bloque);
	char* path=string_new();
	path=concatenarPath(cadenaBloque,"/Bloques/");
	FILE* archivoBloque=fopen(path,"r");
	fseek(archivoBloque,offset,SEEK_SET);//LO ubico al partir del dezplazamiento

	if(faltante>0){
		fputs(buffer[size],archivoBloque);
	}else{
		fputs(buffer[tamanio_bloque-offset],archivoBloque);
		fclose(archivoBloque);
		faltante=(size+offset)-(tamanio_bloque*(primerBloqueALeer+1)); //Lo que falta guardar del buffer (positivo)

		primerBloqueALeer++;

		while(faltante>0){
			//preguntar si hay bloque disponible o no
			if(bloques[primerBloqueALeer]!= NULL){
				int nuevoBloque=obtenerBloque();//Le asigno un bloque extra para que pueda seguir grabando
				if(nuevoBloque!=0){
					asignarBloqueArchivo(nuevoBloque,cadena);
				}else{
					log_info(loggerFS,"No se encontro bloques disponibles para el archivo");
					//MANDAR KERNEL ERROR
					return false;
				}

			}

			//Paso al siguiente bloque
			bloque=atoi(bloques[primerBloqueALeer]);
										//ESTA BIEN HACER OTRO STRIGN NEW Y FILE?
			char* cadenaBloque=string_new();
			cadenaBloque=armarNombreArchivo(bloque);
			char* path=string_new();
			path=concatenarPath(cadenaBloque,"/Bloques/");
			FILE* archivoBloque=fopen(path,"r");
			fseek(archivoBloque,0,SEEK_SET);

			if(faltante<tamanio_bloque){
				fputs(buffer[faltante],archivoBloque);
				faltante=faltante-faltante;
			}else{
				fputs(buffer[tamanio_bloque],archivoBloque);
				faltante=faltante-tamanio_bloque;
			}
			primerBloqueALeer++;

		}
	}
	//MANDAR A KERNEL
	return true;
}

void guardarDatosArchivo(void* mensaje){
	int tamanioPath,tamanioBuffer,size,offset;
	char* pathDeArchivo,buffer;
	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
	memcpy(offset,mensaje+sizeof(char)+sizeof(int)+tamanioPath,sizeof(int));
	memcpy(size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int),sizeof(int));
	memcpy(tamanioBuffer,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int)+sizeof(int),sizeof(int));
	memcpy(buffer,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int)+sizeof(int)+sizeof(int),tamanioBuffer);

	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo,"/Archivos/");

	t_config* archivo=generarT_Config(cadena);
	int tamanioArchivo=config_get_int_value(archivo,"TAMANIO");
	char** bloques= config_get_array_value(archivo,"BLOQUES");
	log_info(loggerFS,"Se encontro el archivo %s y se procede a guardar los datos %s en el mismo",cadena,buffer);
	if(guardarBloques(bloques,tamanioArchivo,size,offset,buffer,cadena)){
		log_info(loggerFS,"Se guardo correctamente el %s en la cadena",buffer,cadena);
	}else{
		log_info(loggerFS,"No se pudo guardar correctamente el %s en la cadena",buffer,cadena);
	}

}

//---------------------------------------------------------------------------------
//void recibirMensajeCrear(void* mensaje){
//	int tamanioPath;
//	memcpy(modoDeArchivo,mensaje,sizeof(char));
//	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
//	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
//}

//void recibirMensajeObtenerDatos(void* mensaje){
//	int tamanioPath;
//	memcpy(modoDeArchivo,mensaje,sizeof(char));
//	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
//	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
//	memcpy(offset,mensaje+sizeof(char)+sizeof(int)+tamanioPath,sizeof(int));
//	memcpy(size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int),sizeof(int));
//}

//void recibirMensajeGuardarDatos(void* mensaje){
//	int tamanioPath;
//	int tamanioBuffer;
//	memcpy(modoDeArchivo,mensaje,sizeof(char));
//	memcpy(tamanioPath,mensaje+sizeof(char),sizeof(int));
//	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
//	memcpy(offset,mensaje+sizeof(char)+sizeof(int)+tamanioPath,sizeof(int));
//	memcpy(size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int),sizeof(int));
//	memcpy(tamanioBuffer,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int)+sizeof(int),sizeof(int));
//	memcpy(size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int)+sizeof(int)+sizeof(int),tamanioBuffer);
//}


int recibirEntero(){
  int enteroRecibido;
  if(recv(socketKernel,&enteroRecibido, sizeof(int), 0)==-1){
    log_info(loggerFS, "Error al recibir mensaje desde kernel.");
    exit(-1);
  }
  return enteroRecibido;
}

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
	mostrarMetadata();
	inicializarMmap();
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
	        existeArchivo = validarArchivo(paqueteRecibidoDeKernel.mensaje);
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
//	    	recibirMensajeCrear(paqueteRecibidoDeKernel.mensaje);
			crearArchivo(paqueteRecibidoDeKernel.mensaje);
	        break;
	      case OBTENER_DATOS:
	    	//recibirMensajeObtenerDatos(paqueteRecibidoDeKernel.mensaje);
			obtenerDatosDelArchivo(paqueteRecibidoDeKernel.mensaje);
	        break;
	      case GUARDAR_DATOS:
//	    	recibirMensajeGuardarDatos(paqueteRecibidoDeKernel.mensaje);
			guardarDatosArchivo(paqueteRecibidoDeKernel.mensaje);
	        break;
	      case CORTO_KERNEL:
	        close(socketKernel);
	        break;
	      default:
			log_info(loggerFS, "Error de operacion");
				}

	    }
}
