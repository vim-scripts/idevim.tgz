#ifndef _GDBV_H
#define _GDBV_H

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#define DATA  1
#define FPOS  2
#define CONT  4
#define ERRN  8
#define QUIT  16
#define CLRS  32
#define NRES  64

EXTERN char *f_iput, *f_oput, *f_data, *f_fpos, *f_ferr, *f_fpid, *f_fasm;

#endif      /* _GDBV_H   */

