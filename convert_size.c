/*++
/* NAME
/*	convert_size 3
/* SUMMARY
/*	string to size conversion
/* SYNOPSIS
/*	#include <convert_size.h>
/*
/*	size_t	convert_size(str)
/*	const char *str;
/* DESCRIPTION
/*	convert_size() converts its argument to internal form. if the
/*	argument ends in 'k', 'm' or 'g' the result is multiplied by
/*	1024 (1K), 1048576 (1M), 1073741824 (1G), respectively.
/*	The suffix is case insensitive.
/* SEE ALSO
/*	error(3) error reporting module.
/* DIAGNOSTICS
/*	The result is negative in case of error.
/* LICENSE
/*	This software is distributed under the IBM Public License.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <stdlib.h>
#include <limits.h>

/* Application-specific. */

#include "convert_size.h"

/* convert_size - convert size to number */

size_t  convert_size(const char *str)
{
    char   *last;
    size_t  result;

    result = strtoul(str, &last, 10);
    if (*str == 0 || last == str)
	return (-1);
    if (*last == 0)				/* no suffix */
	return (result);
    if (last[1] != 0)				/* malformed suffix */
	return (-1);
    if (*last == 'k' || *last == 'K')
	return (result * 1024);
    if (*last == 'm' || *last == 'M')
	return (result * 1024 * 1024);
    if (*last == 'g' || *last == 'G')
	return (result * 1024 * 1024 * 1024);
    return (-1);				/* unknown suffix */
}
