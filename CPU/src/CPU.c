// AQUI VAN TODOS LOS INCLUDES
#include "funcionesCpu.h"
#include "primitivas.h"
#include "socket.h"

AnSISOP_funciones primitivas = {
	.AnSISOP_definirVariable		= definirVariable,
	.AnSISOP_obtenerPosicionVariable= obtenerPosicionVariable,
	.AnSISOP_dereferenciar			= dereferenciar,
	.AnSISOP_asignar				= asignar,
	.AnSISOP_obtenerValorCompartida = obtenerValorCompartida,
	.AnSISOP_asignarValorCompartida = asignarValorCompartida,
	.AnSISOP_irAlLabel				= irAlLabel,
	.AnSISOP_llamarConRetorno		= llamarConRetorno,
	.AnSISOP_llamarSinRetorno		= llamarSinRetorno,
	.AnSISOP_retornar				= retornar,
	.AnSISOP_finalizar				= finalizar,
};

AnSISOP_kernel primitivasDeKernel = {
	.AnSISOP_wait					=pedirWait,
	.AnSISOP_signal					=pedirSignal,
	.AnSISOP_reservar			=reservarMemoria,
	.AnSISOP_liberar = liberarMemoria,
	.AnSISOP_abrir = abrimeArchivo,
	.AnSISOP_borrar = borrameArchivo,
	.AnSISOP_cerrar = cerrameArchivo,
	.AnSISOP_moverCursor = movemeElCursor,
	.AnSISOP_escribir = escribimeArchivo,
	.AnSISOP_leer = leemeDeArchivo,
};



// 	MIS VARIABLES GLOBALES



//----------FUNCIONES BASICAS CPU--------------//


void cpuCrear(t_config *configuracionCPU){
	IP_KERNEL = config_get_string_value(configuracionCPU, "IP_KERNEL");
	PUERTO_KERNEL = config_get_int_value(configuracionCPU, "PUERTO_KERNEL");
	IP_MEMORIA = config_get_string_value(configuracionCPU, "IP_MEMORIA");
	PUERTO_MEMORIA = config_get_int_value(configuracionCPU, "PUERTO_MEMORIA");
}



//----------------MANEJADOR DE SENIAL---------------------//
void chequeameLaSignal(int senial){
	sigusr1EstaActivo = true;
	log_info(loggerCPU, "Se recibio SIGUSR1, se pasa a finalizar ejecucion de CPU.");
	if(yaMePuedoDesconectar){
		close(socketMemoria);
		close(socketKernel);
		log_info(loggerCPU, "Desconectando CPU...");
		exit(0);
	}
}
//--------HANDSHAKES--------//
void armarDatosDeKernel(paquete *paqueteDeArmado){
	int tamanioDeNombreAlgori = paqueteDeArmado->tamMsj - sizeof(int);
	datosParaEjecucion->algoritmo = string_new();
	memcpy(datosParaEjecucion->algoritmo, paqueteDeArmado->mensaje, tamanioDeNombreAlgori);
	memcpy(&datosParaEjecucion->quantum, paqueteDeArmado->mensaje, sizeof(int));
}

void realizarHandshakeConKernel(){
	sendRemasterizado(socketKernel, HANDSHAKE_KERNEL, 0, 0);
	paquete *paqueteConConfigsDeKernel = recvRemasterizado(socketKernel);
	armarDatosDeKernel(paqueteConConfigsDeKernel);
}



void realizarHandshakeConMemoria(){
	sendRemasterizado(socketMemoria, HANDSHAKE_MEMORIA, 0, 0);
	paquete *paqueteConTamPagina;
	paqueteConTamPagina = recvRemasterizado(socketMemoria);
	tamPaginasMemoria = *(int*)paqueteConTamPagina->mensaje;
	free(paqueteConTamPagina);
	log_info(loggerCPU, "Handshake con memoria realizado.");
}

//-------------RECIBO PCB DE KERNEL----------//
void deserializarPCB(void* pcbSerializada){ //ARREGLAR
}

void recibirPCBDeKernel(){
	paquete *paqueteConPCB;
	paqueteConPCB = recvRemasterizado(socketKernel);
	deserializarPCB(paqueteConPCB->mensaje);
	free(paqueteConPCB);
}



//------------HILO PARA KERNEL-------------//
// void escuchaDeKernel(void* socketKernel){
// 	while(!sigusr1EstaActivo){
// 		paquete paqueteDeKernel;
// 		paqueteDeKernel = recvRemasterizado(*(int*)socketKernel);
// 		switch (paqueteDeKernel.tipoMsj) {
// 			case :
// 		}
// 	}
// }

bool programaSigueEjecutando(){
	return programaEnEjecucionFinalizado&&programaEnEjecucionAbortado&&programaEnEjecucionBloqueado;
}

void preparandoParaEjecutar(){
	programaEnEjecucionAbortado = false;
	programaEnEjecucionBloqueado = false;
	programaEnEjecucionFinalizado = false;
}

void avanzarEnEjecucion(){
	pcbEnProceso->ProgramCounter++;
}

void modificarQuantum(){
	if(string_equals_ignore_case(datosParaEjecucion->algoritmo, "RR")){
		datosParaEjecucion->quantum--;
	}
}

char* pedirLineaAMemoria(){
	int cuantoLeer = pcbEnProceso->cod[pcbEnProceso->ProgramCounter].offset;
	char* lineaDeInstruccion = malloc(cuantoLeer+1);
	void* mensajeParaMemoria = malloc(sizeof(int)*4);
	memcpy(mensajeParaMemoria, &pcbEnProceso->PID, sizeof(int));
	int numeroDePagina = (pcbEnProceso->cod[pcbEnProceso->ProgramCounter].comienzo)/tamPaginasMemoria;
	memcpy(mensajeParaMemoria+sizeof(int), &numeroDePagina, sizeof(int));
	int offset = pcbEnProceso->cod[pcbEnProceso->ProgramCounter].comienzo%tamPaginasMemoria;
	memcpy(mensajeParaMemoria+sizeof(int)*2, &offset, sizeof(int));
	memcpy(mensajeParaMemoria+sizeof(int)*3, &cuantoLeer, sizeof(int));
	sendRemasterizado(socketMemoria, LEER_DATOS, cuantoLeer, mensajeParaMemoria);
	free(mensajeParaMemoria);
	paquete *paqueteConSentencia;
	paqueteConSentencia = recvRemasterizado(socketMemoria);
	if(paqueteConSentencia->tipoMsj==DATOS_DE_PAGINA){
		memcpy(lineaDeInstruccion, paqueteConSentencia->mensaje, cuantoLeer);
		free(paqueteConSentencia);
		string_append(&lineaDeInstruccion, "/n");
		return lineaDeInstruccion;
	}
	else{
		programaEnEjecucionAbortado = true;
		return NULL;
	}
}

//-----------------MAIN------------//
int main(int argc, char *argv[]) {
	signal (SIGUSR1,chequeameLaSignal);
	loggerCPU = log_create("./logCPU.txt", "CPU",0,0);
	//verificarParametrosInicio(argc);
	//inicializarCPU(argv[1]);
	inicializarCPU("Debug/CPU.config");
	log_info(loggerCPU, "CPU inicializada correctamente.");
	//mostrarConfiguracionCPU(cpuDelSistema);
	//Conecto
	socketKernel = conectarAServer(IP_KERNEL, PUERTO_KERNEL);
	socketMemoria = conectarAServer(IP_MEMORIA, PUERTO_MEMORIA);
	// pthread_t hiloParaEscucharKernel;
	// pthread_create(&hiloParaEscucharKernel, NULL, escuchaDeKernel, (void*)&socketKernel); //Puede que el error este aca
	//Handshakes
	realizarHandshakeConKernel(socketKernel,HANDSHAKE_KERNEL);
	realizarHandshakeConMemoria(socketMemoria,HANDSHAKE_MEMORIA);

	//MIENTRAS SIGUSR1 ESTE DESACTIVADO
	while(!sigusr1EstaActivo)
	{
		recibirPCBDeKernel();
		preparandoParaEjecutar();
		while(datosParaEjecucion->quantum != 0 && programaSigueEjecutando()){ //programaSigueEjecutando compara si esta bloqueado finalizado, etc
			char* lineaDeEjecucion;
			lineaDeEjecucion = pedirLineaAMemoria();//Aca adentro voy a tener que ponerle el /0 y chequear si esta abortado
			analizadorLinea(lineaDeEjecucion, &primitivas, &primitivasDeKernel);
			modificarQuantum();
			avanzarEnEjecucion();
		}
//		chequearEstaAbortado();
//		chequearEstaBloqueado();
//		chequearFinalizoQuantum();
	}

  return EXIT_SUCCESS;
}


