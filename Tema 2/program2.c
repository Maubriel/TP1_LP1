#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <signal.h>

int main()
{
	FILE *proc;
	proc = fopen("procID.txt", "r");
	pid_t hijo;
	while(1)
	{
		int proceso, señal, seg;
		fscanf(proc, "%d", &proceso);
		fscanf(proc, "%d", &señal);
		fscanf(proc, "%d", &seg);
		
		if(feof(proc)) break;
		
		hijo = fork();
		if(hijo < 0)
		{
			perror("Error al crear proceso\n");
			exit(EXIT_FAILURE);
		}
		else if(hijo == 0)
		{
			sleep(seg);
			kill(proceso, señal);
			exit(0);
		}
		else continue;
	}
	
	fclose(proc);
	
	pid_t wpid;
	int status = 0;
	while((wpid = wait(&status)) > 0);
	
	return 0;
}
