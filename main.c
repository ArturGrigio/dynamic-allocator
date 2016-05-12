#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "mm.c"
#include "memlib.c"


////////////////////////
//    DECLARATIONS    //
////////////////////////
int check_command(char input[], char command[]);

static int alloc_n = 0;
static char *first_block = 0;
static char *alloc_block[MAX_HEAP];

extern int mm_init(void);
extern void *mm_malloc(size_t size);
extern void mm_free(void *ptr);
int place_pol = 0;

void *allocate(size_t size);
void freeblock(int bnum);
void blocklist(void);
void writeheap(int bnum, char letter, int copies);
void printheap(int bnum, size_t size);


////////////////
//    MAIN    //
////////////////
int main(){

    mem_init();
    first_block = mm_malloc(1);
    mm_free(first_block);

    // running boolean
    int exit = 0;

    while(exit == 0)
    {
        printf("> ");
        char command[80];
        fgets(command, 256, stdin);

        if ( check_command(command, "quit") == 1) {
            exit = 1;
        } else {
            char *arguments[80];
            int number_of_args = 1;

            int i;
            for(i = 0; i < strlen(command); i++){
                if(command[i] != ' '){
                    char *file_name;
                    char *rest_of_command;
                    char *temp;

                    file_name = strtok_r(command, " ", &rest_of_command);
                    arguments[0] = file_name;
                    char *token2;

                    // arguments
                    for (temp = command; ; temp = NULL) {
                        token2 = strtok_r(NULL, " ", &rest_of_command);
                        if (token2 == NULL)
                            break;
                        else{
                            arguments[number_of_args] = token2;
                            number_of_args++;
                        }
                    }

                    i = strlen(command);
                } // end !command[i] == ' '
            } // end for loop

            if(check_command(arguments[0], "allocate")){
                int block_size = atoi(arguments[1]); // ASK TA IF WE'LL BE PASSED BAD INPUT
                alloc_block[alloc_n] = allocate(block_size);
            }
            else if(check_command(arguments[0], "free")){
                int block_number = atoi(arguments[1]);
                freeblock(block_number);
            }
            else if(check_command(arguments[0], "blocklist")){
                blocklist();
            }
            else if(check_command(arguments[0], "writeheap")){
                int block_number = atoi(arguments[1]);
                char letter = arguments[2][0];
                int copies = atoi(arguments[3]);

                writeheap(block_number, letter, copies);
            }
            else if(check_command(arguments[0], "printheap")){
                int block_number = atoi(arguments[1]);
                size_t size = atoi(arguments[2]);

                printheap(block_number, size);
            }
        } // end all other statements that are not quit
    }
} // end main

int check_command(char input[], char command[]){
    int return_value = 0;

    int length;
    length = strlen(command);

    char substring[length];

    int i;
    for(i = 0; i < length; i++){
        substring[i] = input[i];
    };
    substring[length] = '\0';


    if( strcmp( substring, command ) == 0){
        return_value = 1;
    }
    return return_value;
}

void *allocate(size_t size) {
    void *bp;
    bp = mm_malloc(size);
    printf("%d\n", ++alloc_n);
    return bp;
}

void freeblock(int bnum) {
    if (bnum <= 0 || bnum > alloc_n) {
        printf("Invalid Block Number\n");
        return;
    }
    mm_free(alloc_block[bnum]);
}

void blocklist(void) {
    void *bp;
    printf("Size  Allocated   Start           End\n");
    for (bp = first_block; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
        printf("%-5d %-11s %-#15x %-#15x\n", GET_SIZE(HDRP(bp)),
               GET_ALLOC(HDRP(bp)) ? "yes" : "no",
               HDRP(bp), FTRP(bp)+3);
}

void writeheap(int bnum, char letter, int copies) {
    if (bnum <= 0 || bnum > alloc_n || !GET_ALLOC(HDRP(alloc_block[bnum]))) {
        printf("Invalid Block Number\n");
        return;
    }
    int i;
    void *p;
    void *bp = alloc_block[bnum];
    for (i = 0, p = bp; (i < copies) && (p != FTRP(bp)); ++i, ++p)
        PUT(p, letter);
}

void printheap(int bnum, size_t size) {
    if (bnum <= 0 || bnum > alloc_n) {
        printf("Invalid Block Number\n");
        return;
    }
    int i;
    void *p;
    void *bp = alloc_block[bnum];
    for (p = bp, i = 0; i < size && GET_SIZE(HDRP(bp)) > 0; ++i){
        if(p == FTRP(bp)){
            bp = NEXT_BLKP(bp);
            p = bp;
        }
        printf("%c", GET(p));
        ++p;
    }

    printf("\n");
}