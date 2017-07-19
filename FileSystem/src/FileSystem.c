#include "funcionesGenericas.h"
#include "socket.h"
#include <commons/bitarray.h>
#include <commons/config.h>
#include <commons/txt.h>
#include <sys/mman.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#define ES_KERNEL 1006
#define CORTO_KERNEL 2
#define GUARDAR_DATOS 403
#define OBTENER_DATOS 404
#define CREAR_ARCHIVO 405
#define BORRAR 406
#define VALIDAR_ARCHIVO 407
#define HANDSHAKE_ACEPTADO 9
#define OPERACION_FINALIZADA_CORRECTAMENTE 410
#define OPERACION_FALLIDA 411
#define EXISTE_ARCHIVO 412
#define NO_EXISTE_ARCHIVO 413
#define DATOS_DE_LECTURA 414
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
char* concatenarPath(char* path, char* carpeta){
	char* cadenaAux=malloc(string_length(punto_Montaje)+string_length(path)+string_length(carpeta));
	string_append(&cadenaAux, punto_Montaje);
	string_append(&cadenaAux,carpeta);
	string_append(&cadenaAux,path);
	return cadenaAux;
}


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

void inicializarBitmap(int file){
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

	bitarray = bitarray_create_with_mode(mmapBitmap,cantidad_bloque/8, MSB_FIRST);
}

void limpiarBitarray(){
	int i=0;
	for(i=0;i<bitarray_get_max_bit(bitarray);i++){
		bitarray_clean_bit(bitarray,i);
	}
}

void inicializarMmap(){
	int file=abrirBitmap();
	if(file<0){
		file=crearBitmap();
		inicializarBitmap(file);
		limpiarBitarray();
	}else{
		inicializarBitmap(file);
	}

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


char* armarNombreArchivo(int bloque){
	char* bloqueAux=malloc(sizeof(int)+string_length(".bin"));
	string_append(&bloqueAux, string_itoa(bloque));
	string_append(&bloqueAux,".bin");
	return bloqueAux;
}



//----------------------------FUNCIONES ARCHIVOS------------------------------------------------

//Validar Archivo
bool validarArchivo(void* mensaje){
	int tamanioPath=0;
	char* pathDeArchivo=string_new();
	memcpy(&tamanioPath,mensaje,sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(int),tamanioPath);
	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo,"/Archivos/");
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

bool seBorroArchivoDeFS(void* mensaje){
	int tamanioPath=0;
	char* pathDeArchivo=string_new();
	memcpy(&tamanioPath,mensaje,sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(int),tamanioPath);
	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo,"/Archivos");
	  t_config* archivo=generarT_Config(cadena);
	  int tamanio = config_get_int_value(archivo,"TAMANIO");
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
	char* cadenaBloque = armarNombreArchivo(nuevoBloque);
	char* path = concatenarPath(cadenaBloque,"/Bloques/");
	struct stat mySt={0};
	if(stat(path,&mySt)==-1){
		mkdir(path,0777);
	}
	int file=creat(path,0777);
	close(file);
}

void asignarBloqueArchivo(int nuevoBloque,char* cadena){
	t_config* datosArchivo=config_create(cadena);
	int tamanioArchivo=config_get_int_value(datosArchivo, "TAMANIO");
	char** bloques=config_get_array_value(datosArchivo,"BLOQUES");
	FILE* archivo=fopen(cadena,"w+");
	char* tamanio="TAMANIO=";
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
	fclose(archivo);
}

int crearArchivo (void* mensaje){
	int tamanioPath=0;
	char* pathDeArchivo=string_new();
	memcpy(&tamanioPath,mensaje,sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(int),tamanioPath);
	int bloque;
	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo,"/Archivos/");
	//CREO DIRECTORIO
	struct stat mySt={0};
	if(stat(cadena,&mySt)==-1){
		mkdir(cadena,0777);
	}

	bloque=obtenerBloque();
	//NO TENDRIA QUE INICIALIZAR EL PATH BLOQUE
	if(bloque!=0){
		asignarBloqueArchivo(bloque,cadena);
		log_info(loggerFS,"Se asigna a %s correctamente el bloque %d",cadena,bloque);
		log_info(loggerFS, "Se creo el archivo %s exitosamente.",cadena);
		return OPERACION_FINALIZADA_CORRECTAMENTE;
	}else{
		log_info(loggerFS,"No se pudo crear el archivo %s por falta de bloques disponibles",cadena);
		return OPERACION_FALLIDA;
	}
}


// OBTENER DATOS
char *leerBloques(int tamanioArchivo, char** bloques, int size, int offset){
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
		return datosDelBloque;

	}else{
		log_info(loggerFS,"Lo pedido para leer supera el tamanio del bloque");
		return NULL;
	}

}

char* obtenerDatosDelArchivo(void* mensaje){
	int tamanioPath=0,size=0,offset=0;
	char* pathDeArchivo=string_new();
	memcpy(&tamanioPath,mensaje+sizeof(char),sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
	memcpy(&offset,mensaje+sizeof(char)+sizeof(int)+tamanioPath,sizeof(int));
	memcpy(&size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int),sizeof(int));

	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo,"/Archivos/");

	t_config* archivo=generarT_Config(cadena);
	int tamanio=config_get_int_value(archivo,"TAMANIO");
	char** bloques= config_get_array_value(archivo,"BLOQUES");
	log_info(loggerFS,"Se encontro el archivo %s y se procede a leer los datos del mismo",cadena);
	char *datosLeidos = leerBloques(tamanio,bloques,size,offset);
	return datosLeidos;
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
	return true;
}

int guardarDatosArchivo(void* mensaje){
	int tamanioPath,tamanioBuffer,size,offset;
	char* pathDeArchivo = string_new();
	char *buffer = string_new();
	memcpy(&tamanioPath,mensaje+sizeof(char),sizeof(int));
	memcpy(pathDeArchivo,mensaje+sizeof(char)+sizeof(int),tamanioPath);
	memcpy(&offset,mensaje+sizeof(char)+sizeof(int)+tamanioPath,sizeof(int));
	memcpy(&size,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int),sizeof(int));
	memcpy(&tamanioBuffer,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int)+sizeof(int),sizeof(int));
	memcpy(buffer,mensaje+sizeof(char)+sizeof(int)+tamanioPath+sizeof(int)+sizeof(int)+sizeof(int),tamanioBuffer);

	char* cadena=malloc(strlen(punto_Montaje)+strlen(pathDeArchivo)+strlen("/Archivos/"));
	cadena=concatenarPath(pathDeArchivo,"/Archivos/");

	t_config* archivo=generarT_Config(cadena);
	int tamanioArchivo=config_get_int_value(archivo,"TAMANIO");
	char** bloques= config_get_array_value(archivo,"BLOQUES");
	log_info(loggerFS,"Se encontro el archivo %s y se procede a guardar los datos %s en el mismo",cadena,buffer);
	if(guardarBloques(bloques,tamanioArchivo,size,offset,buffer,cadena)){
		log_info(loggerFS,"Se guardo correctamente el %s en la cadena",buffer,cadena);
		return OPERACION_FINALIZADA_CORRECTAMENTE;
	}else{
		log_info(loggerFS,"No se pudo guardar correctamente el %s en la cadena",buffer,cadena);
		return OPERACION_FALLIDA;
	}

}
//------------------------------FUNCION RECIBIR--------------------------------------
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
  int quienEs = recvDeNotificacion(socketKernel);
  switch(quienEs){
    case ES_KERNEL:
    	hayKernelConectado = true;
    	sendDeNotificacion(socketKernel, HANDSHAKE_ACEPTADO);
    	break;
    default:
    	log_info(loggerFS, "Se recibio conexion erronea de %d", socketKernel);
    	close(socketKernel);
  }
}

//--------------------------------- MAIN -----------------------------------------

int main(int argc, char *argv[]) {
	loggerFS = log_create("FileSystem.log", "FileSystem", 0, 0);
	//char *path="Debug/fileSystem.config";
	char* datosDeLectura;
	//inicializarFileSystem(path);
	verificarParametrosInicio(argc);
	inicializarFileSystem(argv[1]);
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
		paquete* paqueteRecibidoDeKernel;
	    paqueteRecibidoDeKernel=recvRemasterizado(socketKernel);
	    switch (paqueteRecibidoDeKernel->tipoMsj) {
	      case VALIDAR_ARCHIVO:
	    	  existeArchivo = validarArchivo(paqueteRecibidoDeKernel->mensaje);
	    	  if(existeArchivo){
	    		  log_info(loggerFS, "El archivo %s existe...", (char*)paqueteRecibidoDeKernel->mensaje);
	    		  sendDeNotificacion(socketKernel, EXISTE_ARCHIVO);
	    	  }else{
	    		  log_info(loggerFS, "El archivo %s no existe...", (char*)paqueteRecibidoDeKernel->mensaje);
	    		  sendDeNotificacion(socketKernel, NO_EXISTE_ARCHIVO);
	    	  }
	        break;
	      case BORRAR:
	    	  if(seBorroArchivoDeFS(paqueteRecibidoDeKernel->mensaje)){
	    		  log_info(loggerFS, "Se borro el archivo %s exitosamente.",(char*)paqueteRecibidoDeKernel->mensaje);
	    		  sendDeNotificacion(socketKernel, OPERACION_FINALIZADA_CORRECTAMENTE);
	    	  }else{
	    		  log_info(loggerFS, "No se pudo borrar el archivo %s.", (char*)paqueteRecibidoDeKernel->mensaje);
	    		  sendDeNotificacion(socketKernel, OPERACION_FALLIDA);
	    	  }
	    	  break;
	      case CREAR_ARCHIVO:
	    	  if(crearArchivo(paqueteRecibidoDeKernel->mensaje) == OPERACION_FINALIZADA_CORRECTAMENTE){
	    		  sendDeNotificacion(socketKernel, OPERACION_FINALIZADA_CORRECTAMENTE);
	    	  }else{
	    		  sendDeNotificacion(socketKernel, OPERACION_FALLIDA);
	    	  }
	    	  break;
	      case OBTENER_DATOS:
	    	  datosDeLectura = obtenerDatosDelArchivo(paqueteRecibidoDeKernel->mensaje);
	    	  if(datosDeLectura != NULL){
	    		  sendRemasterizado(socketKernel, DATOS_DE_LECTURA, string_length(datosDeLectura), datosDeLectura);
	    	  }else{
	    		  sendDeNotificacion(socketKernel, OPERACION_FALLIDA);
	    	  }
	    	  free(datosDeLectura);
	    	  break;
	      case GUARDAR_DATOS:
	    	  if(guardarDatosArchivo(paqueteRecibidoDeKernel->mensaje) == OPERACION_FINALIZADA_CORRECTAMENTE){
	    		  sendDeNotificacion(socketKernel, OPERACION_FINALIZADA_CORRECTAMENTE);
	    	  }else{
	    		  sendDeNotificacion(socketKernel, OPERACION_FALLIDA);
	    	  }
	    	  break;
	      case CORTO_KERNEL:
	    	  close(socketEscuchaFS);
	    	  close(socketKernel);
	    	  break;
	      default:
	    	  log_info(loggerFS, "Error de operacion");
	    }
	    free(paqueteRecibidoDeKernel);

	  }
}
