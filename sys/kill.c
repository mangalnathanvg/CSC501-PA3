/* kill.c - kill */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <sem.h>
#include <mem.h>
#include <io.h>
#include <q.h>
#include <stdio.h>
#include <lock.h>
/*------------------------------------------------------------------------
 * kill  --  kill a process and remove it from the system
 *------------------------------------------------------------------------
 */
SYSCALL kill(int pid)
{
	STATWORD ps;    
	struct	pentry	*pptr;		/* points to proc. table for pid*/
	int	dev, i;

	disable(ps);
	if (isbadpid(pid) || (pptr= &proctab[pid])->pstate==PRFREE) {
		restore(ps);
		return(SYSERR);
	}
	if (--numproc == 0)
		xdone();

	dev = pptr->pdevs[0];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->pdevs[1];
	if (! isbaddev(dev) )
		close(dev);
	dev = pptr->ppagedev;
	if (! isbaddev(dev) )
		close(dev);
	
	send(pptr->pnxtkin, pid);

	freestk(pptr->pbase, pptr->pstklen);
	switch (pptr->pstate) {

	case PRCURR:	pptr->pstate = PRFREE;	/* suicide */
					// Release all the locks held by the process
					for(i=0 ;i<NLOCKS;i++)
					{
						if(pptr->locks[i] == 1)
						{
							releaseall(1, i);
						}
					}
					
					resched();

	case PRWAIT:	semaph[pptr->psem].semcnt++;
					// Remove process from lock's wait queue and ramp up priorities of remaining process in the lock's wait queue.
					if(pptr->lockid >= 0)
					{
						dequeue(pid);
						rampPriority(pptr->lockid);
					}

	case PRREADY:	
			// Release all the locks held by the process
			for(i=0; i < NLOCKS; i++)
			{
				if(pptr->locks[i] == 1)
				{
					releaseall(1, i);
				}
			}
			dequeue(pid);
			pptr->pstate = PRFREE;
			break;

	case PRSLEEP:
	case PRTRECV:	unsleep(pid);
						/* fall through	*/
	default:	pptr->pstate = PRFREE;
	}
	restore(ps);
	return(OK);
}
