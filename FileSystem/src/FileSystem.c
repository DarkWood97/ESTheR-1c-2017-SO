/*
 ============================================================================
 Name        : FileSystem.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello World in C, Ansi-style
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/dictionary.h>
#include <commons/config.h>
#include <string.h>

typedef struct
{
	int port;
	char *punto_montaje;
}FileSystem;

bool validarArchivo (char* path)
{
	FILE* f = fopen( path, " r " );
	if(!f)
	{
		return true;
	}
	else
	{
		return false;
	}
}

void crearArchivo(char* path)
{
	bool permisoCreacion = true; //Bitmap?
	if(validarArchivo(path)&&permisoCreacion)
	{
		FILE* f = fopen(path, "w");
	}
}

void borraArchivo(char* path)
{
	remove(path); //Bipmap?
}



int main(void) {
	puts("!!!Hello World!!!"); /* prints !!!Hello World!!! */
	return EXIT_SUCCESS;
}