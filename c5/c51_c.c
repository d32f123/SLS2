#define _SVID_SOURCE

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/loadavg.h>

#include "c51.h"
#include "errors.h"

int main()
{
    int shmid;
    key_t key = 5678;
    struct proc_info * shm;

    if ((shmid = shmget(key, SHMSZ, 0666)) < 0)
    {
        perror("shmget syscall failed");
        _exit(SHMGET_FAILCODE);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1)
    {
        perror("shmat syscall failed");
        _exit(SHMAT_FAILCODE);
    }

    printf("Server pid = %d\nServer pgrp = %d\nServer's uid = %d\n", shm->pid, shm->pgrp, shm->uid);
    printf("Seconds since server startup: %d", pid->time_since_start);
    printf("Load average for:\n\t1 min = %d\n\t5 min = %d\n\t15 min = %d\n", shm->loadavg_arr[LOADAVG_1MIN], shm->loadavg_arr[LOADAVG_5MIN], shm->loadavg_arr[LOADAVG_15MIN]);

    return 0;
}