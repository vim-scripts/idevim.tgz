/*
 *  gdbvcl.c
 *	
 *  	Copyright (C) 2001  ligesh ligesh@lxlinux.com
 *
 *		The client library to access the Gdb server.
 *
 */

#define EXTERN

#include "gdbv.h"
#include <stdlib.h>


void initFiles();
void errExit(char *, int );
void freeNonzero(char *);
int killNonzero(int , int );


void  fileWrite(char *s, char *fname)
{
	FILE *fp = fopen(fname, "w");
	fprintf(fp, "%s\n", s);
	fclose(fp);
}

int makeFifo()
{
	if (!access(f_iput, F_OK)) return 1;
	return (!mkfifo(f_iput,0644) && !mkfifo(f_oput,0644));
}

void cleanOuput()
{
	char c;
	int fd = open(f_oput, O_RDONLY | O_NONBLOCK);
	if (fd < 0) return;
	while (read(fd, &c, 1) > 0);
	close(fd);
}

void execShellServ(char *val)
{
	char cmd[4096];
	sprintf(cmd, "gdbvserv %s &", val);
	system(cmd);
}

char *concatArgs(char **argv)
{
	int i;
	char *val=  malloc(1);
	*val = 0;
	for (i = 1; argv[i] ; i++) {
		val =(char *) realloc(val, strlen(val) + strlen(argv[i]));
		if (i != 1)  strcat(val, " ");
		strcat(val, argv[i]);
	}
	return val;
}


int getServPid()
{
	int pid;
	FILE *fp = fopen(f_fpid, "r");
	if (!fp) return 0;
	fscanf(fp, "%d", &pid);
	fclose(fp);
	return pid;
}

int getReturnVal()
{
	int fd, count = 0;
	char c[2];
	fd_set fd_read;

	fd = open(f_oput, O_RDONLY | O_NONBLOCK);
	if (fd < 0) return QUIT;

	FD_ZERO(&fd_read);
	FD_SET(0, &fd_read);
	FD_SET(fd, &fd_read);
	for (;;) {
		int ret = select(fd + 1, &fd_read, NULL, NULL, NULL);
		if (FD_ISSET(fd, &fd_read)) {
			read(fd, c, 2);
			break;
		}
		if (FD_ISSET(0, &fd_read)) {
			int i;
			read(0, c, 1);
			if (c[0] == 11) {
				int signal = count < 0? SIGKILL: SIGTERM;
				killNonzero(getServPid(), signal);
				if (count < 0) {
					printf("[0;32mServer is killed[m");
					fflush(stdout);
					return QUIT;
				}
				count = -20;
			}
			else if (c[0] == 3) {
				count++;
				write(1, "Sending Sigint to the Server... ", 32);
				write(1, "Press ^K to Terminate the Server\n", 32);
				for (i = 0; i<64; i++)  write(1, "", 1);
				killNonzero(getServPid(), SIGINT);
			}
		}
		FD_ZERO(&fd_read);
		FD_SET(0, &fd_read);
		FD_SET(fd, &fd_read);
	}
	close(fd);
	if(c[0] & QUIT) {
		printf("[0;32mServer is closed                          [m");
		fflush(stdout);
	}
	return count > 0? (int) c[0] | CLRS: (int) c[0] ;
}


void initClient()
{
	int fdt;
	cleanOuput();
	//fdt = open(f_ferr, O_WRONLY | O_TRUNC, 0644);
	//if (fdt >= 0) close(fdt);
}


int waitOutput()
{
	int ret;
	ret = getReturnVal();
	return ret;
}

int startServer(char *val)
{
	int s_present = 0;
	initFiles();
	initClient();
	if (!strncmp(val, "-server", 7)) s_present = 1;
	if (!makeFifo()) {
		printf("[0;32m Could not create Fifo's[m");
		fflush(stdout);
		return QUIT;
	}
	execShellServ(val);
	if (s_present) return 0;
	return waitOutput();
}

int writeServer(char *val)
{
	int fd, fdt, tmp, ret, s_present = 0;
	char *cmd;

	initFiles();

	while(isblank(val[0])) val++;
	if (!strncmp(val, "-server", 7)) s_present = 1;

	if (access(f_fpid, F_OK) || access(f_iput, F_OK)) {
		printf("[0;31mNo Server; Start it with: "); 
		printf("[0;33m'Idestart <arguments>'[m");
		fflush(stdout);
		return QUIT;
	}
	initClient();


	cmd = malloc(strlen(val) + 2);
	sprintf(cmd, "%s\n", val);

	fd = open(f_iput, O_WRONLY | O_NONBLOCK);

	if (fd < 0) {
		usleep(100000);
		fd = open(f_iput, O_WRONLY | O_NONBLOCK);
		if (fd < 0) {
			printf("[0;32m Server Does not Respond[m");
			fflush(stdout);
			return NRES ;
		}
	}
	ret = write(fd, cmd, strlen(cmd) + 1);
	close(fd);
	freeNonzero(cmd);

	if (s_present) return 0;
	return waitOutput();
}



main(int argc, char **argv)
{

	char *val;
	int ret;
	
	if (argc > 1 && !strcmp(argv[1], "start")) {
		argv+=1;
		val = concatArgs(argv);
		ret = startServer(val);
	}
	else {
		val = concatArgs(argv);
		ret = writeServer(val);
	}
			
	exit(ret);
}

