/*
 *  gdbvcl.c
 *	
 *  	Copyright (C) 2001  ligesh ligesh@lxlinux.com
 *  						gtk port by skannan@cyberspace.org
 *
 *		The client library to access the Gdb server.
 *
 */

#define EXTERN

#include "gdbv.h"
#include <gtk/gtk.h>
#include "vim.h"

gint my_handler_id;
GtkWidget *my_mainwin;
int return_val;
gint tag;

gint my_handle_event(GtkWidget *widget, GdkEvent *event, gpointer data);
unsigned char *my_func(unsigned char *arg);



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
	char *val= (char *) malloc(1);
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

int handle_event(GtkWidget *win, GdkEvent  *event, gpointer data)
{
	int i;
	static int count;
	switch(event->type)
	{
		case GDK_KEY_PRESS:
			if( (event->key.state & GDK_CONTROL_MASK) && event->key.keyval=='c')
			{
				int signal = count > 1? SIGKILL: SIGINT;
				count = (count == 1? 2: 1);
				/*printf("it's a <Ctrl-C>, returning.... %d %d \n", getServPid(), count);*/
				killNonzero(getServPid(), signal);
			}
	}
	/*
	if (count < 0) {
		printf("[0;32mServer is killed[m");
		fflush(stdout);
		return QUIT;
	}
	count = -20;
	}
	else if (c == 3) {
		count++;
		write(1, "Sending Sigint to the Server... ", 32);
		write(1, "Press ^K to Terminate the Server\n", 32);
		for (i = 0; i<64; i++)  write(1, "", 1);
		killNonzero(getServPid(), SIGINT);
	}
	*/
	return 1;
}

int get_val(gpointer data, gint fd, GdkInputCondition val)
{
	char c[2];
	read(fd, c, 2);
	if(c[0] & QUIT) {
		fflush(stdout);
	}
	return_val = (int) c[0];
	gdk_input_remove(tag);
	gtk_signal_disconnect(GTK_OBJECT(my_mainwin), my_handler_id);
	gtk_main_quit();
	return 1;
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
	int fd, count = 0;
	char c[2];
	struct stat buf;
	my_mainwin = gui.mainwin;
	
	fd = open(f_oput, O_RDONLY | O_NONBLOCK);
	if (fd < 0) return QUIT;

	my_handler_id = gtk_signal_connect(GTK_OBJECT(my_mainwin), "event", GTK_SIGNAL_FUNC(handle_event), NULL);
	tag = gdk_input_add(fd, GDK_INPUT_READ, (GdkInputFunction) get_val, NULL);
	gtk_main();
	close(fd);
	/*
	if (!access(f_ferr, F_OK)) {
		stat(f_ferr, &buf);
		if (buf.st_size > 0) return_val |= ERRN;
	}
	*/
	return return_val;
}

int startServer(char *val)
{
	int ret;
	initFiles();
	initClient();

	if (!makeFifo()) {
		printf("[0;32m Could not create Fifo's[m");
		fflush(stdout);
		return QUIT;
	}
	execShellServ(val);
	ret = waitOutput();
	return ret;
}

int writeServer(char *val)
{
	int fd, ret;
	char *cmd;

	initFiles();

	if (access(f_fpid, F_OK) || access(f_iput, F_OK)) {
		//printf("[0;31mNo Server; Start it with: "); 
		//printf("[0;33m'Idestart <arguments>'[m");
		fflush(stdout);
		return QUIT;
	}
	initClient();

	cmd = (char *)malloc(strlen(val) + 2);
	sprintf(cmd, "%s\n", val);

	fd = open(f_iput, O_WRONLY | O_NONBLOCK);

	if (fd < 0) {
		usleep(100000);
		fd = open(f_iput, O_WRONLY | O_NONBLOCK);
		if (fd < 0) {
			//printf("[0;32m Server Does not Respond[m");
			return NRES;
		}
	}
	ret = write(fd, cmd, strlen(cmd) + 1);
	close(fd);
	freeNonzero(cmd);

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
