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

#include "struct.h"

#define FILE_PATH "test"

struct data* info;
time_t start_time;
int fd;

void write_info()
{
	info->pid = getpid();
	info->uid = getuid();
	info->gid = getgid();
	info->time = 0;
	getloadavg(info->loadsystemtime, LOADAVG_NSTATS); 
}

void update_info()
{
	info->time = time(NULL) - start_time;	
	getloadavg(info->loadsystemtime, LOADAVG_NSTATS); 
}

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

	start_time = time(NULL);	

	struct sigaction sig;
	memset(&sig, 0, sizeof(struct sigaction));
	sig.sa_handler = handle_sig;
	sigaction(SIGINT, &sig, NULL);

	unlink(FILE_PATH);
 
        if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
        	perror("socket");
                return 1;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, FILE_PATH);

        if(bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) 
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

	info = malloc(sizeof(struct data));

	write_info();

	while (1)
	{
		update_info();
		client = accept(fd, NULL, NULL);
		if(client != -1) 
		{
			write(client, info, sizeof(struct data));
		}
		sleep(1);
	}
	return 0;
}
