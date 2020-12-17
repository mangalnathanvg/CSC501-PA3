/* ldelete.c - ldelete */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

/*------------------------------------------------------------------------
 * ldelete  --  delete a lock by releasing its table entry
 *------------------------------------------------------------------------
 */
SYSCALL ldelete(int lock)
{
	STATWORD ps;    
	int	pid;
	struct	lentry	*lptr;

	lptr = &locktab[lock];
	disable(ps);
	if (isbadlock(lock) || lptr->lstate==LFREE) {
		restore(ps);
		return(SYSERR);
	}
	lptr->lstate = LFREE;
	if (nonempty(lptr->lqhead)) {
		while( (pid=getfirst(lptr->lqhead)) != EMPTY)
		  {
		    proctab[pid].plockret = DELETED;
		    ready(pid,RESCHNO);
		  }
		resched();
	}
	restore(ps);
	return(OK);
}
