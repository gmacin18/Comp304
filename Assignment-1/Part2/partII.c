#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define READ_END 0
#define WRITE_END 1



int main(int argc, char *argv[]){
	//pipe for first child is initilazed
	int parentFirstChild[2];
	int FirstChildParent[2];
	
	if(pipe(parentFirstChild) == -1){
                fprintf(stderr, "Pipe failed.");
                return 1;
        }

        if(pipe(FirstChildParent) == -1){
                fprintf(stderr, "Pipe failed.");
                return 1;
        }

	//pipe for second child is initilazed 
	int parentSecondChild[2];
        int SecondChildParent[2];

        if(pipe(parentSecondChild) == -1){
                fprintf(stderr, "Pipe failed.");
                return 1;
        }

        if(pipe(SecondChildParent) == -1){
                fprintf(stderr, "Pipe failed.");
                return 1;

        }
        
        // pid and the forn operations is created
	pid_t pidList[2];
	//because we have 2 children we should keep the list for these children's pid
	
	pid_t pid;
       	for(int i=0; i<2; i++){
		 pid = fork ();
		if(pid==0){
			pidList[i] = getpid();
		   break;
		}	
	}

	if(pid>0){ // the parent operations is starting
                
		int number=atoi(argv[1]); //the input is taken from command line
	
		close(parentFirstChild[0]); //closing the unneccessary part of the pipe
        	close(FirstChildParent[1]);                          // this number is input of parent pipe for first children
                write(parentFirstChild[1], &number, sizeof(number)); // so we open the write operation the this number
                close(parentFirstChild[1]); //after writing the write operation is closed
	
		int firstoutput; 
                read(FirstChildParent[0], &firstoutput, sizeof(firstoutput));
                close(FirstChildParent[0]); 

	       	close(parentSecondChild[0]); 
                close(SecondChildParent[1]); 
                write(parentSecondChild[1], &number, sizeof(number));
                close(parentSecondChild[1]); 
		
		int secondoutput;
                read(SecondChildParent[0], &secondoutput, sizeof(secondoutput));
                close(SecondChildParent[0]); 

		int finalresult = firstoutput+secondoutput;
	       	printf("Parent: Final result is %d.\n",finalresult);		
		sleep(5);

		int killed = kill(pid, SIGKILL);
                        if (killed==0) {
                                for(int i=0; i<2; i++){
                                printf("Child %d killed \n", pidList[i] );
                                }
                                sleep(9);

                        }
               killed = wait(NULL);
	}

     else if(pid==0){
		    //child operatins are starting
		    if(getpid() == pidList[0]){
			//first child operations
			
	                close(parentFirstChild[1]); 
        	        close(FirstChildParent[0]); 
                
			int parentoutput1;
                	read(parentFirstChild[0], &parentoutput1, sizeof(parentoutput1));
                	close(parentFirstChild[0]);
			sleep(5);


                	int result = getpid() * parentoutput1;
                	printf("First Child: Input %d, Output %d\n", parentoutput1, result);
			sleep(5);

	                write(FirstChildParent[1], &result, sizeof(result));
        	        close(FirstChildParent[1]);    
		    }
		    else if(getpid() == pidList[1]){
                        //second child operations
			close(parentSecondChild[1]); 
                        close(SecondChildParent[0]); 

                        int parentoutput2;
                        read(parentSecondChild[0], &parentoutput2, sizeof(parentoutput2));
                        close(parentSecondChild[0]);
                        
			int result = getpid() / parentoutput2;
                        printf("Second Child: Input %d, Output %d \n", parentoutput2,result);
			sleep(5);
				
                        write(SecondChildParent[1], &result, sizeof(result));
                        close(SecondChildParent[1]);	     	     
	}
     }
	else if (pid <0) printf("Fork failed...");


}
