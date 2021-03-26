#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>

void handler(int signum);

int main()
{
	FILE *proc;
	proc = fopen("procID.txt", "w+");
	
	if(proc == NULL)
	{
		perror("Error al abrir archivo\n");
		return 0;
	}
	
	pid_t procesoID;
	int i, i_max = 3, *aux;
	int auxID = shmget(IPC_PRIVATE, sizeof(int), 0777|IPC_CREAT);
	aux = (int*) shmat(auxID, 0, 0);
	aux[0] = 0;
	for(i = 0; i < i_max; i++)
	{
		procesoID = fork();
		if(procesoID < 0)
		{
			perror("Error al crear proceso\n");
			exit(EXIT_FAILURE);
		}
		else if(procesoID == 0)
		{
			int id = getpid();
			
			fprintf(proc, "%d\n", id);
			printf("el proceso %d espera una señal\n", id);
			
			int j;
			for(j = 1; j <= 64; j++)
				signal(j, handler);
			
			aux[0]++;
			while(aux[0] != i_max) ;
			fclose(proc);
			pause();
			perror("Problema al recibir la señal\n");
			exit(EXIT_FAILURE);
		}
		else continue;
	}
	while(aux[0] != i) ;
	fclose(proc);
	printf("Se puede iniciar el otro programa\n");
	pid_t wpid;
	int status = 0;
	while((wpid = wait(&status)) > 0);
	printf("El padre termina\n");
	
	return 0;
}

void handler(int signum)
{
	printf("El proceso %d recibió la señal %d\n", getpid(), signum);
	exit(0);
}
