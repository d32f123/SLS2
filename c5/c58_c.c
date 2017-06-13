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
int fd;


int main() 
{
	struct sockaddr_un addr;

 
	if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		_exit(SOCKET_FAILCODE);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, FILE_PATH);

	if(connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1)  
	{
		perror("connect");
		_exit(CONNECT_FAILCODE);
	}

	if(read(fd, &info, sizeof(struct proc_info)) != sizeof(struct proc_info))
	{
		printf("Read error!\n");
		_exit(READ_FAILCODE);
	}

	
	printf("Server pid = %ld\nServer pgrp = %ld\nServer's uid = %ld\n", info.pid, info.pgrp, info.uid);
    printf("Seconds since server startup: %ld\n", info.time_since_start);
    printf("Load average for:\n\t1 min = %f\n\t5 min = %f\n\t15 min = %f\n", info.loadavg_arr[LOADAVG_1MIN], info.loadavg_arr[LOADAVG_5MIN], info.loadavg_arr[LOADAVG_15MIN]);

	return 0;
}
