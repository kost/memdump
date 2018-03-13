/*++
/* NAME
/*	memdump 1
/* SUMMARY
/*	memory dumper
/* SYNOPSIS
/* .ad
/* .fi
/*	\fBmemdump\fR [\fB-kv\fR] [\fB-b \fIbuffer_size\fR]
/*	[\fB-d \fIdump_size\fR] [\fB-m \fImap_file\fR] [\fB-p \fIpage_size\fR]
/* DESCRIPTION
/*	This program dumps system memory to the standard output stream, 
/*	skipping over holes in memory maps.
/*	By default, the program dumps the contents of physical memory
/*	(\fB/dev/mem\fR).
/*
/*	Output is in the form of a raw dump; if necessary, use the \fB-m\fR
/*	option to capture memory layout information.
/*
/*	Output should be sent off-host over the network, to avoid changing
/*	all the memory in the file system cache. Use netcat, stunnel, or
/*	openssl, depending on your requirements.
/*
/*	The size arguments below understand the \fBk\fR (kilo) \fBm\fR (mega)
/*	and \fBg\fR (giga) suffixes. Suffixes are case insensitive.
/*
/*	Options
/* .IP \fB-k\fR
/*	Attempt to dump kernel memory (\fB/dev/kmem\fR) rather than physical
/*	memory.
/* .sp
/*	Warning: this can lock up the system to the point that you have
/*	to use the power switch (for example, Solaris 8 on 64-bit SPARC).
/* .sp
/*	Warning: this produces bogus results on Linux 2.2 kernels.
/* .sp
/*	Warning: this is very slow on 64-bit machines because the entire
/*	memory address range has to be searched.
/* .sp
/*	Warning: kernel virtual memory mappings change frequently. Depending
/*	on the operating system, mappings smaller than \fIpage_size\fR or
/*	\fIbuffer_size\fR may be missed or may be reported incorrectly.
/* .IP "\fB-b \fIbuffer_size\fR (default: 0)"
/*	Number of bytes per memory read operation. By default, the program
/*	uses the \fIpage_size\fR value.
/* .sp
/*	Warning: a too large read buffer size causes memory to be missed on
/*	FreeBSD or Solaris.
/* .IP "\fB-d \fIdump-size\fR (default: 0)"
/*	Number of memory bytes to dump. By default, the program runs
/*	until the memory device reports an end-of-file (Linux), or until
/*	it has dumped from \fB/dev/mem\fR as much memory as reported present
/*	by the kernel (FreeBSD, Solaris), or until pointer wrap-around happens.
/* .sp
/*	Warning: a too large value causes the program to spend a lot of time
/*	skipping over non-existent memory on Solaris systems.
/* .sp
/*	Warning: a too large value causes the program to copy non-existent
/*	data on FreeBSD systems.
/* .IP "\fB-m\fR \fImap_file\fR"
/*	Write the memory map to \fImap_file\fR, one entry per line.
/*	Specify \fB-m-\fR to write to the standard error stream.
/*	Each map entry consists of a region start address and the first
/*	address beyond that region. Addresses are separated by space,
/*	and are printed as hexadecimal numbers (0xhhhh).
/* .IP "\fB-p \fIpage_size\fR (default: 0)"
/*	Use \fIpage_size\fR as the memory page size. By default the program
/*	uses the system page size.
/* .sp
/*	Warning: a too large page size causes memory to be missed
/*	while skipping over holes in memory.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the program more verbose.
/* BUGS
/*	On many hardware platforms the firmware (boot PROM, BIOS, etc.)
/*	takes away some memory. This memory is not accessible through
/*	\fB/dev/mem\fR.
/*
/*	This program should produce output in a format that supports
/*	structure information such as ELF.
/* LICENSE
/*	This software is distributed under the IBM Public License.
/* AUTHOR
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	USA
/*--*/

/* System libraries. */

#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef SUNOS5
#include <errno.h>
#define _PATH_MEM "/dev/mem"
#define _PATH_KMEM "/dev/kmem"
#define GETPAGESIZE() sysconf(_SC_PAGESIZE)
#define SUPPORTED
#endif

#if defined(FREEBSD2) || defined(FREEBSD3) || defined(FREEBSD4) \
	|| defined(FREEBSD5) || defined(FREEBSD6) || defined(FREEBSD7) \
	|| defined(OPENBSD2) || defined(OPENBSD3) \
	|| defined(BSDI2) || defined(BSDI3) || defined(BSDI4)
#include <sys/param.h>
#include <sys/sysctl.h>
#include <paths.h>
#define GETPAGESIZE getpagesize
#define SUPPORTED
#endif

#ifdef LINUX2
#include <paths.h>
#define GETPAGESIZE getpagesize
#define SUPPORTED
#endif

 /*
  * Catch-all.
  */
#ifndef SUPPORTED
#error "This operating system version is not supported"
#endif

/* Application-specific. */

#include "convert_size.h"
#include "error.h"
#include "mymalloc.h"

 /*
  * Default settings.
  */
#define DEF_BUFF_SIZE	0		/* use page size */
#define DEF_PAGE_SIZE	0		/* use page size */
#define DEF_SCAN_SIZE	0		/* all memory */

#define OFFT_TYPE	unsigned long
#define OFFT_CAST	unsigned long
#define OFFT_FMT	"lx"

/* usage - complain, explain, and terminate */

static void usage(const char *why)
{
    if (why)
	remark("%s", why);
    error("usage: %s [options]\n"
	  "  -b read_buffer_size"
	  "     (default %lu, use the system page size)\n"
	  "  -k                 "
	  "     (dump kernel memory instead of physical memory)\n"
	  "  -m map_file        "
	  "     (print memory map)\n"
	  "  -p memory_page_size"
	  "     (default %lu, use the system page size)\n"
	  "  -s memory_dump-size"
	  "     (default %lu, dump all memory)\n"
	  "  -v                 "
	  "     (verbose mode for debugging)",
	  progname, (unsigned long) DEF_BUFF_SIZE,
	  (unsigned long) DEF_PAGE_SIZE,
	  (unsigned long) DEF_SCAN_SIZE);
}

/* get_memory_size - figure out the memory size if we need to */

static OFFT_TYPE get_memory_size(void)
{
#ifdef SUNOS5
    OFFT_TYPE pagesz = sysconf(_SC_PAGESIZE);
    OFFT_TYPE pagect = sysconf(_SC_PHYS_PAGES);

    return (pagesz * pagect);
#endif

#if defined(FREEBSD2) || defined(FREEBSD3) || defined(FREEBSD4) \
	|| defined(FREEBSD5) \
	|| defined(OPENBSD2) || defined(OPENBSD3)
    int     name[] = {CTL_HW, HW_PHYSMEM};
    size_t  len;
    unsigned unsigned_memsize;
    unsigned long ulong_memsize;
    OFFT_TYPE offt_memsize;

    if (sysctl(name, 2, (void *) 0, &len, (void *) 0, 0) < 0)
	error("sysctl: %m");
    if (len == sizeof(unsigned)) {
	if (sysctl(name, 2, &unsigned_memsize, &len, (void *) 0, 0) <0)
	    error("sysctl: %m");
	return (unsigned_memsize);
    } else if (len == sizeof(unsigned long)) {
	if (sysctl(name, 2, &ulong_memsize, &len, (void *) 0, 0) <0)
	    error("sysctl: %m");
	return (ulong_memsize);
    } else if (len == sizeof(OFFT_TYPE)) {
	if (sysctl(name, 2, &offt_memsize, &len, (void *) 0, 0) < 0)
	    error("sysctl: %m");
	return (offt_memsize);
    } else {
	error("unexpected sizeof(hw.physmem): %d", len);
    }
#endif

    return (0);
}


/* dump_memory - dump memory blocks between holes */

static void dump_memory(int fd, FILE * map, char *buffer, size_t buffer_size,
		              size_t dump_size, size_t page_size, int flags)
{
    OFFT_TYPE start;
    OFFT_TYPE where;
    OFFT_TYPE count;
    size_t  todo;
    ssize_t read_count;
    int     in_region = 0;

#define ENTER_REGION(map, flags, start, where, in_region) { \
	if (map) \
	    start = where; \
	in_region = 1; \
    }

#define LEAVE_REGION(map, flags, start, where, in_region) { \
	if (map) \
	    fprintf(map, "0x%" OFFT_FMT " 0x%" OFFT_FMT "\n", \
		   (OFFT_CAST) start, \
		   (OFFT_CAST) where); \
	in_region = 0; \
    }

    for (where = 0, count = 0; dump_size == 0 || count < dump_size; /* increment at end */ ) {

	/*
	 * Some systems don't detect EOF, so don't try to read too much.
	 */
	todo = (dump_size > 0 && dump_size - count < buffer_size ?
		dump_size - count : buffer_size);
#ifdef USE_PREAD
	read_count = pread(fd, buffer, todo, where);
#else
	read_count = read(fd, buffer, todo);
#endif
	if (read_count == 0) {
	    if (verbose)
		remark("Stopped on EOF at 0x%" OFFT_FMT, (OFFT_CAST) where);
	    break;
	}
	if (read_count > 0) {
	    if (in_region == 0)
		ENTER_REGION(map, flags, start, where, in_region);
	    if (write(1, buffer, read_count) != read_count)
		error("output write error: %m");
	    count += read_count;
	    if (where + read_count < where) {
		remark("Stopped on OFFT_TYPE wraparound after 0x%" OFFT_FMT,
		       (OFFT_CAST) where);
		break;
	    }
	    where += read_count;
	    if (verbose > 1)
		remark("count = 0x%" OFFT_FMT, (OFFT_CAST) count);
	}
	if (read_count < 0) {
	    if (in_region)
		LEAVE_REGION(map, flags, start, where, in_region);
	    if (where + page_size < where) {
		remark("Stopped on OFFT_TYPE wraparound after 0x%" OFFT_FMT,
		       (OFFT_CAST) where);
		break;
	    }
#ifdef USE_PREAD
	    if (errno != EFAULT) {
		if (verbose)
		    remark("Stopped on read error after 0x%" OFFT_FMT ": %m",
			   (OFFT_CAST) where);
		break;
	    }
#else
	    if (lseek(fd, where + page_size, SEEK_SET) < 0) {
		if (verbose)
		    remark("Stopped on lseek error after 0x%" OFFT_FMT,
			   (OFFT_CAST) where);
		break;
	    }
#endif
	    where += page_size;
	    if (verbose > 1)
		remark("where = 0x%" OFFT_FMT, (OFFT_CAST) where);
	}

	/*
	 * Kluge to prevent pointer wrap-around.
	 */
	if (where != (OFFT_TYPE) (unsigned long) (char *) (unsigned long) where) {
	    if (verbose)
		remark("Stopped on pointer wraparound at 0x%" OFFT_FMT,
		       (OFFT_CAST) where);
	    break;
	}
    }
    if (in_region)
	LEAVE_REGION(map, flags, start, where, in_region);

    /*
     * Sanity check.
     */
    if (dump_size > 0 && where < dump_size)
	remark("warning: found only 0x%" OFFT_FMT " of 0x%" OFFT_FMT "bytes",
	       (OFFT_CAST) count, (OFFT_CAST) dump_size);
    close(fd);
}

/* main - main program */

int     main(int argc, char **argv)
{
    size_t  page_size = DEF_PAGE_SIZE;
    size_t  dump_size = DEF_SCAN_SIZE;
    char   *buffer;
    size_t  buffer_size = DEF_BUFF_SIZE;
    int     ch;
    int     flags = 0;
    int     fd;
    FILE   *map = 0;
    const char *path = _PATH_MEM;

    progname = argv[0];

    /*
     * Parse JCL.
     */
    while ((ch = getopt(argc, argv, "b:km:p:s:v")) > 0) {
	switch (ch) {

	    /*
	     * Read buffer size.
	     */
	case 'b':
	    if ((buffer_size = convert_size(optarg)) == -1)
		usage("bad read buffer size");
	    break;

	    /*
	     * Dump kernel memory instead of physical memory.
	     */
	case 'k':
	    path = _PATH_KMEM;
	    break;

	    /*
	     * Show memory map.
	     */
	case 'm':
	    if (strcmp(optarg, "-") == 0) {
		map = stderr;
	    } else {
		if ((map = fopen(optarg, "w")) == 0)
		    error("create map file %s: %m", optarg);
	    }
	    break;

	    /*
	     * Page size.
	     */
	case 'p':
	    if ((page_size = convert_size(optarg)) == -1)
		usage("bad memory size");
	    break;

	    /*
	     * Amount of memory to copy.
	     */
	case 's':
	    if ((dump_size = convert_size(optarg)) == -1)
		usage("bad memory dump size");
	    break;

	    /*
	     * Verbose mode.
	     */
	case 'v':
	    verbose++;
	    break;

	    /*
	     * Error.
	     */
	default:
	    usage((char *) 0);
	    break;
	}
    }

    /*
     * Sanity checks.
     */
    if (page_size == 0)
	page_size = GETPAGESIZE();
    if (buffer_size == 0)
	buffer_size = page_size;
    if (dump_size == 0 && strcmp(path, _PATH_KMEM) != 0)
	dump_size = get_memory_size();

    /*
     * Audit trail.
     */
    if (verbose) {
	remark("dump size 0x%" OFFT_FMT, (OFFT_CAST) dump_size);
	remark("page size 0x%" OFFT_FMT, (OFFT_CAST) page_size);
	remark("buffer size 0x%" OFFT_FMT, (OFFT_CAST) buffer_size);
    }

    /*
     * Allocate buffer. This does not need to be a multiple of the page size.
     */
    if ((buffer = mymalloc(buffer_size)) == 0)
	error("no read buffer memory available");

    /*
     * Dump memory, skipping holes.
     */
    if (optind != argc)
	usage("too many arguments");
    if ((fd = open(path, O_RDONLY, 0)) < 0)
	error("open %s: %m", path);
    dump_memory(fd, map, buffer, buffer_size,
		dump_size, page_size, flags);
    if (map && fclose(map))
	error("map file write error: %m");
    exit(0);
}
