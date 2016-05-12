#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include "mm.h"
#include "memlib.h"

typedef enum { false, true } bool;
	static int counter = 1;
	static int* blks[100];

int main(void)
{
	mem_init();
	mm_init();
	int i = 0;
	
	char command[64];
	int count = 0;
	char fileName[64];
	int num;
	bool exit = false;

	while (exit != true)
	{
		printf(">");
		scanf("%s", &command, 64);

		if (strcmp(command,"allocate") == 0)
		{
			scanf("%*[ \n\t]%d", &num, 64);
			//struct block* blks = malloc(sizeof(struct block));
			
			blks[counter-1] = mm_malloc(num);
			printf("address is %p\n", blks[counter-1]);
			counter++;

		}
		else if (strcmp(command, "free") == 0)
		{
			
		}
		else if (strcmp(command, "writeheap") == 0)
		{	
			
		}
		else if (strcmp(command, "blocklist") == 0)
		{
			print_blocks();
		}
		else if (strcmp(command, "quit") == 0)
		{
			
				exit = true;
		}
		else;
	}
	return 0;
}
