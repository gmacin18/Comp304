#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define READ_END 0
#define WRITE_END 1



int main(int argc, char *argv[]){
	int parentFirstChild[2];
	int FirstChildParent[2];

        int parentSecondChild[2];
        int SecondChildParent[2];

	if(pipe(parentFirstChild) == -1){
		fprintf(stderr, "Pipe failed.");
		return 1;
	}

        if(pipe(FirstChildParent) == -1){
                fprintf(stderr, "Pipe failed.");
                return 1;       
      
	}

        if(pipe(parentSecondChild) == -1){
                fprintf(stderr, "Pipe failed.");
                return 1;
        }

        if(pipe(SecondChildParent) == -1){
                fprintf(stderr, "Pipe failed.");
                return 1;

        }


	pid_t pidList[2];
	pid_t pid;
       	for(int i=0; i<2; i++){
		 pid = fork ();
		if(pid==0){
			pidList[i] = getpid();
		   break;
		}	
	}

	if(pid>0){
                int number=atoi(argv[1]);
	
		close(parentFirstChild[0]); 
        	close(FirstChildParent[1]); 
                write(parentFirstChild[1], &number, sizeof(number));
                close(parentFirstChild[1]); 
	
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

	       	printf("Parent: Final result is %d.\n",firstoutput+secondoutput);		
		
	}

     else if(pid==0){

		    if(getpid() == pidList[0]){
	                close(parentFirstChild[1]); 
        	        close(FirstChildParent[0]); 

                	int parentoutput1;
                       	read(parentFirstChild[0], &parentoutput1, sizeof(parentoutput1));
                	close(parentFirstChild[0]);



                	int result = getpid() * parentoutput1;
                	printf("First Child: Input %d, Output %d\n", parentoutput1, result);


	                write(FirstChildParent[1], &result, sizeof(result));
        	        close(FirstChildParent[1]);    
		    }
		    else if(getpid() == pidList[1]){
                        close(parentSecondChild[1]); 
                        close(SecondChildParent[0]); 

                        int parentoutput2;
  
                       read(parentSecondChild[0], &parentoutput2, sizeof(parentoutput2));
                       close(parentSecondChild[0]);
 
                        
			int result = getpid() / parentoutput2;
                        printf("Second Child: Input %d, Output %d \n", parentoutput2,  result);
						
                        write(SecondChildParent[1], &result, sizeof(result));
                        close(SecondChildParent[1]);
			
		    } 
	     
	}

	else if (pid <0) printf("Fork failed...");


}
