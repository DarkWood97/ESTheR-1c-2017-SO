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

void mostrarConfiguracionCPU(){
	printf("IP_KERNEL = %s\nPUERTO_KERNEL = %d\nIP_MEMORIA = %s\nPUERTO_MEMORIA = %d\n", IP_KERNEL, PUERTO_KERNEL, IP_MEMORIA, PUERTO_MEMORIA);
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
	datosParaEjecucion = malloc(sizeof(datosDeKernel));
	int tamanioDeNombreAlgori = paqueteDeArmado->tamMsj - sizeof(int)*2;
	datosParaEjecucion->algoritmo = string_new();
	char *nombreAlgoritmo = string_substring_until((char*)paqueteDeArmado->mensaje, tamanioDeNombreAlgori);
	string_append(&datosParaEjecucion->algoritmo, nombreAlgoritmo);
	memcpy(&datosParaEjecucion->quantum, paqueteDeArmado->mensaje+ string_length(nombreAlgoritmo), sizeof(int));
	memcpy(&datosParaEjecucion->quantumSleep, paqueteDeArmado->mensaje+string_length(nombreAlgoritmo)+sizeof(int), sizeof(int));
	free(nombreAlgoritmo);
}

void realizarHandshakeConKernel(){
	sendDeNotificacion(socketKernel, HANDSHAKE_KERNEL);
	paquete *paqueteConConfigsDeKernel = recvRemasterizado(socketKernel);
	if(paqueteConConfigsDeKernel->tipoMsj == DATOS_CONFIG_KERNEL){
		armarDatosDeKernel(paqueteConConfigsDeKernel);
	}else{
		log_error(loggerCPU, "Error al recibir los datos de ejecucion de kernel...");
		exit(-1);
	}
	free(paqueteConConfigsDeKernel->mensaje);
	free(paqueteConConfigsDeKernel);
}



void realizarHandshakeConMemoria(){
	sendDeNotificacion(socketMemoria, HANDSHAKE_MEMORIA);
	paquete *paqueteConTamPagina;
	paqueteConTamPagina = recvRemasterizado(socketMemoria);
	if(paqueteConTamPagina->tipoMsj == TAMANIO_PAGINA_MEMORIA){
		tamPaginasMemoria = *(int*)paqueteConTamPagina->mensaje;
		log_info(loggerCPU, "Handshake con memoria realizado.");
	}else{
		log_error(loggerCPU, "Error al recibir el tamanio de pagina de memoria...");
		exit(-1);
	}
	free(paqueteConTamPagina->mensaje);
	free(paqueteConTamPagina);
}

//-------------RECIBO PCB DE KERNEL----------//
void recibirPCBDeKernel(){
	paquete *paqueteConPCB;
	paqueteConPCB = recvRemasterizado(socketKernel);
	pcbEnProceso = deserializarPCB(paqueteConPCB);
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
	return !programaEnEjecucionFinalizado&&!programaEnEjecucionAbortado&&!programaEnEjecucionBloqueado;
}

void preparandoParaEjecutar(){
	programaEnEjecucionAbortado = false;
	programaEnEjecucionBloqueado = false;
	programaEnEjecucionFinalizado = false;
}

void avanzarEnEjecucion(){
	pcbEnProceso->programCounter++;
}

void modificarQuantum(){
	if(string_equals_ignore_case(datosParaEjecucion->algoritmo, "RR")){
		datosParaEjecucion->quantum--;
	}
}

char* pedirLineaAMemoria(){
	int cuantoLeer = pcbEnProceso->indiceCodigo[pcbEnProceso->programCounter].offset;
	char* lineaDeInstruccion = string_new();
	void* mensajeParaMemoria = malloc(sizeof(int)*4);
	memcpy(mensajeParaMemoria, &pcbEnProceso->pid, sizeof(int));
	int numeroDePagina = (pcbEnProceso->indiceCodigo[pcbEnProceso->programCounter].start)/tamPaginasMemoria;
	memcpy(mensajeParaMemoria+sizeof(int), &numeroDePagina, sizeof(int));
	int offset = pcbEnProceso->indiceCodigo[pcbEnProceso->programCounter].start%tamPaginasMemoria;
	memcpy(mensajeParaMemoria+sizeof(int)*2, &offset, sizeof(int));
	memcpy(mensajeParaMemoria+sizeof(int)*3, &cuantoLeer, sizeof(int));
	sendRemasterizado(socketMemoria, LEER_DATOS, cuantoLeer, mensajeParaMemoria);
	free(mensajeParaMemoria);
	paquete *paqueteConSentencia;
	paqueteConSentencia = recvRemasterizado(socketMemoria);
	if(paqueteConSentencia->tipoMsj==DATOS_DE_PAGINA){
		string_append(&lineaDeInstruccion, paqueteConSentencia->mensaje);
		//memcpy(lineaDeInstruccion, paqueteConSentencia->mensaje, cuantoLeer);
		free(paqueteConSentencia);
		string_append(&lineaDeInstruccion, "\0");
		return lineaDeInstruccion;
	}
	else{
		free(paqueteConSentencia);
		programaEnEjecucionAbortado = true;
		return NULL;
	}
}

void limpiarContexto(stack* contextoALimpiar){
	list_destroy_and_destroy_elements(contextoALimpiar->args, free);
	list_destroy_and_destroy_elements(contextoALimpiar->vars, free);
	free(contextoALimpiar);
}

void destruirPCB(){
	log_info(loggerCPU, "Se prosigue a limpiar la PCB para el proximo proceso...");
	list_destroy_and_destroy_elements(pcbEnProceso->indiceStack, (void*)limpiarContexto);
	free(pcbEnProceso->indiceEtiquetas);
	free(pcbEnProceso->indiceCodigo);
	free(pcbEnProceso);
	log_info(loggerCPU, "PCB limpiada exitosamente...");
}

void enviarPCBAKernel(int tipoDeEnvio){
	int tamanioPCB = sacarTamanioPCB(pcbEnProceso);
	void *pcbSerializada = serializarPCB(pcbEnProceso);
	sendRemasterizado(socketKernel, tipoDeEnvio, tamanioPCB, pcbSerializada);
	free(pcbSerializada);
	destruirPCB();
}

void chequearEstaAbortado(){
	if(programaEnEjecucionAbortado){
		log_info(loggerCPU, "Enviando la PCB del proceso %d por haberse abortado, a kernel...", pcbEnProceso->pid);
		enviarPCBAKernel(PCB_ABORTADO);
	}
}

void chequearEstaFinalizado(){
	if(programaEnEjecucionFinalizado){
		log_info(loggerCPU, "Enviando la PCB del proceso %d por haberse finalizado, a kernel...", pcbEnProceso->pid);
		enviarPCBAKernel(PCB_FINALIZADO);
	}
}

void chequearEstaBloqueado(){
	if(programaEnEjecucionBloqueado){
		log_info(loggerCPU, "Enviando la PCB del proceso %d por haberse bloqueado, a kernel...", pcbEnProceso->pid);
		enviarPCBAKernel(PCB_BLOQUEADO);
	}
}

void chequearFinalizoQuantum(){
	if(datosParaEjecucion->quantum == 0){
		log_info(loggerCPU, "Enviando la PCB del proceso %d por terminar su rafaga, a kernel...", pcbEnProceso->pid);
		enviarPCBAKernel(PCB_FINALIZO_RAFAGA);
	}
}

//-----------------MAIN------------//
int main(int argc, char *argv[]) {
	signal (SIGUSR1,chequeameLaSignal);
	loggerCPU = log_create("./logCPU.txt", "CPU",0,0);
	loggerProgramas = log_create("./logProgramas.txt", "Programas",0,0);
	//verificarParametrosInicio(argc);
	//inicializarCPU(argv[1]);
	inicializarCPU("Debug/CPU.config");
	log_info(loggerCPU, "CPU inicializada correctamente.");
	mostrarConfiguracionCPU();
	//mostrarConfiguracionCPU(cpuDelSistema);
	//Conecto
	socketKernel = conectarAServer(IP_KERNEL, PUERTO_KERNEL);
	socketMemoria = conectarAServer(IP_MEMORIA, PUERTO_MEMORIA);
	// pthread_t hiloParaEscucharKernel;
	// pthread_create(&hiloParaEscucharKernel, NULL, escuchaDeKernel, (void*)&socketKernel); //Puede que el error este aca
	//Handshakes
	realizarHandshakeConKernel(socketKernel,HANDSHAKE_KERNEL);
	realizarHandshakeConMemoria(socketMemoria,HANDSHAKE_MEMORIA);
	int quantumAuxiliar = datosParaEjecucion->quantum;
	//MIENTRAS SIGUSR1 ESTE DESACTIVADO
	while(!sigusr1EstaActivo)
	{
		recibirPCBDeKernel();
		preparandoParaEjecutar();
		while(datosParaEjecucion->quantum != 0 && programaSigueEjecutando()){ //programaSigueEjecutando compara si esta bloqueado finalizado, etc
			char* lineaDeEjecucion;
			lineaDeEjecucion = pedirLineaAMemoria();//Aca adentro voy a tener que ponerle el \0 y chequear si esta abortado
			analizadorLinea(lineaDeEjecucion, &primitivas, &primitivasDeKernel);
			modificarQuantum();
			//avanzarEnEjecucion();
			pcbEnProceso->programCounter++;
			pcbEnProceso->rafagas++;
			usleep(datosParaEjecucion->quantumSleep);
		}
		chequearEstaAbortado();
		chequearEstaFinalizado();
		chequearEstaBloqueado();
		chequearFinalizoQuantum();
		datosParaEjecucion->quantum = quantumAuxiliar;
	}

  return EXIT_SUCCESS;
}
