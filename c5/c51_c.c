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

    if ((shmid = shmget(key, sizeof(struct proc_info *), 0666)) < 0)
    {
        perror("shmget syscall failed");
        _exit(SHMGET_FAILCODE);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (struct proc_info *) -1)
    {
        perror("shmat syscall failed");
        _exit(SHMAT_FAILCODE);
    }

    printf("Server pid = %ld\nServer pgrp = %ld\nServer's uid = %ld\n", shm->pid, shm->pgrp, shm->uid);
    printf("Seconds since server startup: %ld\n", shm->time_since_start);
    printf("Load average for:\n\t1 min = %f\n\t5 min = %f\n\t15 min = %f\n", shm->loadavg_arr[LOADAVG_1MIN], shm->loadavg_arr[LOADAVG_5MIN], shm->loadavg_arr[LOADAVG_15MIN]);

    shmdt(shm);

    return 0;
}
