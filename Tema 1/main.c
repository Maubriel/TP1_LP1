#include "cabecera.h"
#include "cabecera.c"

int main(int arg1, char **arg2)
{
	//int array[] = { 6,5,4};//{5,4,8,9,3,1,4,7,8,9,5,4,8,7,9,6 };
	int procesos = atoi(arg2[1]);
	int tam = hallarTamanio(arg2[2]);
	
	///printf("procesos: %d tamanio: %d\n", procesos, tam);
	
	int nro_niveles = (int) (log((double)procesos+1.0)/log(2.0)); /* calculo la cantidad de niveles que va a tener el arbol
									 en función a la cantidad de procesos requeridos */
	
	int *vector, *niv, *niv2, arrID, i, niv_count, niv_count2;
	
	arrID = shmget(IPC_PRIVATE, tam*sizeof(int), 0777|IPC_CREAT); /* se guarda una dir. de memoria que se compartirá entre
									 los procesos para acceder al array de numeros */
	
	convertirArray(arrID, tam, arg2[2]);
	
	niv_count = shmget(IPC_PRIVATE, nro_niveles*sizeof(int), 0777|IPC_CREAT); /* dir de mem para acceder a un array auxiliar
										     que contará la cantidad de nodos creados en cada nivel,
										     y una vez que el nivel se llene recién se podrá hacer
										     mergeSortTasks() en el sgte nivel */
										     
	niv_count2 = shmget(IPC_PRIVATE, nro_niveles*sizeof(int), 0777|IPC_CREAT); /* lo mismo que el anterior pero para sincronizar las
										      llamadas a la función merge() */
	niv = (int*) shmat(niv_count, 0, 0);
	for(i = 0; i < nro_niveles; i++)
		niv[i] = pow(2, i);	/* aquí cada elemento del vector niv será la cantidad
					   de nodos posibles a colocar en cada nivel, se usará para sincronizar procesos */
		
	niv2 = (int*) shmat(niv_count2, 0, 0);
	for(i = 0; i < nro_niveles; i++)
		niv2[i] = 0;		/* todo empieza en 0, luego irá aumentado
					   hasta llegar al limite de nodos de cada nivel, se usará para sincronizar procesos */
	
	/*vector = (int*) shmat(arrID, 0, 0);
	for(i = 0; i < tam; i++)
		vector[i] = array[i];  se copia el array de entrada en un vector que se compartirá entre procesos */
	
	///printf("Array inicial: ");
	///imprimir(arrID, 0, tam-1);
	///printf("\n\n");
	
	int indices[procesos][2];
	imprimir_esq_map(arrID, nro_niveles, tam-1, 0, indices);
	imprimir_esq_map(arrID, nro_niveles, tam-1, 1, indices);
	
	printf("=== procesamiento ===\n");
	/* Comienza lo importante */
	/* 
	   arrID: ID del array a ordenar
	   0: inicio del array
	   tam-1: fin del array
	   0+1: proceso "actual", se envía 1 porque el proceso 0 es el array entero y a partir del 1 empieza dividirse
	   nro_niveles: niveles totales del arbol de procesos
	   niv_count y niv_count2: IDs de los arrays auxiliares
	*/
	mergeSortTasks(arrID, 0, tam-1, 0+1, nro_niveles, niv_count, niv_count2, indices);
	
	
	/* Para comprobar si se ordenó */
	///printf("\nArray ordenado: ");
	///imprimir(arrID, 0, tam-1);
	///printf("\n");
	
	shmctl(arrID, IPC_RMID, 0);
		
	return 0;
}

