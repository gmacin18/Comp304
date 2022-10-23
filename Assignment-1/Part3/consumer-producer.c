#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

void swap(char string[]){
        int num1 = (rand() % (strlen(string) - 0  + 1));
	int num2 = (rand() % (strlen(string) - 0  + 1));
	char temp = string[num1];
	string[num1] = string[num2];
	string[num2] =temp;
}

int main(int argc, char *argv[])
{
	srand(getpid());
	const char *name = "OS";
	const int SIZE = 4096;

	int shm_fd;
	void *ptr;
	
	char *message = argv[1];
	shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

        /* configure the size of the shared memory segment */
        ftruncate(shm_fd,SIZE);

        /* now map the shared memory segment in the address space of the process */
        ptr = mmap(0,SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (ptr == MAP_FAILED) {
                printf("Map failed\n");
                return -1;

        }
}
