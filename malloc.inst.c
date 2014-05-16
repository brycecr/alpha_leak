/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: malloc.inst.c,v $
 * Revision 1.1.2.2  1995/04/27  19:47:30  Greg_Lueck
 * 	First revision of productized Atom and tools.
 * 	[1995/04/26  21:20:13  Greg_Lueck]
 *
 * $EndLog$
 */
#pragma ident "@(#)$RCSfile: malloc.inst.c,v $ $Revision: 1.1.2.2 $ (DEC) $Date: 1995/04/27 19:47:30 $"

/*
**  malloc.inst.c -	Instrumentation routines for malloc recording tool.
**
**	This tool records each call to the routine malloc() and prints a
**	summary of the application's allocated memory.
*/

#include <stdio.h>
#include <cmplrs/atom.inst.h>


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
    AddCallProto("AfterMalloc()");
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
    ResolveNamedProc("malloc", &pres);
    if (pres.proc != NULL) {
	AddCallProc(pres.proc, ProcBefore, "BeforeMalloc", REG_ARG_1);
	AddCallProc(pres.proc, ProcAfter, "AfterMalloc");
    }

    /*
     * If this object defines brk(), add an instrumentation point.
     * This allows us to keep track of the end of the application's heap.
     */
    ResolveNamedProc("__brk", &pres);
    if (pres.proc != NULL) {
	AddCallProc(pres.proc, ProcBefore, "BeforeBrk", REG_ARG_1);
    }

    /*
     * If this object defines sbrk(), add an instrumentation point.
     * This allows us to keep track of the end of the application's heap.
     */
    ResolveNamedProc("__sbrk", &pres);
    if (pres.proc != NULL) {
	AddCallProc(pres.proc, ProcBefore, "BeforeSbrk", REG_ARG_1);
    }
}
/****************************************************************************
 *                                                                          *
 *  COPYRIGHT (c) 1995 BY                                                   *
 *  DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASSACHUSETTS.                  *
 *  ALL RIGHTS RESERVED.                                                    *
 *                                                                          *
 *  THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE USED AND COPIED   *
 *  ONLY IN  ACCORDANCE WITH  THE  TERMS  OF  SUCH  LICENSE  AND WITH THE   *
 *  INCLUSION OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE OR ANY  OTHER   *
 *  COPIES THEREOF MAY NOT BE PROVIDED OR OTHERWISE MADE AVAILABLE TO ANY   *
 *  OTHER PERSON.  NO TITLE TO AND OWNERSHIP OF  THE  SOFTWARE IS  HEREBY   *
 *  TRANSFERRED.                                                            *
 *                                                                          *
 *  THE INFORMATION IN THIS SOFTWARE IS  SUBJECT TO CHANGE WITHOUT NOTICE   *
 *  AND  SHOULD  NOT  BE  CONSTRUED AS  A COMMITMENT BY DIGITAL EQUIPMENT   *
 *  CORPORATION.                                                            *
 *                                                                          *
 *  DIGITAL ASSUMES NO RESPONSIBILITY FOR THE USE  OR  RELIABILITY OF ITS   *
 *  SOFTWARE ON EQUIPMENT WHICH IS NOT SUPPLIED BY DIGITAL.                 *
 *                                                                          *
 ****************************************************************************
 *              Copyright 1995 Digital Equipment Corporation
 *                         All Rights Reserved
 *
 * Permission to use, copy, and modify this software and its documentation is
 * hereby granted only under the following terms and conditions.  Both the
 * above copyright notice and this permission notice must appear in all copies
 * of the software, derivative works or modified versions, and any portions
 * thereof, and both notices must appear in supporting documentation.
 *
 * This software may be distributed (but not offered for sale or transferred
 * for compensation) to third parties, provided such third parties agree to
 * abide by the terms and conditions of this notice.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL EQUIPMENT
 * CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
