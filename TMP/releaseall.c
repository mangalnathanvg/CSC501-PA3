#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <q.h>

/*--------------------------------------------------------------------------
 * releaseProcFromLockQ  --  Release current process from lock's wait queue
 *--------------------------------------------------------------------------
 */
int releaseProcFromLockQ(int ldes)
{
    struct lentry *lptr;
    struct pentry *pptr;

    lptr = &locktab[ldes];

    if(isempty(lptr->lqhead))
    {
        return SYSERR;
    }

    int highPrio = q[lptr->lqtail].qprev;

    // Get priority of highest writer process in lock's wait queue.
    int maxWritePrio = getMaxWriteProcPrio(ldes);

    // If highest waiting priority process is writer, then release the writer process from lock queue.
    if(proctab[highPrio].ltype == WRITE)
    {
        dequeue(highPrio);
        rampPriority(ldes);
        ready(highPrio, RESCHYES);

        return OK;
    }

    int prev = q[highPrio].qprev;

    while(prev != lptr->lqhead)
    {
        // If there are multiple, same high wait priority processes.
        if(q[prev].qkey == q[highPrio].qkey)
        {
            // Check for difference in wait times
            if(proctab[prev].waittime - proctab[highPrio].waittime < 1000)
            {
                // Prefer write processes over read process.
                if(proctab[prev].ltype == WRITE)
                {
                    dequeue(prev);
                    rampPriority(ldes);
                    ready(prev, RESCHYES);

                    return OK;
                }
            }
        }

        // Release the reader with the highest wait priority
        if(proctab[prev].ltype == READ)
        {
            if(q[prev].qkey >= maxWritePrio)
            {
                dequeue(prev);
                ready(prev, RESCHNO);
            }
        }
        prev = q[prev].qprev;
    }

    // Release the process with highest waiting priority from lock's wait queue and ramp the remaining priorities in lock's wait queue.
    dequeue(highPrio);
    rampPriority(ldes);
    ready(highPrio, RESCHYES);

    return OK;
}

/*--------------------------------------------------------------------------
 * releaseall  --  Release all/multiple locks held by current process
 *--------------------------------------------------------------------------
 */
int releaseall(int numlocks, int ldes)
{
    unsigned long *p;  /* Points to list of args */

    p = (unsigned long *)(&ldes) + (numlocks - 1);

    int contextLocks[numlocks];
    int isValidLocks[numlocks];

    int i;

    // Initializing temporary arrays.
    for(i = 0; i < numlocks; i++)
    {
        contextLocks[(numlocks - 1) - i] = *p;
        isValidLocks[(numlocks - 1) - i] = 0;
        p--;
    }

    struct pentry *pptr;
    struct lentry *lptr;

    pptr = &proctab[currpid];

    // Check validity of locks. (If locks are held by current process)
    for(i=0; i < numlocks; i++)
    {
        lptr = &locktab[contextLocks[i]];

        if(pptr->locks[contextLocks[i]] == 0 || isbadlock(contextLocks[i]) || lptr->lstate == LFREE)
        {
            isValidLocks[i] = 0;
        } 
        else
        {
            isValidLocks[i] = 1;
        }

    }

    // Release only valid locks.
    for(i=0; i < numlocks; i++)
    {
        if(isValidLocks[i])
        {
            lptr->lprocmap[currpid] = 0;

            pptr->locks[contextLocks[i]] = 0;
            pptr->ltype = DELETED;

            // Locks held by a reader
            if(lptr->ltype == READ)
            {
                lptr->readerCount--;

                // No readers have acquired the READ lock, then release the lock.
                if(lptr->readerCount == 0)
                {
                    int rc = releaseProcFromLockQ(contextLocks[i]);
                    
                    // If lock wait queue is empty, then mark lock as deleted.
                    if(rc == SYSERR)
                    {
                        lptr->ltype = DELETED;
                    }
                }
            }
            else
            {
                int rc = releaseProcFromLockQ(contextLocks[i]);
                    
                    // If lock wait queue is empty, then mark lock as deleted.
                    if(rc == SYSERR)
                    {
                        lptr->ltype = DELETED;
                    }
            }            
        }   
    }

    // Return SYSERR if any invalid locks are passed in parameters
    for(i=0; i < numlocks; i++)
    {
        if(isValidLocks[i] == 0)
        {
            return SYSERR;
        }
    }

    return OK;
}