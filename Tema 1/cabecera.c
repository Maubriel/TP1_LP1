#include "cabecera.h"


void mergeSortTasks(int arrID, int izq, int der, int nivel_actual, int nro_niveles, int niv_count, int niv_count2, int indices[][2])
{
	/* aquí se consiguen los arrays auxiliares para la sincronización */
	int *niv, *niv2;
	niv = (int*) shmat(niv_count, 0, 0);
	niv2 = (int*) shmat(niv_count2, 0, 0);
	
	if(der == izq)
	{
		niv[nivel_actual]-=2;
		niv2[nivel_actual]+=2;
		imprimir_nro_proceso(izq, der, indices);
		printf("Lista ordenada {");
		imprimir(arrID, izq, der);
		printf("}\n");
	}
	
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
		
		imprimir_nro_proceso(izq, der, indices);
		printf("Lista izquierda {");
		imprimir(arrID, izq, m);
		printf("}, Lista derecha {");
		imprimir(arrID, m+1, der);
		
		merge(arrID, izq, m, der);
		
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
	
	while(nivel_actual < nro_niveles && (der-izq) >= 1)
	{
		nodos = pow(2, nivel_actual);
		izq = 0;
		der = tam/nodos;
	
		for(i = 0; i < nodos; i++)
		{
			if(der > tam) der = tam;
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
	
	printf("Proceso (%d): ", i);
}

void convertirArray(int arrID, int tam, char *arg2)
{
	int i = 0, *array;
	char *token = strtok(arg2, ",");
	array = (int*) shmat(arrID, 0, 0);
	
	while(token != NULL && i < tam)
	{
		array[i++] = atoi(token);
		token = strtok(NULL, ",");
	}
}

int hallarTamanio(char *arg2)
{
	int i = 0, numeros = 0;
	while(arg2[i] != '\0')
	{
		while(arg2[i] != ',' && arg2[i] != '\0')
			i++;
		numeros++;
		if(arg2[i] == '\0')
			break;
		i++;
	}
	return numeros;
}
