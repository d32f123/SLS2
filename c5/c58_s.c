#define _XOPEN_SOURCE 500
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>

#include "c51.h"
#include "errors.h"

#define FILE_PATH "socket"

struct proc_info info;
time_t start_time;
int fd;

void handle_sig(int sig_number)
{
	(void)sig_number;
	close(fd);
	_exit(0);	
}

int main() 
{
	struct sockaddr_un addr;
	int client, flags;
	pid_t pid = getpid(), pgrp = getpgrp();
    	uid_t uid = getuid();
    	time_t start_time = time(0);
	struct sigaction sig;

	if (start_time == (time_t)-1)
    	{
        	perror("time syscall failed");
        	_exit(TIMECALL_FAILCODE);
    	}

	start_time = time(NULL);	

	
	memset(&sig, 0, sizeof(struct sigaction));
	sig.sa_handler = handle_sig;
	sigaction(SIGINT, &sig, NULL);

	unlink(FILE_PATH);
 
    	if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, FILE_PATH);

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
	{
		perror("bind");
		return 1;
	}

	if(listen(fd, 10) == -1) 
	{
		perror("listen");
		return 1;
	}

	flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);

	info.pid = pid;
	info.pgrp = pgrp;
	info.uid = uid;

	while (1)
	{
		time_t curr_time = time(0);
        	if (curr_time == (time_t)-1)
        	{
            		perror("time syscall failed");
            		_exit(TIMECALL_FAILCODE);
        	}

		info.time_since_start = curr_time - start_time;

        	if (getloadavg(info.loadavg_arr, LOADAVG_ARR_MAX) == -1)
        	{
            		perror("loadavg function call failed");
            		_exit(LOADAVG_FAILCODE);
        	}

		while ((client = accept(fd, NULL, NULL)) != -1)
		{
			write(client, &info, sizeof(struct proc_info));
		}
		sleep(1);
	}
	return 0;
}
