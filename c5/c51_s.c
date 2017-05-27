#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include <sys/loadavg.h>

#include "errors.h"
#include "c51.h"

int main()
{
    pid_t pid = getpid(), pgrp = getpgrp();
    uid_t uid = getuid();
    time_t start_time = time(0);
    size_t memory_size = sizeof(struct proc_info);             /* total size of memory */
    int shmid;
    key_t key = 5678;
    struct proc_info * shm;

    if (start_time == (time_t)-1)
    {
        perror("time syscall failed");
        _exit(TIMECALL_FAILCODE);
    }

    if ((shmid = shmget(key, memory_size, IPC_CREAT | 0666)) < 0)
    {
        perror("shmget syscall failed");
        _exit(SHMGET_FAILCODE);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (struct proc_info *) -1)
    {
        perror("shmat syscall failed");
        _exit(SHMAT_FAILCODE);
    }

    shm->pid = pid;
    shm->pgrp = pgrp;
    shm->uid = uid;

    while (1)
    {
        time_t curr_time = time(0);
        if (curr_time == (time_t)-1)
        {
            perror("time syscall failed");
            shmctl(key, IPC_RMID, NULL);
            _exit(TIMECALL_FAILCODE);
        }

        shm->time_since_start = curr_time - start_time;

        if (getloadavg(shm->loadavg_arr, LOADAVG_ARR_MAX) == -1)
        {
            perror("loadavg function call failed");
            shmctl(key, IPC_RMID, NULL);
            _exit(LOADAVG_FAILCODE);
        }

        sleep(1);
    }

    return 0;
}