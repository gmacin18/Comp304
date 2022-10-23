#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>



//this function choose random number between len of the input then swap the index randomly
void swap(char string[]){
        int num1 = (rand() % (strlen(string) + 1));
	int num2 = (rand() % (strlen(string) + 1));
	char temp = string[num1];
	string[num1] = string[num2];
	string[num2] =temp;
}

int main(int argc, char *argv[])
{
	//for rand fınction srand is used
	srand(getpid());
	//const variables for shared memory
	const char *name = "OS";
	const int SIZE = 4096;

	//below this line is for creating and confirung the shared memory 
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

	//I could not write the part of reading the memory :(

}
