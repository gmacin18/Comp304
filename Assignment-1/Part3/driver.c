#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>

int main(int argc, char *argv[])
{	
	//taking inputs from command line
	int N = atoi(argv[2]);
	char stringmessage[50];
	strcpy(stringmessage,argv[3]);
	if (N<3){
		printf("The number of process should be bigger than 2...\n");
		exit(-1);
	}
        
	pid_t pid;
	//pid and fork operations         
	pid = fork();
        if (pid == 0) { 
		//parent process
         	printf("Parent: Playing Chinese whisper with %d processes \n", N);
	       	exit(0);
	}
	else if (pid > 0){
        wait(NULL);
       	for(int i=1; i<N; i++){
            pid = fork();
            if (pid == 0) {
		    //child operations should be her but i could not completed child part......
            	break;
	    } 
        }
        }
		
	    								
	printf("Parent terminating.... \n");


}
