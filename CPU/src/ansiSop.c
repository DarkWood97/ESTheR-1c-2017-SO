#include "funcionesGenericas.h"
#include <parser/metadata_program.h>
#include <parser/parser.h>
t_puntero definirVariable(t_nombre_variable nombreVariable)
{
	t_nombre_variable identificador_variable = malloc(t_nombre_variable);
	return *identificador_variable;
}
t_puntero obtenerPosicionVariable(t_nombre_variable identifcador_variable)
{
	return NULL;
}
t_puntero dereferenciar (t_puntero direccion_variable)
{
	return NULL;
}
void asignar (t_puntero direccion_variable, t_valor_variable valor)
{

}
t_valor_variable obtenerValorCompartida (t_nombre_compartida variable)
{
	return NULL;
}
t_valor_variable asignarValorCompartida (t_nombre_compartida variable, t_valor_variable valor)
{
	return NULL;
}
void irAlLabel (t_nombre_etiqueta t_nombre_etiqueta)
{

}
void llamarSinRetorno (t_nombre_etiqueta etiqueta)
{

}
void llamarConRetorno (t_nombre_etiqueta etiqueta, t_puntero donde_retornar)
{

}
void finalizar ()
{

}
void retornar(t_valor_variable retorno)
{

}
