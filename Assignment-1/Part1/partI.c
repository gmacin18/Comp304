#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

int main ()
{
        pid_t pid;
        pid = fork();
        time_t rawtime;
        struct tm * timeinfo;
        if(pid==0){
                int i=0;
                while(i<1){
                 i++;
                 printf("Child: %d \n", getpid());
                 time ( &rawtime );
                 timeinfo = localtime ( &rawtime );
                 char *time = asctime (timeinfo);
                 printf("Time: %s", time);
                 sleep(1);
         }
        }
         else{
                sleep(5);
                kill(pid, SIGSEGV);
                printf("Child %d is killed... \n",pid);
      }

  return 0;

}

