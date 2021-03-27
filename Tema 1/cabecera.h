#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <math.h>
#include <string.h>

void mergeSortTasks(int arrID, int izq, int der, int nivel_actual, int nro_niveles, int niv_count, int niv_count2, int indices[][2]);
void mergeSortNormal(int arrID, int izq, int der);
void merge(int arrID, int izq, int m, int der);
void imprimir(int arrID, int izq, int der);
void imprimir_esq_map(int arrID, int nro_niveles, int tam, int esq_map, int indices[][2]);
void imprimir_nro_proceso(int izq, int der, int indices[][2]);
int hallarTamanio(char *arg2);
void convertirArray(int arrID, int tam, char *arg2);
