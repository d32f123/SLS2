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
int fd;


int main() 
{
	struct sockaddr_un addr;

 
        if((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
	{
        	perror("socket");
                return 1;
        }

        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, FILE_PATH);


	info = malloc(sizeof(struct data));

	if(connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)  
	{
		perror("connect");
		return 1;
	}

	if(read(fd, info, sizeof(struct data)) != sizeof(struct data))
	{
		printf("Read error!\n");
		return 1; 
	}

	
	printf("%li\n%li\n%li\n", info->pid, info->uid,  info->gid);
	printf("%li\n", info->time);
	printf("%.6lf %.6lf %.6lf\n", info->loadsystemtime[0], info->loadsystemtime[1], info->loadsystemtime[2]);

	return 0;
}
