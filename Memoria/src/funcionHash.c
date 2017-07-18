
#include "funcionHash.h"

bool esElFrameCorrecto(int pid, int numeroPagina, int frameAChequear){
    return (entradasDeTabla[frameAChequear].pid == pid && entradasDeTabla[frameAChequear].pagina == numeroPagina);
}



bool esPaginaLibre(int pid, int numeroPagina, int frameAChequear){
    return entradasDeTabla[frameAChequear].pagina == -1;
}

unsigned int calcularPosicion(int pid, int num_pagina) {
    char str1[20];
    char str2[20];
    sprintf(str1, "%d", pid);
    sprintf(str2, "%d", num_pagina);
    strcat(str1, str2);
    unsigned int indice = atoi(str1) % MARCOS;
    return indice;
}



int buscarFrameProceso(int pid, int numeroPagina, bool(*funcionMagica)(int, int, int)){
    bool noEsLaBuscada = true;
    log_info(loggerMemoria, "Pasando a buscar numero de frame de la pagina %d del proceso %d con la funcion hash...", numeroPagina, pid);
    int paginaDeHash = calcularPosicion(pid, numeroPagina), deDondeEmpiezo;
    log_info(loggerMemoria, "Frame calculado con hash %d...", paginaDeHash);
    log_info(loggerMemoria, "Chequeando que sea el correcto...");
    if(!funcionMagica(pid, numeroPagina, paginaDeHash)){
        log_info(loggerMemoria, "El frame obtenido con la funcion de hashing no es correcto...");
        log_info(loggerMemoria, "Pasando a buscar el frame correcto...");
        deDondeEmpiezo = paginaDeHash;
        while(noEsLaBuscada){
        	paginaDeHash++;
            //YA DI TODA LA VUELTA, SIGNIFICA QUE LA PAGINA NO ESTA
            if(paginaDeHash == deDondeEmpiezo){
                log_info(loggerMemoria, "No se encontro el frame perteneciente a la pagina %d del proceso %d...", numeroPagina, pid);
                noEsLaBuscada = false;
                return -1;
                //ENCONTRE LA PAGINA
            }else if(funcionMagica(pid, numeroPagina, paginaDeHash)){
                log_info(loggerMemoria, "Se encontro el frame %d perteneciente al proceso %d que almacena la pagina %d", paginaDeHash, pid, numeroPagina);
                noEsLaBuscada = false;
                return paginaDeHash;
                //LLEGUE AL FINAL, RECORRO LO QUE QUEDA
            }else if(paginaDeHash == MARCOS-1){
                log_info(loggerMemoria, "Se llego al final de la tabla de paginas invertidas, volviendo a empezar para completar el recorrido...");
                paginaDeHash = 0;
            }
            //paginaDeHash++;
        }
    }else{
        log_info(loggerMemoria, "El frame obtenido con la funcion hash es el correcto...");
    }
    return paginaDeHash;
}
