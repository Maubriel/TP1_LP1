#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <math.h>

void mergeSortTasks(int arrID, int izq, int der, int nivel_actual, int nro_niveles, int niv_count, int niv_count2, int indices[][2]);
void mergeSortNormal(int arrID, int izq, int der);
void merge(int arrID, int izq, int m, int der);
void imprimir(int arrID, int izq, int der);
void imprimir_esq_map(int arrID, int nro_niveles, int tam, int esq_map, int indices[][2]);
void imprimir_nro_proceso(int izq, int der, int indices[][2]);

int main()
{
	/* aún no implemento que reciba datos de la terminal
	   habrá que modificar los datos aquí */
	int array[] = { 5,4,8,9,3,1,4,7,8,9,5,4,8,7,9,6 };
	int procesos = 7;
	
	int tam = sizeof(array)/sizeof(array[0]);
	
	int nro_niveles = (int) (log((double)procesos+1.0)/log(2.0)); /* calculo la cantidad de niveles que va a tener el arbol
									 en función a la cantidad de procesos requeridos */
	
	int *vector, *niv, *niv2, arrID, i, niv_count, niv_count2;
	
	arrID = shmget(IPC_PRIVATE, tam*sizeof(int), 0777|IPC_CREAT); /* se guarda una dir. de memoria que se compartirá entre
									 los procesos para acceder al array de numeros */
									 
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
	
	vector = (int*) shmat(arrID, 0, 0);
	for(i = 0; i < tam; i++)
		vector[i] = array[i]; /* se copia el array de entrada en un vector que se compartirá entre procesos */
	
	printf("Array inicial: ");
	imprimir(arrID, 0, tam-1);
	printf("\n\n");
	
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
	printf("\nArray ordenado: ");
	imprimir(arrID, 0, tam-1);
	printf("\n");
	
	shmctl(arrID, IPC_RMID, 0);
		
	return 0;
}

void mergeSortTasks(int arrID, int izq, int der, int nivel_actual, int nro_niveles, int niv_count, int niv_count2, int indices[][2])
{
	/* aquí se consiguen los arrays auxiliares para la sincronización */
	int *niv, *niv2;
	niv = (int*) shmat(niv_count, 0, 0);
	niv2 = (int*) shmat(niv_count2, 0, 0);
	
	/* la verdad no me acuerdo para que era esto,
	   creo que era para uno de mis intentos de imprimir
	   y creo que es irrelevante para el algoritmo */
	if(der == izq) nivel_actual--;
	
	/* el if de siempre de un merge sort */
	if(izq < der)
	{
		/* si se llega a la cantidad de niveles de procesos maximos
		   empieza a hacer merge sort de forma normal (sin procesos) */
		if(nivel_actual >= nro_niveles)
		{
			//printf("Empieza merge sort normal izq:%d der:%d\n", izq, der);
			mergeSortNormal(arrID, izq, der);
			imprimir_nro_proceso(izq, der, indices);
			printf("Lista ordenada {");
			imprimir(arrID, izq, der);
			printf("}\n");
			shmdt(niv);
			return;
		}
		
		int m = izq + (der - izq) / 2;
		int i;
		
		/* aquí crea un proceso hijo que se encargará
		   de la mitad izquierda del array */
		pid_t hijo1 = fork(), hijo2;
		
		
		
		if(hijo1 == 0)
		{
			/* 
			  Esta parte es IMPORTANTE
			  Cada vez que procesos paralelos lleguen aquí, irán disminuyendo
			  el valor correspondiente al nivel en el que están del vector auxiliar "niv"
			  Cada proceso se quedará estancado en el "while" hasta que se terminen de crear todos
			  nodos requeridos por cada nivel, esto es para
			  que un proceso no se adelante demás en el algoritmo y todos avancen al sgte nivel juntos
			*/
			niv[nivel_actual]--;
			while(niv[nivel_actual] > 0);
			
			/* aquí se avanza al sgte nivel */
			mergeSortTasks(arrID, izq, m, nivel_actual+1, nro_niveles, niv_count, niv_count2, indices);
			
			/* 
			  Otra parte IMPORTANTE
			  Las ordenaciones realizadas en merge() también se deben realizar al mismo tiempo
			  por ello cada proceso se estancará en el while hasta que el valor de niv2 
			  llegue al max de nodos por nivel
			*/
			niv2[nivel_actual]++;
			while(niv2[nivel_actual] < pow(2, nivel_actual));
			
			shmdt(niv);
			
			/* termina el proceso */
			exit(0);
		}
		else if(hijo1 < 0)
		{
			perror("Error al crear hijo izquierdo\n");
			exit(EXIT_FAILURE);
		}
		else
		{
			/* Se crea un segundo proceso que trabajará
			   con la segunda mitad del array recibido */
			hijo2 = fork();
			if(hijo2 == 0)
			{
				/* 
				  Esta parte es IMPORTANTE
				  Ya explicado arriba, pero aquí en el nodo derecho
				*/
				niv[nivel_actual]--;
				while(niv[nivel_actual] > 0);
				
				mergeSortTasks(arrID, m+1, der, nivel_actual+1, nro_niveles, niv_count, niv_count2, indices);
				
				/* 
				  Otra parte IMPORTANTE
				  Ya explicado arriba, pero aquí en el nodo derecho
				*/
				niv2[nivel_actual]++;
				while(niv2[nivel_actual] < pow(2, nivel_actual));
				
				shmdt(niv);
				
				/* termina el proceso */
				exit(0);
			}
			else if(hijo2 < 0)
			{
				perror("Error al crear hijo derecho\n");
				exit(EXIT_FAILURE);
			}
		}
		
		/* aquí el nodo padre se quedará esperando a que
		   sus hijos terminen de ordenar sus partes para luego ordenar ambas */
		int estado;
		waitpid(hijo1, &estado, 0);
		waitpid(hijo2, &estado, 0);
		
		merge(arrID, izq, m, der);
		
		imprimir_nro_proceso(izq, der, indices);
		printf("Lista izquierda {");
		imprimir(arrID, izq, m);
		printf("}, Lista derecha {");
		imprimir(arrID, m+1, der);
		printf("} => {");
		imprimir(arrID, izq, der);
		printf("}\n");
		
	}
	
	shmdt(niv);
}

void mergeSortNormal(int arrID, int izq, int der)
{
	if(izq < der)
	{
		int m = izq + (der - izq) / 2;

		mergeSortNormal(arrID, izq, m);
		mergeSortNormal(arrID, m+1, der);

		merge(arrID, izq, m, der);
	}
}

void merge(int arrID, int izq, int m, int der)
{
	int i, j, k, *arr;
	int n1 = m - izq + 1;
	int n2 = der - m;
	
	arr = (int*) shmat(arrID, 0, 0);

	/* create temp arrays */
	int L[n1], R[n2];

	/* Copy data to temp arrays L[] and R[] */
	for (i = 0; i < n1; i++)
		L[i] = arr[izq + i];
	for (j = 0; j < n2; j++)
		R[j] = arr[m + 1 + j];

	/* Merge the temp arrays back into arr[l..r]*/
	i = 0; // Initial index of first subarray
	j = 0; // Initial index of second subarray
	k = izq; // Initial index of merged subarray
	while (i < n1 && j < n2) {
		if (L[i] <= R[j]) {
			arr[k] = L[i];
			i++;
		}
		else {
			arr[k] = R[j];
			j++;
		}
		k++;
	}

	/* Copy the remaining elements of L[], if there
	are any */
	while (i < n1) {
		arr[k] = L[i];
		i++;
		k++;
	}

	/* Copy the remaining elements of R[], if there
	are any */
	while (j < n2) {
		arr[k] = R[j];
		j++;
		k++;
	}
	
}

void imprimir(int arrID, int izq, int der)
{
	int *arr, i;
	arr = (int*) shmat(arrID, 0, 0);
	
	for(i = izq; i <= der-1; i++)
		printf("%d,", arr[i]);
	printf("%d", arr[der]);
	//printf("\n");
}

void imprimir_esq_map(int arrID, int nro_niveles, int tam, int esq_map, int indices[][2])
{
	int nivel_actual = 0;
	int proceso = 0;
	int nodos, izq=0, der=tam, i, *aux;
	
	if(esq_map == 0)
		printf("=== esquema de arbol ===\n");
	else if(esq_map == 1)
		printf("=== mapeos ===\n");
	else
	{
		printf("Error al imprimir esquema o arbol\n");
		return;
	}
		
	while(nivel_actual < nro_niveles && (der-izq) > 1)
	{
		nodos = pow(2, nivel_actual);
		izq = 0;
		der = tam/nodos;
	
		for(i = 0; i < nodos; i++)
		{
			indices[proceso][0] = izq;
			indices[proceso][1] = der;
			printf("Proceso %d: ", proceso++);
			imprimir(arrID, izq, der);
			if(esq_map == 1)
				printf("\n");
			else
				printf("  ");
			
			int aux = der-izq+1;
			izq += aux;
			der += aux;
		}
		if(esq_map == 0)
			printf("\n");
		
		nivel_actual++;
	}
}

void imprimir_nro_proceso(int izq, int der, int indices[][2])
{
	int i = 0;
	
	while(!(indices[i][0] == izq && indices[i][1] == der))
		i++;
	
	printf("Proceso %d: ", i);
}
