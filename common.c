/*
 *  common.c
 *
 *
 *  	Copyright (C) 2001  Ligesh ligesh@lxlinux.com
 *		
 *
 */

#define EXTERN extern

#include "gdbv.h"

void freeNonzero(char *ptr)
{
	if (ptr) free(ptr);
}

int killNonzero(int pid, int signal)
{
	return pid? kill(pid, signal): 0;
}

void initFiles()
{
	f_iput = ".gt_iput";
	f_oput = ".gt_oput";
	f_data = ".gt_data";
	f_fpos = ".gt_fpos";
	f_ferr = ".gt_ferr";
	f_fpid = ".gt_fpid";
	f_fasm = ".gt_asm";
}


int checkTimeStamp(char *fname)
{
	struct stat buf_asm, buf_file;
	char *fasm = ".gt_asm";
	if (fname == NULL) return 0;
	if (access(fname, F_OK)) return 0;
	if (access(fasm, F_OK)) return 1;
	stat(fname, &buf_file);
	stat(fasm, &buf_asm);
	if (buf_file.st_mtime > buf_asm.st_mtime) return 0;
	return 1;
}



void errExit(char *s, int val)
{
	if (val <= 0) {
		printf("%s\n",s);
		val*=-1;
		exit(val);
	}
	else {
		printf("%d: %s\n", val, s);
		return;
	}
}
		

