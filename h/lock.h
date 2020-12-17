/* lock.h - isbadlock */

#ifndef _LOCK_H_
#define _LOCK_H_

#ifndef	NLOCKS
#define	NLOCKS		50	/* number of locks, if not defined	*/
#endif

#define	LFREE       0		/* this lock is free		*/
#define	LUSED	    1		/* this lock is used		*/
       
#define READ        2
#define WRITE       3

struct	lentry	{		/* lock table entry		*/
	int  lstate;		/* the state LFREE or LUSED		*/
    int  ltype;         /* type of lock (READ, WRITE, DELETED) */
	int	 lqhead;		/* q index of head of list		*/
	int	 lqtail;		/* q index of tail of list		*/
    int  readerCount;       /* Count of readers using the same lock */
    int  lprocmap[NPROC]  /* processes that have acquired this lock */ 
};
extern	struct	lentry	locktab[];
extern	int	nextlock;
extern unsigned long ctr1000;

#define	isbadlock(l)	(l<0 || l>=NLOCKS)

#endif
