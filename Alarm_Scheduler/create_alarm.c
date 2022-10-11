#include<stdio.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<unistd.h>
#include <time.h>



int main () {


	int counter = 0;
	struct data{
		int list[10];
	};

	struct data alarms;
	for (int j = 0; j<10; j++){
		alarms.list[j] = 0;
	}
	for (int j = 0; j<10; j++){
		printf("%d : %d \n", j+1, alarms.list[j]);
	}
	printf("\n");
	

	long int current = time(NULL);
	long int alarm = time(NULL) + 2;
	long int nap = alarm-current;

	int pid;
	char *argv[2] = {"ps", NULL};

	int cancel = 1; 
	while (cancel){
		

		pid = fork(); 
		
		if (pid > 0){
			alarms.list[counter] = pid;
			counter++;
			for (int j = 0; j<5; j++){
				printf("%d : %d \n", j+1, alarms.list[j]);
			}
			printf("\n");
			sleep(3);
			if(counter == 3){
				cancel = 0;
			}
		}
		else if (pid == 0){
		    sleep(5);
		 	printf("RING \n");
		 	exit(0);
		}	
	}

	int pid_kill = fork();
	

	if(pid_kill == 0){
		printf("\n %d \n", getpid());
		sleep(nap);
	 	exit(0);
	}	
	pid = fork();

	if(pid == 0){
		alarms.list[7] = getpid();
		printf("\n \n");
		execv("/bin/ps", argv); 
		exit(0);
	}	

	if (pid>0){
		sleep(5);
		kill(pid_kill, SIGCHLD); // SIGCHLD SIGINT
	}
	pid = fork();
	if(pid == 0){
		alarms.list[8] = getpid();
		printf("\n \n");
		sleep(20);
		exit(0);
	}

	if(pid>0){
		sleep(5);
		int status;
		for (int j = 0; j<10; j++){
				waitpid(alarms.list[j],&status,WNOHANG);
				alarms.list[j] = 0;
		}
		execv("/bin/ps", argv); 
	}	
}
