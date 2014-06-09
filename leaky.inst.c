/*
 * *****************************************************************
 * *                                                               *
 * *   Copyright 2002 Compaq Information Technologies Group, L.P.  *
 * *                                                               *
 * *   The software contained on this media  is  proprietary  to   *
 * *   and  embodies  the  confidential  technology  of  Compaq    *
 * *   Computer Corporation.  Possession, use,  duplication  or    *
 * *   dissemination of the software and media is authorized only  *
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
#pragma ident "@(#)$RCSfile: malloc.inst.c,v $ $Revision: 1.1.4.2 $ (DEC) $Date: 1997/08/18 18:26:35 $"

/*
**  malloc.inst.c -	Instrumentation routines for malloc recording tool.
**
**	This tool records each call to the routine malloc() and prints a
**	summary of the application's allocated memory.
*/

#include <stdio.h>
#include <string.h>
#include <cmplrs/atom.inst.h>

static int check_obj_name(Obj *);
static const char *get_name(Obj *);

/*
**  Perform once-per-application initializations.
*/
void InstrumentInit(int iargc, char **iargv)
{
    unsigned long	BrkStart;
    Obj *        	ObjMain;


    /*
     * Define prototypes for the analysis routines.
     */
    AddCallProto("Initialize(long)");
    AddCallProto("BeforeMalloc(REGV)");
    AddCallProto("AfterMalloc(REGV, long)");
    AddCallProto("BeforeRealloc(REGV, long)");
    AddCallProto("AfterRealloc(REGV, long)");
    AddCallProto("BeforeFree(REGV, long)");
    AddCallProto("BeforeBrk(REGV)");
    AddCallProto("BeforeSbrk(REGV)");
    AddCallProto("PrintResults()");

    /*
     * Calculate the start of this application's heap and pass it to the
     * initialization routine in the analysis code.  The application's
     * heap starts immediately after the main object's uninitialized
     * data area.
     */
    ObjMain = GetFirstObj();
    BrkStart = GetObjInfo(ObjMain, ObjUninitDataStartAddress) +
	GetObjInfo(ObjMain, ObjUninitDataSize);
    AddCallProgram(ProgramBefore, "Initialize", BrkStart);

    /*
     * Print out the results when the application finishes.
     */
    AddCallProgram(ProgramAfter, "PrintResults");
}


/*
 * Instrument each object in this application.
 */
Instrument(int iargc, char **iargv, Obj *obj)
{
    ProcRes	pres;


    /*
     * If this object defines malloc(), add an instrumentation point
     * to trace it.
     */
    /* ResolveNamedProc("malloc", &pres);
    if (pres.proc != NULL) {
	printf("found malloc");
	if (check_obj_name(pres.obj)) {
	    AddCallProc(pres.proc, ProcBefore, "BeforeMalloc", REG_ARG_1);
	    AddCallProc(pres.proc, ProcAfter, "AfterMalloc", REG_0);
	}
    }*/

    /*
     * If this object defines brk(), add an instrumentation point.
     * This allows us to keep track of the end of the application's heap.
     */
    ResolveNamedProc("__brk", &pres);
    if (pres.proc != NULL) {
	if (check_obj_name(pres.obj)) {
	    AddCallProc(pres.proc, ProcBefore, "BeforeBrk", REG_ARG_1);
	}
    }

    /*
     * If this object defines sbrk(), add an instrumentation point.
     * This allows us to keep track of the end of the application's heap.
     */
    ResolveNamedProc("__sbrk", &pres);
    if (pres.proc != NULL) {
	if (check_obj_name(pres.obj)) {
	    AddCallProc(pres.proc, ProcBefore, "BeforeSbrk", REG_ARG_1);
	}
    }
    /*
     * Instrument signal handler
     */
    ResolveNamedProc("SignalReceived", &pres);
    if(pres.proc != NULL) {
	if(check_obj_name(pres.obj)) {
	    AddCallProc(pres.proc, ProcBefore, "PrintResults");
	}
    }
    
    /*
     * Addition to test what is wrong with the malloc
     * instrumentation code.
     *
     */
    Proc *p; Block *b; Inst *i;
    ProcRes target;
    int opcode, function;

    for (p = GetFirstObjProc(obj); p != NULL; p = GetNextProc(p)) {
        for (b = GetFirstBlock(p); b != NULL; b = GetNextBlock(b)) {
            for (i = GetFirstInst(b); i != NULL; i = GetNextInst(i)) {
		/* JSR and BSR */
		opcode = GetInstInfo(i, InstOpcode);
		function = (GetInstInfo(i, InstMemDisp) >> 14) & 0x3;
		if ((opcode == 0x1A && function == 1) || opcode == 0x34) {
		    ResolveTargetProc(i, &target);
		    if (target.name && (!strcmp(target.name ,"malloc") || 
			(!strcmp(target.name, "calloc")) ||
			(!strcmp(target.name, "valloc")))) 
		    {
			AddCallInst(i, InstBefore, "BeforeMalloc", REG_ARG_1);
			AddCallInst(i, InstAfter, "AfterMalloc", REG_0, 
				InstLineNo(i));
		    }	
		    if (target.name && !strcmp(target.name ,"free")) {
			AddCallInst(i, InstBefore, "BeforeFree", REG_ARG_1,
				InstLineNo(i));
                    }
		    if (target.name && !strcmp(target.name, "realloc")) {
			AddCallInst(i, InstBefore, "BeforeRealloc", REG_ARG_1,
				InstLineNo(i));
			AddCallInst(i, InstAfter, "AfterRealloc", REG_0,
				InstLineNo(i));
		    }
		}
		
	    }
	}
    }
}
/*
**  Ensure that the procedure being replaced is not a user written
**  routine, but the expected standard function. This test can only
**  be done for shared libraries.
*/
static int check_obj_name(Obj *	obj)
{
    long	otype;
    int		result=0;

    if (obj) {
	otype =  GetObjInfo(obj, ObjShared);

	/*
	 * If the object is sharable (.so), see if the routine is in 
	 * the library we expect it to be.
	 */
	if (otype == OBJ_SHARABLE) { 
	    if (strcmp(get_name(obj), "libc.so") == 0) {
		result=1;
	    }
	}
	/*
	 * If the object is non-shared, presume that the correct 
	 * library module was linked in.
	 */
	else if (otype == OBJ_NON_SHARED) {
	    result=1;
	}
    }
    return(result);
}

/*
** Get the simple name of an object. Don't use basename(3) because its API 
** requires ability to modify the string, and GetObjName returns a const.
*/
static const char *get_name(Obj *object)
{
    const char *pname = GetObjName(object);
    const char *pslash;

    /*
     * Return pointer to char following last '/', or NULL if no name.
     */
    if (!pname)
	return NULL;
    else
	pslash = strrchr(pname,'/');

    if (pslash)
	pname = ++pslash;

    if (*pname == '\0')
	return NULL;
    else
	return pname;
}
