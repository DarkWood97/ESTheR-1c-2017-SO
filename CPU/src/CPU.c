/*
 ============================================================================
 Name        : CPU.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */
#include "socket.h"
#include "primitivas.h"
#include "funcionesCpu.h"

//----------------------VARIABLES GLOBALES-TIPO MENSAJES---------------------------
#define HANDSHAKE_MEMORIA 1001
#define HANDSHAKE_KERNEL 1003
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define MENSAJE_PCB 1015
#define RECIBIR_PCB 1016
#define PROXIMA_INSTRUCCION 1020
#define CONCLUYE_EJECUCION 503
#define ERROR -1
#define CORTO 0
int size_int=sizeof(int);
int size_char=sizeof(char);
//---ANSISOP--------------------------------------------------------------
AnSISOP_funciones primitivas = {
		.AnSISOP_definirVariable		= definirVariable,
		.AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
		.AnSISOP_dereferenciar			= dereferenciar,
		.AnSISOP_asignar				= asignar,
	//	.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
		.AnSISOP_asignarValorCompartida = asignarValorCompartida,
		.AnSISOP_irAlLabel				= irAlLabel,
	//	.AnSISOP_llamarConRetorno		= llamarConRetorno,
	//	.AnSISOP_llamarSinRetorno		= llamarSinRetorno,
		.AnSISOP_retornar				= retornar,
	//	.AnSISOP_finalizar				= finalizar,
};

AnSISOP_kernel primitivas_kernel = {
//		.AnSISOP_wait					=wait_kernel,
//		.AnSISOP_signal					=signal_kernel,
};

//--TYPEDEF----------------------------------------------------------------
typedef struct {
	char *numero;
}ip;
typedef struct {
	ip ipKernel;
	int puertoKernel;
	ip ipMemoria;
	int puertoMemoria;
}cpu;
typedef struct
{
	char * sentencia;
	int tam;
}instruccion;

instruccion instruccionAEjecutar;
//----FUNCIONES CPU--------------------------------------------------------
cpu cpuCrear(t_config *configuracionCPU){
	cpu nuevaCPU;
	nuevaCPU.ipKernel.numero = config_get_string_value(configuracionCPU, "IP_KERNEL");
	nuevaCPU.puertoKernel = config_get_int_value(configuracionCPU, "PUERTO_KERNEL");
	nuevaCPU.ipMemoria.numero = config_get_string_value(configuracionCPU, "IP_MEMORIA");
	nuevaCPU.puertoMemoria = config_get_int_value(configuracionCPU, "PUERTO_MEMORIA");
	return nuevaCPU;
}
cpu inicializarCPU(char *path){
	t_config *configuracionCPU = malloc(sizeof(t_config));
	*configuracionCPU = generarT_ConfigParaCargar(path);
	cpu cpuSistemas;
	cpuSistemas= cpuCrear(configuracionCPU);
	return cpuSistemas;
	free(configuracionCPU);
}

void mostrarConfiguracionCPU(cpu cpuAMostrar){
	printf("PUERTO_KERNEL=%d\n",cpuAMostrar.puertoKernel);
	printf("IP_KERNEL=%s\n",cpuAMostrar.ipKernel.numero);
	printf("PUERTO_MEMORIA=%d\n",cpuAMostrar.puertoMemoria);
	printf("IP_MEMORIA=%s\n",cpuAMostrar.ipMemoria.numero);
}
//------------------------SIGNAL--------------------------------
void manejadorSenial(int signal)
{
	printf("---------------Se recibio una signal-------------");
	switch(signal)
	{
		case SIGUSR1:
			printf("Se recibio la signal: %d", signal);
			printf("---------------------------------");
			printf("--------DESCONECTANDO CPU--------");
			printf("---------------------------------");
			close(socketKernel);
			close(socketMemoria);
			break;
		default:
			printf("No se le presta servicio a la signal recibida");
	}
}

//----------------PCB--------------------------------------------

void deserealizarPCB(paquete mensaje,PCB aux)
{
	char * recibido=malloc(sizeof(paquete));
	recibido=mensaje.mensaje;
	int sizeAuxiliar=sizeof(pcb->PID*size_int*2);
	aux.PID=(int)malloc(sizeAuxiliar);
	memcpy(&aux.PID,recibido,sizeAuxiliar);
	sizeAuxiliar=0;
	sizeAuxiliar=sizeof(pcb->ProgramCounter*size_int*3);
	aux.ProgramCounter=(int)malloc(sizeAuxiliar);
	memcpy(&aux.ProgramCounter,recibido+size_int,size_int);
	sizeAuxiliar=(int)sizeof(pcb->paginas_Codigo*size_int*4);
	aux.paginas_Codigo=(int)malloc(sizeAuxiliar);
	memcpy(&aux.paginas_Codigo,recibido+size_int*2,size_int);
	sizeAuxiliar=sizeof(pcb->cod.comienzo*size_int*5);
	aux.cod.comienzo=(int)malloc(sizeAuxiliar);
	memcpy(&aux.cod.comienzo,recibido+size_int*3,size_int);
	sizeAuxiliar=sizeof(pcb->cod.offset*size_int*6);
	aux.cod.offset=(int)malloc(sizeAuxiliar);
	memcpy(&aux.cod.offset,recibido+size_int*4,size_int);
	sizeAuxiliar=sizeof(pcb->etiquetas+size_int*4);
	aux.etiquetas=(char *)malloc(sizeAuxiliar);
	memcpy(aux.etiquetas,recibido+size_int*5,size_char);
	sizeAuxiliar=size_int*5+size_char*16;
	aux.exitCode=(int)malloc(sizeAuxiliar);
	memcpy(&aux.exitCode,recibido+size_int*4+size_char*16,size_int);
	sizeAuxiliar=size_int*6+size_char*16;
	aux.contextoActual=malloc(sizeof(t_list));
	memcpy(aux.contextoActual,recibido+size_int*5+size_char*16,sizeof(t_list));
	int i=5;
	int b=6;
	sizeAuxiliar=_obtenerSizeActual_(b);
	aux.tamContextoActual=(int)malloc(size_int);
	memcpy(&aux.tamContextoActual,recibido+sizeof(int)*i+sizeof(char)*16+sizeof(t_list)*16,size_int);
	i++;
	sizeAuxiliar=_obtenerSizeActual_(b);
	aux.tamEtiquetas=(int)malloc(size_int);
	memcpy(&aux.tamEtiquetas,recibido+sizeof(int)*i+sizeof(char)*16+sizeof(t_list)*16,size_int);
	sizeAuxiliar=_obtenerSizeActual_(b);
	i++;
	aux.tablaKernel.paginas=(int)malloc(size_int);
	memcpy(&aux.tablaKernel.paginas,recibido+sizeof(int)*i+sizeof(char)*16+sizeof(t_list)*16,size_int);
	sizeAuxiliar=_obtenerSizeActual_(b);
	aux.tablaKernel.pid=(int)malloc(size_int);
	i++;
	memcpy(&aux.tablaKernel.tamaniosPaginas,recibido+sizeof(int)*i+sizeof(char)*16+sizeof(t_list)*16,size_int);
	sizeAuxiliar=_obtenerSizeActual_(b);
	i++;
	aux.tablaKernel.tamaniosPaginas=(int)malloc(size_int);
	memcpy(&aux.tablaKernel.tamaniosPaginas,recibido+sizeof(int)*i+sizeof(char)*16+sizeof(t_list)*16,size_int);

}

/*void deserealizarProxInstruccion (paquete mensaje,PCB aux)
{
	char * recibido=malloc(sizeof(paquete));
	recibido=mensaje.mensaje;
	int sizeAuxiliar=sizeof(codeIndex);
	sizeAuxiliar=sizeof(pcb->cod.comienzo*size_int);
	aux.cod.comienzo=(int)malloc(sizeAuxiliar);
	memcpy(&aux.cod.comienzo,recibido+size_int,size_int);
	sizeAuxiliar=sizeof(pcb->cod.offset*size_int*2);
	aux.cod.offset=(int)malloc(sizeAuxiliar);
	free(recibido);
}*/

//-----------------FUNCIONES MENSAJES--------------------------------------
void * deserealizarMensaje(int socket)
{
	int *tamanio = 0;
	if(recv(socket,tamanio,16,0)==-1)
		{
			perror("Error de receive");
			exit(-1);
		}
	paquete recibido;
	PCB auxiliar;
	recv(socket,&recibido,(int)tamanio,0);
	int caso=recibido.tipoMsj;
	switch(caso)
	{
	case MENSAJE_IMPRIMIR:
		printf("Mensaje recibido: %p",recibido.mensaje);
		break;
	case HANDSHAKE_KERNEL:
		printf("Handshake con kernel \n Mensaje recibido: %p", recibido.mensaje);
		break;
	case HANDSHAKE_MEMORIA:
		printf("Handshake con memoria \n Mensaje recibido:%p", recibido.mensaje);
		break;
	case CORTO:
		printf("El socket %d corto la conexion", socket);
		close(socket);
		break;
	case ERROR:
		perror("Error de recv");
		exit(-1);
		break;
	case MENSAJE_PATH:
		printf("Path recibido: %p ", recibido.mensaje);
		break;
	case RECIBIR_PCB:
		deserealizarPCB(recibido,auxiliar);
		auxiliar.ProgramCounter++;
		break;
	case PROXIMA_INSTRUCCION:
		//deserealizarProxInstruccion(recibido, auxiliar);
		instruccionAEjecutar.sentencia=malloc(recibido.tamMsj);
		instruccionAEjecutar.sentencia=recibido.mensaje;
		instruccionAEjecutar.tam=recibido.tamMsj;
		break;
	default:
		break;
	}
	return NULL;
}


//---FUNCIONES DE CONEXION CON MEMORIA----------------------------------
/*void proximaInstruccion ()
{
	retVar* direccion = malloc(sizeof(char*)*16);
	char* sentencia= loQueTengoQueLeer(direccion->pagina,direccion->offset,direccion->tam);
	char* barraCero="\0";
	memcpy(sentencia+(direccion->tam-1), barraCero, 1);
	analizadorLinea(lineaParaElAnalizador(sentencia), &primitivas, &primitivas_kernel);
	free(direccion);
	free(sentencia);
	pcb->ProgramCounter++;
}*/
void parsear()
{
	retVar * direccion=malloc(sizeof(char*)*16);
	char * sentencia = instruccionAEjecutar.sentencia;
	char * barraCero='\0';
	memcpy(sentencia+(direccion->tam-1), barraCero, 1);
	char * textoLiteral=malloc(instruccionAEjecutar.tam);
	lineaParaElAnalizador(sentencia,textoLiteral);
	analizadorLinea(textoLiteral, &primitivas, &primitivas_kernel);
	free(direccion);
	free(sentencia);
	free(textoLiteral);
}
void pedirProximaInstruccion()
{
	paquete aEnviar;
	codeIndex * auxiliar= &pcb->cod;
	aEnviar=serializar(auxiliar,PROXIMA_INSTRUCCION);
	enviar(socketMemoria,aEnviar);
	deserealizarMensaje(socketMemoria);
}
void recibirSentencia()
{	deserealizarMensaje(socketMemoria);
}
void enviarPCB()
{
	paquete auxiliar;
	PCB * pcbAuxiliar;
	pcbAuxiliar=pcb;
	auxiliar=serializar(pcbAuxiliar,MENSAJE_PCB);
	enviar(socketMemoria,auxiliar);
	pcb->ProgramCounter++;
}
void notificarKernel()
{
	paquete auxiliar;
	auxiliar=serializar(NULL,CONCLUYE_EJECUCION);
	enviar(socketKernel,auxiliar);
}

//----------------------MAIN---------------------------------------------------
int main(int argc, char *argv[])
{
	verificarParametrosInicio(argc);
	cpu cpuDelSistema = inicializarCPU(argv[1]);
	mostrarConfiguracionCPU(cpuDelSistema);
	int socketParaMemoria, socketParaKernel;
	//Me conecto con kernel.
	socketParaKernel = conectarAServer(cpuDelSistema.ipKernel.numero, cpuDelSistema.puertoKernel);
	socketKernel=socketParaKernel;
	socketParaMemoria = conectarAServer(cpuDelSistema.ipMemoria.numero, cpuDelSistema.puertoMemoria);
	socketMemoria=socketParaMemoria; /*para ansisop*/
	deserealizarMensaje(socketParaMemoria);
	//Handshake con kernel
	realizarHandshake(socketKernel,HANDSHAKE_KERNEL);
	/*handshake con memoria */
	realizarHandshake(socketMemoria,HANDSHAKE_MEMORIA);
	//Me quedo a la espera de que kernel me envie la PCB
	while(1)
	{
		deserealizarMensaje(socketParaKernel);
		//Pedir a la memoria proxima sentencia
		pedirProximaInstruccion();
		recibirSentencia();
		parsear();
		enviarPCB();
		notificarKernel();
	/* Aplico la senial para poder recibirla en cualquier momento*/
		signal (SIGUSR1,manejadorSenial);
	}
	return EXIT_SUCCESS;
}
