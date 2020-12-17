#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <q.h>
#include <stdio.h>

void linit()
{
    int i, j;
    
    for(i = 0; i < NLOCKS; i++)
    {
        locktab[i].lstate = LFREE;
        locktab[i].ltype = DELETED;
        locktab[i].readerCount = 0;

        // Create wait queue for lock
        locktab[i].lqhead = newqueue();

        locktab[i].lqtail = 1 + locktab[i].lqhead;

        // Initialise lock process mapping.
        for(j = 0; j < NPROC; j++)
        {
            locktab[i].lprocmap[j] = 0;
        }
    }
}