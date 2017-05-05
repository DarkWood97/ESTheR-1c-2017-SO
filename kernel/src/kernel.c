#include "funcionesGenericas.h"
#include "socket.h"

//--TYPEDEF------------------------------------------------------
#define  CONEXION_CONSOLA  201
#define CORTO  0
#define ERROR -1
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PAGINA 102
#define MENSAJE_PATH 103
#define MENSAJE_MEMORIA 1001 //Uso el mismo que el CPU?
#define ASIGNAR_PAGINAS 1002 //DEFINIR

bool YA_HAY_UNA_CONSOLA = false;
int pid_actual = 1;

//--TYPEDEF------------------------------------------------------

typedef struct {
	int puerto_Prog;
	int puerto_Cpu;
	_ip ip_FS;
	int puerto_Memoria;
	_ip ip_Memoria;
	int puerto_FS;
	int quantum;
	int quantum_Sleep;
	char *algoritmo;
	int grado_Multiprog;
	unsigned char *sem_Ids;
	unsigned int *sem_Init;
	unsigned char *shared_Vars;
	int stack_Size;
} kernel;

typedef struct{
int PID;
// int PC;
//int paginas_Codigo;
// codigo cod;
// etiquetas etiq;
// Stack SP;
// int EX;
}PCB;
//--------------indicide de codigo-----
// typdef struct{
// int inicio;
// int longitud;
// }codigo;
//--------------indice de etiquetas----
// typdef struct{
// char* funcion;
// int etiqueta;
// }etiquetas;
//---------------indice de stack----
// typdef struct{
// int pos;
// t_list args;
// t_list vars;
// int retPos;
// retVar retVar;
// }stack;
//
// typdef struct{
// int pagina;
// int offset;
// int size;
// }retVar;

typedef struct __attribute__((packed)){
  int tipoMsj;
  int tamMsj;
  char* mensaje;//CONTROLAR VOID O CHAR?
}paquete;

//-----FUNCIONES KERNEL------------------------------------------
kernel crearKernel(t_config *configuracion) {
	kernel kernelAuxiliar;
	verificarParametrosCrear(configuracion,14);
	kernelAuxiliar.puerto_Prog = config_get_int_value(configuracion,"PUERTO_PROG");
	kernelAuxiliar.puerto_Cpu = config_get_int_value(configuracion,"PUERTO_CPU");
	kernelAuxiliar.ip_FS.numero = config_get_string_value(configuracion, "IP_FS");
	kernelAuxiliar.puerto_Memoria = config_get_int_value(configuracion,"PUERTO_MEMORIA");
	kernelAuxiliar.ip_Memoria.numero = config_get_string_value(configuracion,"IP_MEMORIA");
	kernelAuxiliar.puerto_FS = config_get_int_value(configuracion, "PUERTO_FS");
	kernelAuxiliar.quantum = config_get_int_value(configuracion, "QUANTUM");
	kernelAuxiliar.quantum_Sleep = config_get_int_value(configuracion,"QUANTUM_SLEEP");
	kernelAuxiliar.algoritmo = config_get_string_value(configuracion,"ALGORITMO");
	kernelAuxiliar.grado_Multiprog = config_get_int_value(configuracion,"GRADO_MULTIPROG");
	kernelAuxiliar.sem_Ids = config_get_array_value(configuracion, "SEM_IDS");
	kernelAuxiliar.sem_Init = config_get_array_value(configuracion, "SEM_INIT");
	kernelAuxiliar.shared_Vars = config_get_array_value(configuracion,"SHARED_VARS");
	kernelAuxiliar.stack_Size = config_get_int_value(configuracion,"STACK_SIZE");
	return kernelAuxiliar;
}

kernel inicializarKernel(char *path) {
	t_config *configuracionKernel = (t_config*)malloc(sizeof(t_config));
	*configuracionKernel = generarT_ConfigParaCargar(path);
	kernel kernelSistema = crearKernel(configuracionKernel);
	return kernelSistema;
	free(configuracionKernel);
}
void mostrarConfiguracionesKernel(kernel kernel) {
	printf("PUERTO_PROG=%d\n", kernel.puerto_Prog);
	printf("PUERTO_CPU=%d\n", kernel.puerto_Cpu);
	printf("IP_FS=%s\n", kernel.ip_FS.numero);
	printf("PUERTO_FS=%d\n", kernel.puerto_FS);
	printf("IP_MEMORIA=%s\n", kernel.ip_Memoria.numero);
	printf("PUERTO_MEMORIA=%d\n", kernel.puerto_Memoria);
	printf("QUANTUM=%d\n", kernel.quantum);
	printf("QUANTUM_SLEEP=%d\n", kernel.quantum_Sleep);
	printf("ALGORITMO=%s\n", kernel.algoritmo);
	printf("GRADO_MULTIPROG=%d\n", kernel.grado_Multiprog);
	printf("SEM_IDS=");
	imprimirArrayDeChar(kernel.sem_Ids);
	printf("SEM_INIT=");
	imprimirArrayDeInt(kernel.sem_Init);
	printf("SHARED_VARS=");
	imprimirArrayDeChar(kernel.shared_Vars);
	printf("STACK_SIZE=%d\n", kernel.stack_Size);
}

paquete serializar(int tipoMensaje, void* bufferDeData) {
  paquete paqueteAEnviar;
  int tamPaquete =sizeof(paquete);
  //paqueteAEnviar = (paquete) malloc(tamPaquete);
  paqueteAEnviar.tamMsj = tamPaquete;
  paqueteAEnviar.tipoMsj = tipoMensaje;
  paqueteAEnviar.mensaje = bufferDeData;
  return paqueteAEnviar;
  //Hacer free
}




void deserealizar(int socketChequear){
  paquete mensajeAux;
  PCB nuevoPcb;
  paquete consola;
  void* dataRecibida;
  int caso = recv(socketChequear,&mensajeAux.tipoMsj,sizeof(int),0);
  switch (caso) {
    case CORTO:
      printf("El socket %d corto la conexion\n", socketChequear);
      close(socketChequear);
      break;
    case ERROR:
      perror("Error de recv");
      close(socketChequear);
      exit(-1);
      break;
    case MENSAJE_IMPRIMIR: //Imprimir por pantalla
      recv(socketChequear, &mensajeAux.tamMsj,sizeof(int),0);//Verificar si llego bien en todos los recv (hacer una funcion generica para todas)
      dataRecibida = malloc(mensajeAux.tamMsj);
      recv(socketChequear,&dataRecibida,mensajeAux.tamMsj,0);
      printf("Mensaje recibido: %s\n", dataRecibida);
      break;
    case MENSAJE_PATH: //Recibir path de archivo
      recv(socketChequear, &mensajeAux.tamMsj, sizeof(int),0);
      dataRecibida = malloc(mensajeAux.tamMsj);
      recv(socketChequear,&dataRecibida,mensajeAux.tamMsj,0);
      FILE *archivoRecibido = fopen(dataRecibida,"r+w");
//      nuevoPcb =inicializarPCB();
      break;
    case MENSAJE_PAGINA: //Recibo una pagina de memoria
      recv(socketChequear, &mensajeAux.tamMsj,sizeof(int),0);
		  //dataRecibida = malloc(mensajeAux.tamMsj);
		  //Tendria que verificar el tamaño de la pagina con el tamaño del codigo
//      asignarPagina(socketChequear,nuevo.pid, tabla.pagina);
      break;

    case CONEXION_CONSOLA:
      if(YA_HAY_UNA_CONSOLA){
        perror("Ya hay una consola conectada");
        //FD_CLR(socketChequear,&socketsMaster);
        close(socketChequear);
      }else{
        YA_HAY_UNA_CONSOLA = true;
        //FD_SET(socketChequear, &socketsMaster);
        recv(socketChequear,&mensajeAux.tamMsj,sizeof(int),0);
        dataRecibida = malloc(mensajeAux.tamMsj);
        recv(socketChequear,&dataRecibida,mensajeAux.tamMsj,0);
        printf("Handshake con Consola: %s\n", dataRecibida);
      }
      break;
    }
}

void realizarHandshake(int socket, paquete mensaje) {
	int longitud = sizeof(mensaje);
		if (send(socket, &mensaje, longitud, 0) == -1) {
			perror("Error de send");
			close(socket);
			exit(-1);
	}

}


void asignarPagina(int socket, int pid, int contadorPaginas){
	paquete paqMemoria, aux;
	aux = serializar(ASIGNAR_PAGINAS,contadorPaginas);
	memcpy(&paqMemoria,&aux,sizeof(aux));
	//enviarMensaje(socket,paqMemoria);  CREAR FUNCION
}

//int proximoPID(){
//	return pid_Actual++;
//}
//
//PCB inicializarPCB(){
//	PCB pcb.PID=proximoPID();
//	return pcb;
//}




//------FUNCIONES DE ARRAY----------------------------------------

void imprimirArrayDeChar(char* arrayDeChar[]){
	int i = 0;
	printf("[");
	for(; arrayDeChar[i]!=NULL; i++){
		printf("%s ",arrayDeChar[i]);
	}
	puts("]");
}

void imprimirArrayDeInt(int arrayDeInt[]){
	int i = 0;
	printf("[");
	for(; arrayDeInt[i]!=NULL; i++){
		printf("%d ",arrayDeInt[i]);
	}
	puts("]");
}
//-----------------------------------MAIN-----------------------------------------------


//-----------------------------------MAIN-----------------------------------------------


int main(int argc, char *argv[]) {
	verificarParametrosInicio(argc);
	//char *path = "Debug/kernel.config";
	//kernel kernel = inicializarKernel(path);
	kernel kernel = inicializarKernel(argv[1]);
	fd_set socketsCliente, socketsConPeticion, socketsMaster; //socketsMaster son los sockets clientes + sockets servidor
	FD_ZERO(&socketsCliente);
	FD_ZERO(&socketsConPeticion);
	FD_ZERO(&socketsMaster);
	mostrarConfiguracionesKernel(kernel);
	char *buff = malloc(sizeof(char)*16);
	int socketParaMemoria, socketParaFileSystem, socketMaxCliente, socketMaxMaster, socketAChequear, socketAEnviarMensaje, socketQueAcepta, bytesRecibidos;
	int tamMsj = strlen(buff);
	int socketEscucha = ponerseAEscucharClientes(kernel.puerto_Prog, 0);
	socketParaMemoria = conectarAServer(kernel.ip_Memoria.numero, kernel.puerto_Memoria);
	socketParaFileSystem = conectarAServer(kernel.ip_FS.numero, kernel.puerto_FS);
	FD_SET(socketEscucha,&socketsCliente);
	FD_SET(socketParaMemoria,&socketsMaster);
	FD_SET(socketParaFileSystem,&socketsMaster);
	socketMaxCliente = socketEscucha;
	socketMaxMaster = calcularSocketMaximo(socketParaMemoria, socketParaFileSystem);

	while(1){
		socketsConPeticion=socketsCliente;
		if(select(socketMaxCliente+1, &socketsConPeticion, NULL, NULL, NULL) == -1){
			perror("Error de select");
			exit(-1);
		}
		for(socketAChequear = 0; socketAChequear<=socketMaxCliente; socketAChequear++){
			if(FD_ISSET(socketAChequear, &socketsConPeticion)){
				if(socketAChequear == socketEscucha){
					socketQueAcepta = aceptarConexionDeCliente(socketEscucha);
					FD_SET(socketQueAcepta, &socketsMaster);
					FD_SET(socketQueAcepta, &socketsCliente);
					socketMaxCliente = calcularSocketMaximo(socketQueAcepta, socketMaxCliente);
					socketMaxMaster = calcularSocketMaximo(socketQueAcepta, socketMaxMaster);
				}else {
					if((bytesRecibidos = recv(socketAChequear, buff, 16, 0))<=0){
						revisarSiCortoCliente(socketAChequear, bytesRecibidos);
						close(socketAChequear);
						FD_CLR(socketAChequear,&socketsMaster);
						FD_CLR(socketAChequear, &socketsCliente);
					} else {
						for(socketAEnviarMensaje = 0; socketAEnviarMensaje<=socketMaxMaster; socketAEnviarMensaje++){
							if(FD_ISSET(socketAEnviarMensaje,&socketsMaster)){
								chequearErrorDeSend(socketAEnviarMensaje, bytesRecibidos,buff);
							}
						}
						printf("%s\n",buff);
					}
				}
			}
		}
	}
	return EXIT_SUCCESS;
}

