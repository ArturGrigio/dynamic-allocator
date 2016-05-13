#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <assert.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>

#define	MAXLINE	100
#define MAXARGS 100
#define MAX_HEAP ((1<<9))  /* Heap will never grow */
#define WSIZE       4       /* Word and header/footer size (bytes) */
#define DSIZE       8       /* Doubleword size (bytes) */
#define CHUNKSIZE  ((1<<8)+144) /* Extend heap by this amount (bytes) initially */
#define MAX(x, y) ((x) > (y)? (x) : (y))

// PACK GET PUT ... FROM BOOK !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put
#define GET_SIZE(p)  (GET(p) & ~0x3)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp


/* Private global variables */
static char *mem_heap;     /* Points to first byte of heap */
static char *mem_brk;      /* Points to last byte of heap plus 1 */
static char *mem_max_addr; /* Max legal heap addr plus 1*/

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */
extern int place_pol;

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp);
static void checkheap(int verbose);
static void checkblock(void *bp);
int place_pol = 0;

void *allocate(size_t size);
void freeblock(int bnum);
void blocklist(void);
void writeheap(int bnum, char letter, int copies);
void printheap(int bnum, size_t size);
static int alloc_n = 0;
static char *first_block = 0;
static char *alloc_block[MAX_HEAP];

int eval(char *cmdline);
int program_command(char **argv);
int parseline(char *buf, char **argv);

void mem_init(void);
void *mem_sbrk(int incr);

/* $begin mallocinterface */
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
/* $end mallocinterface */

void mm_checkheap(int verbose);
void *mm_realloc(void *ptr, size_t size);

void mem_init(void)
{
	mem_heap = (char *)malloc(MAX_HEAP);
	mem_brk = (char *)mem_heap;
	mem_max_addr = (char *)(mem_heap + MAX_HEAP);
}

void *mem_sbrk(int incr)
{
	char *old_brk = mem_brk;

	if ( (incr < 0) || ((mem_brk + incr) > mem_max_addr)) {
		errno = ENOMEM;
		fprintf(stderr, "ERROR: Ran out of memory...\n");
		return (void *)-1;
	}
	mem_brk += incr;
	return (void *)old_brk;
}

void mem_deinit(void) { free(mem_heap); }
void mem_reset_brk() { mem_brk = (char *)mem_heap; }
void *mem_heap_lo() { return (void *)mem_heap; }
void *mem_heap_hi() { return (void *)(mem_brk - 1); }
size_t mem_heapsize() { return (size_t)((void *)mem_brk - (void *)mem_heap); }
size_t mem_pagesize() { return (size_t)getpagesize(); }

// CODE FROM BOOK FOR mm.c !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int mm_init(void)
{
	/* Create the initial empty heap */
	if ((heap_listp = mem_sbrk(3*WSIZE)) == (void *)-1) //line:vm:mm:begininit
		return -1;
	PUT(heap_listp, PACK(DSIZE, 1)); /* Prologue header */
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
	PUT(heap_listp + (2*WSIZE), PACK(0, 1)); /* Epilogue header */
	heap_listp += (1*WSIZE); //line:vm:mm:endinit

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}

void *mm_malloc(size_t size)
{
	size_t asize;      /* Adjusted block size */
	size_t extendsize; /* Amount to extend heap if no fit */
	char *bp;

	if (heap_listp == 0){
		mm_init();
	}

	/* Ignore spurious requests */
	if (size == 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= WSIZE)                                          //line:vm:mm:sizeadjust1
		asize = 3*WSIZE;                                        //line:vm:mm:sizeadjust2
	else
		if (size % WSIZE != 0)
			asize = (size / WSIZE) * WSIZE + 2*WSIZE + WSIZE;
		else
			asize = (size / WSIZE) * WSIZE + 2*WSIZE;

	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {  //line:vm:mm:findfitcall
		place(bp, asize);                  //line:vm:mm:findfitplace
		return bp;
	}

	/* No fit found. Well... youre fucked */
	printf("Out of Memory\n");
	return NULL;
//	extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
//	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
//		return NULL;                                  //line:vm:mm:growheap2
//	place(bp, asize);                                 //line:vm:mm:growheap3
//	return bp;
}

void mm_free(void *bp)
{
	if(bp == 0)
		return;

	size_t size = GET_SIZE(HDRP(bp));

	if (heap_listp == 0){
		mm_init();
	}

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp);
}

static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) {            /* Case 1 */
		return bp;
	}

	else if (prev_alloc && !next_alloc) {      /* Case 2 */
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
	}

	else if (!prev_alloc && next_alloc) {      /* Case 3 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	else {                                     /* Case 4 */
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
		GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	return bp;
}

void *mm_realloc(void *ptr, size_t size)
{
	size_t oldsize;
	void *newptr;

	/* If size == 0 then this is just free, and we return NULL. */
	if(size == 0) {
		mm_free(ptr);
		return 0;
	}

	/* If oldptr is NULL, then this is just malloc. */
	if(ptr == NULL) {
		return mm_malloc(size);
	}

	newptr = mm_malloc(size);

	/* If realloc() fails the original block is left untouched  */
	if(!newptr) {
		return 0;
	}

	/* Copy the old data. */
	oldsize = GET_SIZE(HDRP(ptr));
	if(size < oldsize) oldsize = size;
	memcpy(newptr, ptr, oldsize);

	/* Free the old block. */
	mm_free(ptr);

	return newptr;
}

void mm_checkheap(int verbose)
{
	// TO DO
}

static void *extend_heap(size_t words)
{
	char *bp;
	size_t size;

	size = words * WSIZE; //line:vm:mm:beginextend
	if ((long)(bp = mem_sbrk(size)) == -1)
		return NULL;                                        //line:vm:mm:endextend

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
	PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr

	/* Coalesce if the previous block was free, 1 TA SAID IT'S OK, REQ DOC SAYS NOTHING ABOUT IT */
	return coalesce(bp);
}

static void place(void *bp, size_t asize)
{
	size_t csize = GET_SIZE(HDRP(bp));

	if ((csize - asize) >= (2*DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
	}
	else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
}

static void *find_fit(size_t asize)
{
	if (place_pol == 0) {
		/* First fit search */
		void *bp;

		for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
			if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
				return bp;
			}
		}
	} else if (place_pol == 1) {
		/* Best fit search */
		int min_size = 0;
		void *bp, *min;

		for (bp = heap_listp, min = NULL; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
			if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))) &&
				(GET_SIZE(HDRP(bp)) < min_size || min_size == 0)) {
				min_size = GET_SIZE(HDRP(bp));
				min = bp;
			}
		}
		return min;
	}
	return NULL; /* No fit */
}

static void printblock(void *bp)
{
	size_t hsize, halloc, fsize, falloc;

	checkheap(0);
	hsize = GET_SIZE(HDRP(bp));
	halloc = GET_ALLOC(HDRP(bp));
	fsize = GET_SIZE(FTRP(bp));
	falloc = GET_ALLOC(FTRP(bp));

	if (hsize == 0) {
		printf("%p: EOL\n", bp);
		return;
	}

	/*  printf("%p: header: [%p:%c] footer: [%p:%c]\n", bp,
	 hsize, (halloc ? 'a' : 'f'),
	 fsize, (falloc ? 'a' : 'f')); */
}

static void checkblock(void *bp)
{
	if ((size_t)bp % 4)
		printf("Error: %p is not word aligned\n", bp);
	if (GET(HDRP(bp)) != GET(FTRP(bp)))
		printf("Error: header does not match footer\n");
}

void checkheap(int verbose)
{
	char *bp = heap_listp;

	if (verbose)
		printf("Heap (%p):\n", heap_listp);

	if ((GET_SIZE(HDRP(heap_listp)) != DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
		printf("Bad prologue header\n");
	checkblock(heap_listp);

	for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
		if (verbose)
			printblock(bp);
		checkblock(bp);
	}

	if (verbose)
		printblock(bp);
	if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
		printf("Bad epilogue header\n");
}


// MAIN LIBRARY !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
int check_command(char input[], char command[]);

int main(){

	mem_init();
	first_block = mm_malloc(1);
	mm_free(first_block);

	char cmdline[MAXLINE]; /* Command line */
	do {
		printf("> ");
		fgets(cmdline, MAXLINE, stdin);
	} while (eval(cmdline));

	mem_deinit();
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
	} else if (strcmp(argv[0], "free") == 0){
		if(argv[1] == NULL)
			printf("Error: no arguments for free\n");
		else {
			int block_number = atoi(argv[1]);
			freeblock(block_number);
		}
		return 1;
	} else if (strcmp(argv[0], "blocklist") == 0){
		blocklist();
		return 1;
	} else if (strcmp(argv[0], "writeheap") == 0){
		if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL)
			printf("Error: wrong arguments for writeheap\n");
		else {
			int block_number = atoi(argv[1]);
			char letter = argv[2][0];
			int copies = atoi(argv[3]);
			writeheap(block_number, letter, copies);
		}
		return 1;
	} else if(strcmp(argv[0], "printheap") == 0){
		if (argv[1] == NULL || argv[2] == NULL)
			printf("Error: wrong arguments for printheap\n");
		else {
			int block_number = atoi(argv[1]);
			size_t size = atoi(argv[2]);
			printheap(block_number, size);
		}
		return 1;
	} else if (strcmp(argv[0], "quit") == 0) { /* quit command */
		return 0;
	} else {
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
	if(bp)
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
			   HDRP(bp), FTRP(bp)+4);
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