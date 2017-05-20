# tp-2017-1c-Caia

Codigos.
--CPU
#define HANDSHAKE_MEMORIA 1001
#define HANDSHAKE_KERNEL 1003
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define MENSAJE_PCB 1015
#define ERROR -1
#define CORTO 0
---CONSOLA
#define MENSAJE_IMPRIMIR 101
#define MENSAJE_PATH  103
#define MENSAJE_PID   105
#define HANDSHAKE_CONSOLA 1005
#define ESPACIO_INSUFICIENTE -2
#define INICIAR 1
#define FINALIZAR 503
#define DESCONECTAR 3
#define LIMPIAR 4
--MEMORIA
#define FALTA_RECURSOS_MEMORIA -50
#define ESPACIO_INSUFICIENTE -2
#define ES_CPU 1001
#define HANG_UP_CPU -1001
#define ES_KERNEL 1002
#define INICIALIZAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define LEER_DATOS 504
#define ESCRIBIR_DATOS 505
#define INICIO_EXITOSO 50
#define SE_PUDO_COPIAR 2001
#define NO_SE_PUEDE_COPIAR -2001
#define DATOS_DE_PAGINA 103
#define TAMANIO_PAGINA_PARA_KERNEL 102
-- KERNEL
#define CONEXION_CONSOLA 1005
#define CORTO 0
#define ERROR -1
#define ESPACIO_INSUFICIENTE -2
#define MENSAJE_IMPRIMIR 101
#define TAMANIO_PAGINA 102
#define MENSAJE_PATH 103
#define HANDSHAKE_MEMORIA 1002
#define HANDSHAKE_CPU 1003
#define INICIAR_PROGRAMA 501
#define ASIGNAR_PAGINAS 502
#define FINALIZAR_PROGRAMA 503
#define INICIO_EXITOSO 50
#define MENSAJE_PCB 1015
#define backlog 10 //cantidad de conexiones en la cola
-------ESTRUCTURAS
>>PCB
typedef struct __attribute__((packed)) {
 int PID;
 int PC;
 int paginas_Codigo;
 codeIndex cod;
 char* etiquetas;
 int exitCode;
 t_list *contextoActual;
 int tamContextoActual;
 int tamEtiquetas;
 TablaKernel tablaKernel;
} PCB;
>>STACK
typedef struct __attribute__((packed)) {
 int pagina;
 int offset;
 int size;
} retVar;

typedef struct __attribute__((packed)) {
 int pos;
 t_list* args;
 t_list* vars;
 int retPos;
 retVar retVar;
 int tamArgs;
 int tamVars;
} Stack;
typedef struct __attribute__((packed)) {
 int comienzo;
 int offset;
} codeIndex;
typedef struct __attribute__((packed)) {
 int pid;
 int tamaniosPaginas;
 int paginas;
} TablaKernel;
