/*----------------------------------------------------------------------*/
/* Tag callback mechanism 						*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <tk.h>
#include <string.h>   /* for strlen() */
#include "net.h"	/* defines TRUE and FALSE */

Tcl_HashTable IrsimTagTable;

/*----------------------------------------------------------------------*/
/* Quick reimplementation of strdup() using Tcl's alloc calls		*/
/*----------------------------------------------------------------------*/

char *Tcl_StrDup(const char *s)
{
   char *snew;
   int slen;

   slen = 1 + strlen(s);
   snew = Tcl_Alloc(slen);
   if (snew != NULL)
      memcpy(snew, s, slen);

   return snew;
}

/*----------------------------------------------------------------------*/
/* Implement tag callbacks on functions					*/
/* Find any tags associated with a command and execute them.		*/
/*----------------------------------------------------------------------*/

int IrsimTagCallback(Tcl_Interp *interp, int argc, char *argv[])
{
    int argidx, result = TCL_OK;
    char *postcmd, *substcmd, *newcmd, *sptr, *sres;
    char *croot = argv[0];
    Tcl_HashEntry *entry;
    Tcl_SavedResult state;
    int reset = FALSE;
    int i, llen, cmdnum;

    /* Skip over namespace qualifier, if any */

    if (!strncmp(croot, "::", 2)) croot += 2;
    if (!strncmp(croot, "irsim::", 10)) croot += 10;

    entry = Tcl_FindHashEntry(&IrsimTagTable, croot);
    postcmd = (entry) ? (char *)Tcl_GetHashValue(entry) : NULL;

    if (postcmd)
    {
	substcmd = (char *)Tcl_Alloc(strlen(postcmd) + 1);
	strcpy(substcmd, postcmd);
	sptr = substcmd;

	/*--------------------------------------------------------------*/
	/* Parse "postcmd" for Tk-substitution escapes			*/
	/* Allowed escapes are:						*/
	/* 	%W	substitute the tk path of the calling window	*/
	/*	%r	substitute the previous Tcl result string	*/
	/*	%R	substitute the previous Tcl result string and	*/
	/*		reset the Tcl result.				*/
	/*	%[0-5]  substitute the argument to the original command	*/
	/*	%N	substitute all arguments as a list		*/
	/*	%%	substitute a single percent character		*/
	/*	%*	(all others) no action: print as-is.		*/
	/*								*/
	/* Characters "[" and "]" in IRSIM commands are escaped to	*/
	/* prevent Tcl from attempting to treat them as an immediate	*/
	/* evaluation.							*/
	/*--------------------------------------------------------------*/

	while ((sptr = strchr(sptr, '%')) != NULL)
	{
	    switch (*(sptr + 1))
	    {
		case 'W': {
		    char *tkpath = NULL;
		    Tk_Window tkwind = Tk_MainWindow(interp);
		    if (tkwind != NULL) tkpath = Tk_PathName(tkwind);
		    if (tkpath == NULL)
			newcmd = (char *)Tcl_Alloc(strlen(substcmd));
		    else
			newcmd = (char *)Tcl_Alloc(strlen(substcmd) + strlen(tkpath));

		    strcpy(newcmd, substcmd);

		    if (tkpath == NULL)
			strcpy(newcmd + (int)(sptr - substcmd), sptr + 2);
		    else
		    {
			strcpy(newcmd + (int)(sptr - substcmd), tkpath);
			strcat(newcmd, sptr + 2);
		    }
		    Tcl_Free(substcmd);
		    substcmd = newcmd;
		    sptr = substcmd;
		    } break;

		case 'R':
		    reset = TRUE;
		case 'r':
		    sres = (char *)Tcl_GetStringResult(interp);
		    newcmd = (char *)Tcl_Alloc(strlen(substcmd)
				+ strlen(sres) + 1);
		    strcpy(newcmd, substcmd);
		    sprintf(newcmd + (int)(sptr - substcmd), "\"%s\"", sres);
		    strcat(newcmd, sptr + 2);
		    Tcl_Free(substcmd);
		    substcmd = newcmd;
		    sptr = substcmd;
		    break;

		case '0': case '1': case '2': case '3': case '4': case '5':
		    argidx = (int)(*(sptr + 1) - '0');
		    if ((argidx >= 0) && (argidx < argc))
		    {
			int needList = 0;
			if (strchr(argv[argidx], '[') != NULL ||
				strchr(argv[argidx], ']') != NULL)
			    needList = 1;
		        newcmd = (char *)Tcl_Alloc(strlen(substcmd)
				+ strlen(argv[argidx]) + 2 * needList);
		        strcpy(newcmd, substcmd);
			if (needList)
			    strcpy(newcmd + (int)(sptr - substcmd), "{");
			strcpy(newcmd + (int)(sptr - substcmd) + needList,
				argv[argidx]);
			if (needList)
			    strcat(newcmd, "}");
			strcat(newcmd, sptr + 2);
			Tcl_Free(substcmd);
			substcmd = newcmd;
			sptr = substcmd;
		    }
		    else if (argidx >= argc)
		    {
		        newcmd = (char *)Tcl_Alloc(strlen(substcmd) + 1);
		        strcpy(newcmd, substcmd);
			strcpy(newcmd + (int)(sptr - substcmd), sptr + 2);
			Tcl_Free(substcmd);
			substcmd = newcmd;
			sptr = substcmd;
		    }
		    else sptr++;
		    break;

		case 'N':
		    llen = 1;
		    for (i = 1; i < argc; i++)
		       llen += (1 + strlen(argv[i]));
		    newcmd = (char *)Tcl_Alloc(strlen(substcmd) + llen);
		    strcpy(newcmd, substcmd);
		    strcpy(newcmd + (int)(sptr - substcmd), "{");
		    for (i = 1; i < argc; i++) {
		       strcat(newcmd, argv[i]);
		       if (i < (argc - 1))
			  strcat(newcmd, " ");
		    }
		    strcat(newcmd, "}");
		    strcat(newcmd, sptr + 2);
		    Tcl_Free(substcmd);
		    substcmd = newcmd;
		    sptr = substcmd;
		    break;

		case '%':
		    newcmd = (char *)Tcl_Alloc(strlen(substcmd) + 1);
		    strcpy(newcmd, substcmd);
		    strcpy(newcmd + (int)(sptr - substcmd), sptr + 1);
		    Tcl_Free(substcmd);
		    substcmd = newcmd;
		    sptr = substcmd;
		    break;

		default:
		    break;
	    }
	}

	/* lprintf(stderr, "Substituted tag callback is \"%s\"\n", substcmd); */

	Tcl_SaveResult(interp, &state);
	result = Tcl_Eval(interp, substcmd);
	if ((result == TCL_OK) && (reset == FALSE))
	    Tcl_RestoreResult(interp, &state);
	else
	    Tcl_DiscardResult(&state);

	Tcl_Free(substcmd);
    }
    return result;
}

/*--------------------------------------------------------------*/
/* Add a command tag callback					*/
/*--------------------------------------------------------------*/

int _irsim_tag(ClientData clientData,
        Tcl_Interp *interp, int objc, Tcl_Obj *CONST objv[])
{
    Tcl_HashEntry *entry;
    char *hstring;
    int new;

    if (objc != 2 && objc != 3)
	return TCL_ERROR;

    entry = Tcl_CreateHashEntry(&IrsimTagTable, Tcl_GetString(objv[1]), &new);
    if (entry == NULL) return TCL_ERROR;

    hstring = (char *)Tcl_GetHashValue(entry);
    if (objc == 2)
    {
	Tcl_SetResult(interp, hstring, NULL);
	return TCL_OK;
    }

    if (strlen(Tcl_GetString(objv[2])) == 0)
    {
	Tcl_DeleteHashEntry(entry);
    }
    else
    {
	hstring = Tcl_StrDup(Tcl_GetString(objv[2]));
	Tcl_SetHashValue(entry, hstring);
    }
    return TCL_OK;
}

/*--------------------------------------------------------------*/
/* Initialize the tag callback stuff.				*/
/*--------------------------------------------------------------*/

void TagInit(interp)
    Tcl_Interp *interp;
{
    Tcl_InitHashTable(&IrsimTagTable, TCL_STRING_KEYS);

    Tcl_CreateObjCommand(interp, "irsim::tag",
		(Tcl_ObjCmdProc *)_irsim_tag,
		(ClientData)NULL, (Tcl_CmdDeleteProc *)NULL);
}
