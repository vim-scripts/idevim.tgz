/* 	  gdbvserv.c
 *
 *  
 *  	Copyright (C) 2001  ligesh ligesh@lxlinux.com
 *
 *		The server that runs gdb and controls its i/o.
 *
 */

#define EXTERN
#include "gdbv.h"
#include <stdlib.h>

#define GDB_MAX_OUTPUT 409600

int gdb_pid;
int g_assembley;
char *s_name;
int p_fd_r[2], p_fd_w[2], err_fd;
char disfile[1024];

struct out {
	char *data;
	char *fpos;
	int flag;
} out;

void initFiles();
void errExit(char *, int );
void freeNonzero(char *);

void cleanServer()
{
	unlink(f_oput);
	unlink(f_iput);
	unlink(f_fpid);
	//unlink(f_ferr);
	unlink(f_data);
	unlink(f_fpos);
		
}


void returnCmd(int val, int noblock)
{
	int fd, ret, open_flag = O_WRONLY;
	char c = val;
	char cmd[1024];

	if (s_name) {
		int ret;
		/*
		sprintf(cmd, "vim --serverlist | grep -q %s", s_name);
		ret = system(cmd);
		if (ret) {
			cleanServer();
			kill(gdb_pid, SIGTERM);
			exit(10);
		}
		*/
		sprintf(cmd, "gvim --servername %s --remote-send '<C-\\><C-N>:call GdbDataChange(%d)<CR>'", s_name, val);
		system(cmd);
		return ;
	}

	if (noblock) open_flag |= O_NONBLOCK;

	fd = open(f_oput, open_flag);
	if (fd < 0) {
		usleep(100000);
		fd = open(f_oput, open_flag);
		if (fd < 0) return;
	}
	write(fd, &c, 1);
	close(fd);


}



void cleanExit(int val)
{
	int status, pid;
	if (val == 17) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid != gdb_pid) return ;
	}
	returnCmd(QUIT, 1);
	cleanServer();
	kill(gdb_pid, SIGTERM);
	exit(10);
}


void writePidToFile()
{
	FILE *fp = fopen(f_fpid, "w");
	if (!fp) {
		returnCmd(QUIT, 1);
		errExit("Could not Open Pid File", -8);
	}
	fprintf(fp, "%d", gdb_pid);
	fclose(fp);
}

void initServer()
{
	pipe(p_fd_r);
	pipe(p_fd_w);
	//err_fd = open(f_ferr, O_WRONLY | O_CREAT, 0644);
	unlink(f_data);
	signal(SIGCHLD, cleanExit);
	signal(SIGTERM, cleanExit);
}

void execGdb(int i_fd, int o_fd, int e_fd, int argc, char **argv)
{
	int i;
	char *prog_name;
	char **exec_arg = (char **) malloc((argc + 2)*sizeof(char *));
	if((prog_name = getenv("VIM_GDBCMD")) == 0) prog_name = "gdb";

	strcpy((exec_arg[0] = (char *) malloc(strlen(prog_name) + 1)), prog_name);

	exec_arg[1] = "-f";

	for (i = 2; i < argc+2; i++) exec_arg[i] = argv[i-1];
		
	gdb_pid = fork();

	if (gdb_pid < 0) {
		returnCmd(ERRN | QUIT, 1);
		errExit("Fork Failed", -1);
	}
	else if (!gdb_pid) {
		close(0); dup(i_fd);
		close(1); dup(o_fd);
		// close(2); dup(e_fd);  Std error Differnt from Stdout
		close(2); dup(o_fd); 
		execvp(prog_name, exec_arg);
		printf("%s: %s Exec Failed You ass!\n", prog_name, exec_arg);
	}
	else return;
}
		
int checkForPrompt(char *dat)
{
	char *end = dat + strlen(dat);
	//printf("In check %s\n", dat);
	if (!strncmp(end - 6, "(gdb) ", 6)) {
		return 6;
	}
	if (*(end - 1) == '>') {
		if ((end - 1) == dat || *(end - 2) == '\n')
			return 1;
	}
	return 0;
}

int checkForBlock(int fd)
{
	fd_set fd_read;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 90;

	FD_ZERO(&fd_read);
	FD_SET(fd, &fd_read);
	return select(fd + 1, &fd_read, NULL, NULL, &tv) > 0? 0: 1 ;
}
	

void waitForCommand(char *cmd)
{
	int fd = open(f_iput, O_RDONLY);
	while(read(fd, cmd++, 1));
	*cmd = 0;
	close(fd);
}


/* Recognition of Gdb output happens here. We search the ouput for the 
 * prompt, but that is not enough. Then we check whether the output
 * is blocked. Now we can be sure that we have reached the end of the 
 * output.
 */

int  waitForOutput(int fd, char *op)
{
	char *ptr = op;
	int prompt, count = 0;
	for (;;) {
		count++;
		if(count >= GDB_MAX_OUTPUT) {
			*ptr = 0;
			return 0;
		}
		read(fd, ptr++, 1);
		if ((prompt = checkForPrompt(op)) && checkForBlock(fd)) break;
	}

	ptr-=prompt;
	*ptr = 0;
	return prompt == 1? 4: 0;
}

char *getLineFromString(char *s, char *l)
{
	while (*s) {
		if (*s == '\n') {
			*l++ = *s++;
			*l = 0;
			return *s? s: 0;
		}
		*l++ = *s++;
	}
	*l = 0;
	return 0;
}

/* This function checks for "function () at file.c:number" which is 
 *  printed when moving from one function to another. 
 * The checking is pretty vague but should work.
 */

int checkFilterLine(char *s)
{
	char *ptr;
	/* The frames shouldn't be filtered */
	if (s[0] == '#') return 0; 

	ptr = strchr(s, '(');
	if (!ptr) return 0;
	ptr = strchr(ptr, ')');
	if (!ptr) return 0;
	ptr = strstr(ptr, " at ");
	if (!ptr) return 0;
	ptr = strchr(ptr, ':');
	return ptr? 1: 0;
}
	
/* This function is not very essential. Should keep an option using which
 * this can be turned off by the user.
 */

void filterOutput(char *s)
{
	char line[8192];
	char *ptr;
	char *tmp = malloc(strlen (s) + 1);
	strcpy(tmp, s);
	s[0] = 0;
	ptr = strstr(tmp, "This GDB was configured as");
	if (ptr) {
		strcpy(tmp, "Vim interface ");
		strcat(tmp, "by ligesh <ligesh@lxlinux.com>\n");
		strcat(tmp, ptr);
	}
	while ((tmp = getLineFromString(tmp, line))) {
		if (line[0] == '\n' || checkFilterLine(line) )
			continue;
		strcat(s, line);
	}
	/* We allow the last one line without filtering */
	strcat(s, line);
	freeNonzero(tmp);
}

int processOutput(char *op, struct out *val)
{
	char *ptr, *tptr;;
	filterOutput(op);

	ptr = strstr(op, "\032\032");
	if (!ptr) {
		val->data = malloc(strlen(op) + 1);
		strcpy(val->data, op);
		val->flag |= DATA;
		return val->flag; 
	}
	*ptr = 0;
	if (ptr != op) {
		val->data = malloc(strlen(op) + 1);
		strcpy(val->data, op);
		val->flag |= DATA;
	}
	ptr+=2;
	tptr = strstr(ptr, "\n");
	*tptr = 0;
	val->fpos = malloc(strlen(ptr) + 1);
	strcpy(val->fpos, ptr);
	val->flag |= FPOS;
	ptr = tptr + 1;

	if (*ptr != 0) {
		int len = strlen(ptr);
		if (val->flag & DATA) 
			val->data = (char *)realloc(val->data, strlen(val->data) + len +1);
		else 
			(val->data = malloc(len + 1))[0] = 0;
		val->flag |= DATA;
		strcat(val->data, ptr);
	}
	return val->flag;
}

void printOutput(struct out *val, char *cmd)
{
	if (val->flag & DATA ) {
		if (g_assembley) {
			FILE *fp = fopen(f_fasm, "a");
			g_assembley = 0;
			fprintf(fp, "%s", val->data);
			val->flag &= ~DATA;
			fclose(fp);
			unlink(disfile);
		} else {
			FILE *fp = fopen(f_data, "a");
			fprintf(fp, "Gdb> ");
			while (cmd[0] == 32) cmd++;

			if (cmd[0] == 'p' || cmd[0] == 'x') fprintf(fp,"%s", cmd); 
			else fprintf(fp, "\n");

			fprintf(fp, "%s", val->data);
			fclose(fp);
		}
	}

	if (val->flag & FPOS) {
		FILE *fp = fopen(f_fpos, "w");
		fprintf(fp, "%s\n", val->fpos);
		fclose(fp);
	}
}
	

void tmpWrite(char *filename, char *string)
{
	FILE *ftmp = fopen(filename, "w");
	fprintf(ftmp, "Hell%s\n", string);
	fclose(ftmp);
}

char *tmpFileName(char *filename, char *base)
{
	time_t tim;
	do {
		time(&tim);
		sprintf(filename, "%s_%d", base, tim);
	} while (!access(filename, F_OK));
	return filename;
}

int main(int argc, char **argv)
{
	int n_pass = 0;
	char cmd[4096], *cmdptr = cmd, obuff[GDB_MAX_OUTPUT];
	out.data = out.fpos = 0;
	g_assembley = 0;

	signal(SIGPIPE, SIG_IGN);
	initFiles();
	initServer();
	if (argc > 1 && !strcmp(argv[1], "-server")) {
		if (argc < 2) return 0;
		s_name = (char *) malloc(strlen(argv[2]) + 1);
		strcpy(s_name, argv[2]);
		argc-=2;
		argv+=2;
	} else s_name = 0;

	execGdb(p_fd_w[0], p_fd_r[1], err_fd, argc, argv);
	writePidToFile();

	for (;;) {
		n_pass++;
		bzero(obuff, sizeof(obuff));
		if (n_pass > 1) {
			waitForCommand(cmd);
			cmdptr = cmd;
			while(isblank(cmdptr[0])) cmdptr++;
			if (!strncmp(cmdptr, "-server", 7)) {
				char tserver[1024];
				cmdptr+=7;
				while (isblank(cmdptr[0])) cmdptr++;
				sscanf(cmdptr, "%s", tserver);
				cmdptr+=strlen(tserver);
				while (isblank(cmdptr[0])) cmdptr++;
				freeNonzero(s_name);
				s_name = (char *) malloc(strlen(tserver) + 1);
				strcpy(s_name, tserver);
				//printf("Server Name: %s %s\n", s_name, cmdptr);
			} else {
				freeNonzero(s_name);
				s_name = 0;
			}
			if (!strncmp(cmdptr, "my_disassemble", 14)) {
				FILE *fpt;
				char tmpstr[1024];
				g_assembley = 1;
				tmpFileName(disfile, "/tmp/vimgdbdis");
				fpt = fopen(disfile, "w");
				if (fpt) {
					cmdptr+=3;
					fprintf(fpt, "define my_dis\ndont-repeat\n %send\nmy_dis\n", cmdptr);
					fclose(fpt);
					sprintf(cmdptr, "source %s\n", disfile);
				} else {
					strcpy(tmpstr, &cmdptr[3]);
					sprintf(cmdptr, "define my_dis\ndont-repeat\n %send\nmy_dis\n", tmpstr);
				}
			}
			write(p_fd_w[1], cmdptr, strlen(cmdptr));
		}

		out.flag = waitForOutput(p_fd_r[0], obuff);
		if (obuff[0] != 0) {
			processOutput(obuff, &out);
			printOutput(&out, cmdptr);
		}
		returnCmd(out.flag, 0);
		freeNonzero(out.data);
		freeNonzero(out.fpos);
		out.data = out.fpos = 0;
		out.flag = 0;
	}
}

