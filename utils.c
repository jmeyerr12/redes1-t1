#include "utils.h"

void allocation_test(void *ptr)
{
    if (!ptr)
    {
        fprintf(stderr, "memory allocation error\n");
        exit(1);
    }
}

double timestamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((double)(tp.tv_sec * 1000.0 + tp.tv_usec / 1000.0));
}