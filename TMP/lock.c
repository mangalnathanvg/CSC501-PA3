#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <q.h>

/*-----------------------------------------------------------------------------------------------
 * getMaxWriteProcPrio  -- Get the writer process from the lock wait queue with highest priority
 *-----------------------------------------------------------------------------------------------
 */
int getMaxWriteProcPrio(int ldes1)
{
    struct lentry *lptr;

    lptr = &locktab[ldes1];

    if(isempty(lptr->lqhead))
    {
        return SYSERR;
    }

    int maxPrio = -1;

    int next = q[lptr->lqhead].qnext;

    // Traverse lock wait queue to find writer with highest waiting priority.

    while(next!=lptr->lqtail)
    {
        if(proctab[next].ltype == WRITE)
        {
            int nextProc = q[next].qkey;

            if(nextProc >= maxPrio)
            {
                maxPrio = nextProc;
            }
        }
        next = q[next].qnext;
    }

    // Return highest priority of writer process in lock wait queue.
    return maxPrio;
}

/*---------------------------------------------------------------------------------------------
 * maxPrioLockWQ  --  obtain the process with maximum priority in the wait queue of lock ldes1
 *---------------------------------------------------------------------------------------------
 */
int maxPrioLockWQ(int ldes1)
{
    struct lentry *lptr;

    lptr = &locktab[ldes1];

    if(isempty(lptr->lqhead))
    {
        return SYSERR;
    }

    int maxPrio = -1;

    int next = q[lptr->lqhead].qnext;

    while(next != lptr->lqtail)
    {
        if(proctab[next].pprio >= maxPrio)
        {
            maxPrio = proctab[next].pprio;
        }
    
        next = q[next].qnext;
    }

    return maxPrio;
}

/*------------------------------------------------------------------------
 * rampPriority  --  ramp up the priority to implement priority inheritance
 *------------------------------------------------------------------------
 */
void rampPriority(int ldes1)
{
    struct lentry *lptr;

    lptr = &locktab[ldes1];
    
    // Get the maximum priority in the lock's wait queue.
    int maxPrioLQ = maxPrioLockWQ(ldes1);
    
    int i;

    // Ramp up all the priorities in the lock wait queue also taking care of transitivity of priority inheritance condition.
    for(i = 0; i < NPROC; i++)
    {
        if(lptr->lprocmap[i])
        {
            // Update process priority to highest priority in lock wait queue.
            if(proctab[i].pinh < maxPrioLQ)
            {
                proctab[i].pprio = maxPrioLQ;
            }
            // Otherwise update to inherited priority.
            else
            {   
                proctab[i].pprio = proctab[i].pinh;
            }
            // If process is waiting for a lock, then ramp up the priorities of processes in that lock's waiting queue too.
            if(proctab[i].lockid != -1)
            {
                rampPriority(proctab[i].lockid);
            }
        }
    }

}

/*------------------------------------------------------------------------
 * blockProcess  --  Make process wait by adding to lock's wait queue
 *------------------------------------------------------------------------
 */
void blockProcess(struct lentry *lptr, struct pentry *pptr, int ldes1, int type, int priority)
{
        pptr->pstate = PRWAIT;
        pptr->ltype = type;
        pptr->lockid = ldes1;
        pptr->waittime = ctr1000;
        pptr->plockret = OK;  

        // Add process to lock's wait queue with respect to wait priority.
        insert(currpid, lptr->lqhead, priority);
}

/*------------------------------------------------------------------------
 * priorityInheritance  --  Perform priority inheritance
 *------------------------------------------------------------------------
 */
void priorityInheritance(struct lentry *lptr, int ldes1)
{
    int i;
    // Ramp priority for all processes in lock queue with priority less than that of current process.
    for(i = 0; i<NPROC; i++)
    {
        if(lptr->lprocmap[i])
        {
            if(proctab[i].pprio <= proctab[currpid].pprio)
            {
                rampPriority(ldes1);
            }
            break;
        }
    }

    // Reschedule
    resched();
}

/*------------------------------------------------------------------------
 * lock  --  Acquire a lock ldes1 for type operation with waiting priority
 *------------------------------------------------------------------------
 */
int lock(int ldes1, int type, int priority)
{
    struct lentry *lptr;
    struct pentry *pptr;

    int i;

    lptr = &locktab[ldes1];
    pptr = &proctab[currpid];

    // If lock does not exist, return SYSERR
    if(isbadlock(ldes1) || lptr->lstate == LFREE)
    {   
        return SYSERR;
    }

    // If lock exists, but not used by anyone then assign lock.
    if(lptr->ltype == DELETED)
    {
        lptr->ltype = type;
        lptr->lprocmap[currpid] = 1;
        pptr->locks[ldes1] = 1;
        pptr->plockret = OK;

        // If lock is acquired by reader, then increase reader count.
        if(type == READ)
        {
            ++lptr->readerCount;
        }

        return pptr->plockret;
    }

    // If lock is acquired and writer is requesting a lock or reader is requesting a lock acquired by a writer, then process is blocked.
    else if(type == WRITE || (type == READ && lptr->ltype == WRITE))
    {
        // Block the process
        blockProcess(lptr, pptr, ldes1, type, priority);
        
        // Priority Inheritance to eliminate priority inversion
        priorityInheritance(lptr, ldes1);

        // If lock is acquired by reader, then increase reader count.
        if(type == READ)
            ++lptr->readerCount;
        
        lptr->lprocmap[currpid] = 1;
        lptr->ltype = type;

        pptr->locks[ldes1] = 1;
        pptr->lockid = -1;

        return pptr->plockret;
    }
    // If a reader tries to acquire the lock or the lock has already been acquired by a reader, then check if its priority is 
    // higher than any writer process in the lock's wait queue.
    else if(type == READ || lptr->ltype == READ)
    {
        int maxWritePrio = getMaxWriteProcPrio(ldes1);

        if(priority < maxWritePrio)
        {
            // Block the process
            blockProcess(lptr, pptr, ldes1, type, priority);
            
            // Ramp priority for processes in the lock's wait queue.
            priorityInheritance(lptr, ldes1);

            // Increase Reader count
			++lptr->readerCount;

    		lptr->lprocmap[currpid]=1;
    		lptr->ltype = type;

			pptr->locks[ldes1]=1;
            pptr->lockid = -1;

	        
			return pptr->plockret;
        }
        else{
            
            ++lptr->readerCount;

			lptr->lprocmap[currpid] = 1;

			pptr->locks[ldes1] = 1;

			return OK;
        }
    }
}