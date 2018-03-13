/*++
/* NAME
/*	error 3h
/* SUMMARY
/*	diagnostics handlers
/* SYNOPSIS
/*	#include <error.h>
/* DESCRIPTION
/* .nf

 /*
  * External interface.
  */
#ifndef PRINTFLIKE
#if __GNUC__ == 2 && __GNUC_MINOR__ >= 7
#define PRINTFLIKE(x,y) __attribute__ ((format (printf, (x), (y))))
#else
#define PRINTFLIKE(x,y)
#endif
#endif
extern void PRINTFLIKE(1, 2) remark(char *,...);
extern void PRINTFLIKE(1, 2) error(char *,...);
extern void PRINTFLIKE(1, 2) panic(char *,...);
extern char *progname;
extern int verbose;

#ifdef MISSING_STRERROR

extern const char *strerror(int);

#endif

/* LICENSE
/* .ad
/* .fi
/*	The IBM Public License must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/
