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
typedef struct __attribute__((__packed__)){
	int tamMsj;
	int tipoMsj;
	void* mensaje;
}paquete;

t_config generarT_ConfigParaCargar(char *path) {
	t_config *configuracionDelComponente = (t_config*)malloc(sizeof(t_config));
	if((configuracionDelComponente = config_create(path)) == NULL){
		perror("Error de ruta de archivo de configuracion");
		exit(-1);
	}
	return *configuracionDelComponente;
	free(configuracionDelComponente);
}

/*void recibirMensajeDeKernel(int socketKernel){
	char *buff = malloc(sizeof(char)*16);
//	int tamanioBuff = strlen(buff)+1;
	if(recv(socketKernel,buff,16,0) == -1){
		perror("Error de receive");
		exit(-1);
	}
	printf("%s\n",buff);
	free(buff);
}*/
void verificarParametrosInicio(int argc)
{
	if(argc!=2){
			perror("Faltan parametros");
			exit(-1);
		}
}
void verificarParametrosCrear(t_config* configuracion, int sizeStruct)
{
	if (dictionary_size(configuracion->properties) != sizeStruct) {
			perror("Faltan parametros para inicializar el fileSystem");
			exit(-1);
		}
}
paquete serializar(void *bufferDeData, int tipoDeMensaje)
{
  paquete paqueteAEnviar;
  int longitud= obtenerLongitudBuff(bufferDeData);
  paqueteAEnviar.mensaje=malloc(sizeof(char)*16);
  paqueteAEnviar.tamMsj = longitud;
  paqueteAEnviar.tipoMsj=tipoDeMensaje;
  paqueteAEnviar.mensaje = bufferDeData;
  return paqueteAEnviar;
}

void realizarHandshake(int socket, paquete mensaje) { //Socket que envia mensaje

	int longitud = sizeof(mensaje); //sino no lee \0
		if (send(socket, &mensaje, longitud, 0) == -1) {
			perror("Error de send");
			close(socket);
			exit(-1);
	}

}
void destruirPaquete(paquete * paquete)
{
	free(paquete->mensaje);
	free(paquete);
}
