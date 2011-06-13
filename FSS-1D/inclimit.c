//inclimit.c
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

// Increases the limit for the number of UNIX file descriptors, 
// needeed when running MPI programms on many machines

void inclimit(void)
{
        struct rlimit my_limit;
        getrlimit (RLIMIT_NOFILE, &my_limit);
        my_limit.rlim_cur = 1024;
        if (setrlimit(RLIMIT_NOFILE, &my_limit) == -1)
                perror("error in setrlimit");
        else
                printf("Increased limit\n");
}
