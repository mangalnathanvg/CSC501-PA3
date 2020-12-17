/* lcreate.c - lcreate, newlock */

#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <q.h>
#include <lock.h>
#include <stdio.h>

LOCAL int newlock();

/*------------------------------------------------------------------------
 * lcreate  --  create and initialize a lock, returning its id
 *------------------------------------------------------------------------
 */
SYSCALL lcreate()
{
	STATWORD ps;   
	disable(ps);
	
	int	lock;

	if ((lock=newlock())==SYSERR ) {
		restore(ps);
		return(SYSERR);
	}

	restore(ps);
	return lock;

}

/*------------------------------------------------------------------------
 * newlock  --  allocate a free lock and return its index
 *------------------------------------------------------------------------
 */
LOCAL int newlock()
{
	int	lock;
	int	i;

	for (i=0 ; i<NLOCKS ; i++) {
		lock=nextlock--;
		if (nextlock < 0)
			nextlock = NLOCKS-1;
		if (locktab[lock].lstate==LFREE) {
			locktab[lock].lstate = LUSED;
			return(lock);
		}
	}
	return(SYSERR);
}
