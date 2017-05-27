#ifndef _C51_H
#define _C51_H
#define _SVID_SOURCE

#include <sys/types.h>
#include <sys/loadavg.h>
#include <unistd.h>
#include <time.h>

#define LOADAVG_ARR_MAX

struct proc_info 
{
    pid_t pid;
    pid_t pgrp;
    uid_t uid;

    time_t time_since_start;

    double loadavg_arr[LOADAVG_ARR_MAX];
};

#endif