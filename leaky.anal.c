/*
 * *****************************************************************
 * *                                                               *
 * *   Copyright 2002 Compaq Information Technologies Group, L.P.  *
 * *                                                               *
 * *   The software contained on this media  is  proprietary  to   *
 * *   and  embodies  the  confidential  technology  of  Compaq    *
 * *   Computer Corporation.  Possession, use,  duplication  or    *

 * *   pursuant to a valid written license from Compaq Computer    *
 * *   Corporation.                                                *
 * *                                                               *
 * *   RESTRICTED RIGHTS LEGEND   Use, duplication, or disclosure  *
 * *   by the U.S. Government is subject to restrictions  as  set  *
 * *   forth in Subparagraph (c)(1)(ii)  of  DFARS  252.227-7013,  *
 * *   or  in  FAR 52.227-19, as applicable.                       *
 * *                                                               *
 * *****************************************************************
 */
/*
 * HISTORY
 */
#pragma ident "@(#)$RCSfile: malloc.anal.c,v $ $Revision: 1.1.6.1 $ (DEC) $Date: 2000/10/25 18:03:13 $"

/*
**  malloc.anal.c -	Analysis routines for malloc recording tool.
**
**	This tool records each call to the routine malloc() and prints a
**	summary of the application's allocated memory.  See "malloc.inst.c"
**	for a description of how to use this tool.
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <cmplrs/atom.anal.h>
#include <signal.h>


/*
 * We record a histogram of allocation request.  Requests for less than
 * this amount are included in the histogram.  Requests for more than this
 * amount are reported outside of the histogram.
 */
#define MAXSIZE 	16384

/*
 * Allocate an array of structures to record information about various
 * sized allocations.  Allocate an additional structure to hold
 * information about large allocations.
 */
struct {
    long 	calls;
    long 	memoryAllocated;
} mallocHistory[MAXSIZE], mallocOther;


/*
 * Malloc() may call itself recursively.  This keeps track of recursive
 * calls, so we only record allocations made directly by the application.
 */
int 	depth = 0;


/*
 * These keep track of the initial, current, and maximum 'break' value
 * for the application.  The 'break' value is the last address of the
 * application's heap area.  We use these to determine the amount of
 * heap memory actually used by the application.
 */
long 	StartBrk;
long 	CurBrk;
long 	MaxBrk;


/*
 * The handle for the output file.
 */
FILE *	file;

/*
 * We define a struct for recording addresses where
 * leaks occur and the corresponding line numbers
 * in the source file.
 */
typedef struct mallocRec 
{
    void *addr;
    long lineNum;
} mallocRec;

/* An array of mallocRec structs */
mallocRec *mallocd;

/* Number of malloc call sites */ 
int num_mallocd;

/* Maximum size of mallocd array */
int max_mallocd;

/* 
** Comparison function
*/
int compare(const void* a, const void* b) {
    void* vala = ((mallocRec*)a)->addr;
    void* valb = ((mallocRec*)b)->addr;
    if (vala > valb) {
        return -1;
    } else if (vala == valb) {
        return 0;
    } else {
        return 1;
    }
}

/* 
** Inserts a new mallocRec struct into the array
** We build a struct from the parameter values
** We maintain an array as a naive storage datastructure
** which we keep sorted and dynamically increase when
** it reaches a certain size
*/
void insert(void* val, long lineNum) {
    if (!val) {
        fprintf(file, "Malloc returned NULL at line %u\n", lineNum);
        return;
    }
    if (num_mallocd >= max_mallocd -10) {
        mallocd = realloc(mallocd, max_mallocd*2);
        max_mallocd*=2;
    }
    mallocd[num_mallocd].addr = val;
    mallocd[num_mallocd++].lineNum = lineNum;
    qsort(mallocd, num_mallocd, sizeof(mallocRec), compare);
}

/*
** Locates a given address in the array of
** mallocRec's
*/
void* find(void* val) {
    void* result = bsearch(&val, mallocd, num_mallocd, sizeof(mallocRec),
			 compare);
    return result;
}

/*
** Removes a given mallocRec from the array given the
** address it contains
*/
int remove_elem(void* val) {
    assert(val);
    void* result = find(val);
    if (!result) {
        return 0;
    }
    ((mallocRec*)result)->addr = 0;
    qsort(mallocd, num_mallocd--, sizeof(mallocRec), compare);
    return 1;
}
 
void BeforeFree(void* addr, long lineNum) {
    if (!addr) {
        fprintf(file, "Attempt to free NULL at %u\n", lineNum);
        return;
    }
    int result = remove_elem(addr);
    if (!result) {
        fprintf(file, "Found a double free for param %p at line %d\n", 
		addr, lineNum);
    }
}

/*
**  Record a call to malloc().
*/
void BeforeMalloc(long size)
{
   /*
    * Ignore recursive calls.
    */
   if (depth++ != 0)
	return;

   /*
    * Record the allocation request.
    */
   if (size < MAXSIZE) {
	mallocHistory[size].calls++;
	mallocHistory[size].memoryAllocated += size;
   } else {
	mallocOther.calls++;
	mallocOther.memoryAllocated += size;
   }
}


/*
**  Note that we have returned from a malloc() call, so we can keep track
**  of recursive calls.
*/
void AfterMalloc(void* retval, long lineNum)
{
    //fprintf(file, "Return Value %p\n", retval);
    insert(retval, lineNum);
//    fprintf(file, "First elem of mallocd: %p @ line %u\n", mallocd[0].addr, mallocd[0].lineNum);  
    depth--;
}

/*
** Record a call to realloc(). 
**
*/
void BeforeRealloc(void* param, long lineNum)
{
    BeforeFree(param, lineNum);
}

/*
** Record return from realloc
*/
void AfterRealloc(void *retval, long lineNum)
{
    AfterMalloc(retval, lineNum);
}

/*
**  Record a call to brk().  This call changes the application's
**  'break' value.
*/
void BeforeBrk(long newbrk)
{
    CurBrk = newbrk;
    if (CurBrk > MaxBrk)
	MaxBrk = CurBrk;
}


/*
**  Record a call to sbrk().  This call increases the application's
**  'break' value.
*/
void BeforeSbrk(long size)
{
    CurBrk += size;
    if (CurBrk > MaxBrk)
	MaxBrk = CurBrk;
}


/* Forward declaration */
void PrintResults();


/*
**  Actual signal handler function
*/
void handle(int signum, siginfo_t* siginfo, void* context) {
    PrintResults();
    printf("\n");
    printf("ATOM detected forced program termination.\n");
    printf("Signal: %s\n", __sys_siglist[signum]);
    exit(42);
}

/*
**  Create the output file and set the initial address for the
**  application's 'break' value.
*/
void Initialize(long startbrk)
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = handle;
    sigaction(SIGSEGV, &act, 0);
    sigaction(SIGABRT, &act, 0);
    sigaction(SIGTERM, &act, 0);
    sigaction(SIGILL, &act, 0);
    sigaction(SIGFPE, &act, 0);
    sigaction(SIGBUS, &act, 0);

    file = fopen("leaky_output.txt", "w");
    if (file == NULL) {
	fprintf(stderr, "malloc: Unable to create file 'malloc.out'.\n");
	exit(1);
    }
    fprintf(file, "LEAKY: THE LEAK CHECKER FOR ATOM\n");
    fprintf(file, "/***********************************/\n");
    fprintf(file, "        Leak Detection Log\n");
    fprintf(file, "/***********************************/\n");

    StartBrk = startbrk;
    MaxBrk = startbrk;
    CurBrk = startbrk;

    max_mallocd = 1024;
    mallocd = calloc(max_mallocd, sizeof(mallocRec));
    num_mallocd = 0;
}


/*
**  Print the summary information about this application's malloc() calls.
*/
void PrintResults()
{
    long 	calls;
    long	allocated;
    int 	i;


/*
    fprintf(file, "%8s %15s %20s\n", "Size", "calls", "allocated");

    calls = 0;
    allocated = 0;
    for (i = 0; i < MAXSIZE; i++) {
	if (mallocHistory[i].calls > 0) {
	    fprintf(file, "%8ld %15ld %20ld\n", i,
		mallocHistory[i].calls, mallocHistory[i].memoryAllocated);
	    calls += mallocHistory[i].calls;
	    allocated += mallocHistory[i].memoryAllocated;
	}
    }

    if (mallocOther.calls > 0) {
	fprintf(file, ">=%7ld %15ld %20ld\n", MAXSIZE,
	    mallocOther.calls, mallocOther.memoryAllocated);
	calls += mallocOther.calls;
	allocated += mallocOther.memoryAllocated;
    }
*/
    if (num_mallocd == 0) {
        fprintf(file, "No leaked memory found!\n");
    } else {

	    int j;
	    for (j=0; j<num_mallocd; ++j) {
			    fprintf(file, "Found leaked memory at location %p, line %u\n", 
			mallocd[j].addr, mallocd[j].lineNum);
    }
    }

 //   fprintf(file, "\n%8s %15ld %20ld\n", "Totals", calls, allocated);
 //   fprintf(file, "\nMaximum Memory Allocated: %ld bytes\n", MaxBrk - StartBrk);
    fclose(file);
}
