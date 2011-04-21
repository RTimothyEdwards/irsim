/* lookup.c --
 *
 * This file contains a single routine used to look up a string in
 * a table, allowing unique abbreviations.
 *
 * Lifted in large part from the source for magic.
 */

#include <stdio.h>
#include <ctype.h>

#include "globals.h"

/*---------------------------------------------------------
 * Lookup --
 *	Searches a table of strings to find one that matches a given
 *	string.  It's useful mostly for command lookup.
 *
 *	Only the portion of a string in the table up to the first
 *	blank character is considered significant for matching.
 *
 * Return value:
 *	If str is the same as or an unambiguous abbreviation for one
 *	of the entries in table, then the index of the matching entry
 *	is returned.  If str is not the same as any entry in the table,
 *	but an abbreviation for more than one entry, then -1 is returned
 *	(ambiguous result).  If str doesn't match any entry, then
 *	-2 is returned.  Case differences are ignored.
 *---------------------------------------------------------
 */

int lookup(char *str, char *(table[]), int quiet)
{
    int match = -2;	/* result, initialized to -2 = no match */
    int pos;
    int ststart = 0;
    char mesg[50];

    static char *namespace = "::irsim::";

    /* Skip over prefix of qualified namespaces "::irsim::" and "irsim::" */
    for (pos = 0; pos < 9; pos++)
	if ((str[pos] != namespace[pos]) || (str[pos] == '\0')) break;
    if (pos == 9) ststart = 9;
    else
    {
	for (pos = 0; pos < 7; pos++)
	    if ((str[pos] != namespace[pos + 2]) || (str[pos] == '\0')) break;
	if (pos == 7) ststart = 7;
    }

    /* search for match */
    for (pos=0; table[pos] != NULL; pos++)
    {
	char *tabc = table[pos];
	char *strc = &(str[ststart]);
	while(*strc!='\0' && *tabc!=' ' &&
	    ((*tabc==*strc) ||
	     (isupper(*tabc) && islower(*strc) && (tolower(*tabc)== *strc))||
	     (islower(*tabc) && isupper(*strc) && (toupper(*tabc)== *strc))))
	{
	    strc++;
	    tabc++;
	}

	if (*strc == '\0') 
	{
	    /* entry matches */
	    if (*tabc == ' ' || *tabc == '\0')
	    {
		/* exact match - record it and terminate search */
		match = pos;
		break;
	    }    
	    else if (match == -2)
	    {
		/* inexact match and no previous match - record this one 
		 * and continue search */
		match = pos;
	    }	
	    else
	    {
		/* previous match, so string is ambiguous unless exact
		 * match exists.  Mark ambiguous for now, and continue
		 * search.
		 */
		match = -1;
	    }
	}
    }

    if (!quiet) {
	if (match == -1) {
	    sprintf(mesg, "Ambiguous option \"%s\"\n", str);
	    lprintf(stderr, mesg);
        }
	else if (match == -2) {
	    lprintf(stderr, "Unknown option.  Valid options are: ");
	    for (pos=0; table[pos] != NULL; pos++) {
		lprintf(stderr, table[pos]);
		lprintf(stderr, " ");
	    }
	    lprintf(stderr, "\n");
	}
    }
    return(match);
}
