#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "mm.c"
#include "memlib.c"


#define	MAXLINE	100
#define MAXARGS 100

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

int eval(char *cmdline);
int program_command(char **argv);
int parseline(char *buf, char **argv);

int main(){

    mem_init();
    first_block = mm_malloc(1);
    mm_free(first_block);

    char cmdline[MAXLINE]; /* Command line */
    do {
        printf("> ");
        fgets(cmdline, MAXLINE, stdin);
    } while (eval(cmdline));
} // end main


int eval(char *cmdline) {
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;

    /* Argument list*/
    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return 1;   /* Ignore empty lines */
    if (!program_command(argv))
        return 0;

    return 1;
}


int program_command(char **argv) {
    if (strcmp(argv[0], "allocate") == 0){
        if(argv[1] == NULL)
            printf("Error: no arguments for allocate\n");
        else {
            int block_size = atoi(argv[1]);
            alloc_block[alloc_n] = allocate(block_size);
        }
        return 1;
    }
    else if (strcmp(argv[0], "free") == 0){
        if(argv[1] == NULL)
            printf("Error: no arguments for free\n");
        else {
            int block_number = atoi(argv[1]);
            freeblock(block_number);
        }
        return 1;
    }
    else if (strcmp(argv[0], "blocklist") == 0){
        blocklist();
        return 1;
    }
    else if (strcmp(argv[0], "writeheap") == 0){
        if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL)
            printf("Error: wrong arguments for writeheap\n");
        else {
            int block_number = atoi(argv[1]);
            char letter = argv[2][0];
            int copies = atoi(argv[3]);
            writeheap(block_number, letter, copies);
        }
        return 1;
    }
    else if(strcmp(argv[0], "printheap") == 0){
        if (argv[1] == NULL || argv[2] == NULL)
            printf("Error: wrong arguments for printheap\n");
        else {
            int block_number = atoi(argv[1]);
            size_t size = atoi(argv[2]);
            printheap(block_number, size);
        }
        return 1;
    }
    else if (strcmp(argv[0], "quit") == 0) /* quit command */
        return 0;
    else {
        printf("%s: Command not found.\n", argv[0]);
        return 1;
    }

}

/* parseline - Parse the command line and build the argv array */
int parseline(char *buf, char **argv) {
    char *delim;
    int argc;
    int bg;
    buf[strlen(buf)-1] = ' ';
    while (*buf && (*buf == ' ' ))
        buf++;
    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' '))
            buf++;
    }
    argv[argc] = NULL;
    if (argc == 0)
        return 1;
    if ((bg = (*argv[argc-1] == '&')) != 0)
        argv[--argc] = NULL;
    return bg;
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