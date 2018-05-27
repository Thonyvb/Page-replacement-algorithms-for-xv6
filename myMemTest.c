#include "types.h"
#include "stat.h"
#include "user.h"
#include "syscall.h"
#include "mmu.h"


#define PGSIZE 4096
#define COUNT 80

//char* m1[COUNT];

int
main(){
	//MALLOC TEST FOR FIFO COMMENT TO TEST FORK
	printf(1, "---INITIALIZING ALLOCATION TEST ---\n");
	int i=0;
	char c = 'A';
	char* array;
	char achar = 0;
	array = (char *) sbrk(PGSIZE*80);

	for(i=0; i<80*PGSIZE/4; i++){	
		if(!(i % PGSIZE)){
			c += 1;
			printf(1, "SANITY WRITING: 0x%x\n", &array[i]);
		}
		array[i] = achar;
		achar+=1;
	}

	c = 'A';
	achar = 0;
	for (int i = 0; i < PGSIZE*80/4; ++i)
	{
		if(!(i % PGSIZE)){
			c += 1;
			printf(1, "SANITY READING: 0x%x\n",&array[i]);
		}
		if(array[i] != achar){
			printf(1,"ERRROR\n");
		}
		achar+=1;
	}

	exit();
	return 0;

	//FORK TEST uncoment to test and comment the allocation code above 
	// printf(1, "---INITIALIZING FORK TEST ---\n");
	// int q, pid, ppid = 0, waitpid;
	// char* addr = (char*)0;
	// for (q = 0; q < 15; ++q)
	// {
	// 	if ((pid = fork()) == 0){ 
	// 		addr = (char *) sbrk(PGSIZE*80);
	// 		ppid = getpid();
	// 		exit();
	// 	}
	// }
	
	// printf(1, "address 0x%x pid %d\n", addr, ppid );
	// while((waitpid=wait()) >= 0){
	// }

	// printf(1,"FORK OK!\n");
	// exit();
	// return 0;
}