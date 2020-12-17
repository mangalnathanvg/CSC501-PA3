#include <conf.h>
#include <kernel.h>
#include <proc.h>
#include <lock.h>
#include <stdio.h>
#include <lock.h>

#define DEFAULT_LOCK_PRIO 20

#define assert(x,error) if(!(x)){ \
            kprintf(error);\
            return;\
            }

/*----------------------------------Test Semaphore---------------------------*/
void readersem (char *msg, int sem)
{
        kprintf ("  %s: to acquire semaphore\n", msg);
        wait(sem);
        kprintf ("  %s: acquired semaphore\n", msg);
        kprintf ("  %s: to release semaphore\n", msg);
        signal(sem);
}

void writersem (char *msg, int sem)
{
        kprintf ("  %s: to acquire semaphore\n", msg);
        wait(sem);
        kprintf ("  %s: acquired semaphore, sleep 10s\n", msg);
        sleep (10);
        kprintf ("  %s: to release semaphore\n", msg);
        signal(sem);
}

void testsemaphore ()
{
        int     sem;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nSemaphore Test : test the basic priority inheritence\n");
        sem  = screate (1);
        assert (sem != SYSERR, "Test for Semaphore failed");

        rd1 = create(readersem, 2000, 40, "readersem", 2, "reader A", sem);
        rd2 = create(readersem, 2000, 50, "readersem", 2, "reader B", sem);
        wr1 = create(writersem, 2000, 30, "writersem", 2, "writer", sem);

        kprintf("-start writer, then sleep 1s. semaphore granted to write (prio 30)\n");
        resume(wr1);
        sleep (1);

        kprintf("-start reader A, then sleep 1s. reader A(prio 40) blocked on the semaphore\n");
        resume(rd1);
        sleep (1);
	    
        assert (getprio(wr1) == 40, "Test for Semaphore failed");

        kprintf("-start reader B, then sleep 1s. reader B(prio 50) blocked on the lock\n");
        resume (rd2);
	    sleep (1);
	    
        assert (getprio(wr1) == 50, "Test for Semaphore failed");
	
	kprintf("-kill reader B, then sleep 1s\n");
	kill (rd2);
	sleep (1);
	assert (getprio(wr1) == 40, "Test for Semaphore failed");

	kprintf("-kill reader A, then sleep 1s\n");
	kill (rd1);
	sleep(1);
	assert(getprio(wr1) == 30, "Test for Semaphore failed");

        sleep (8);
        kprintf ("Test Test for Semaphore OK\n");
}

/*----------------------------------Test Lock--------------------------*/
void readerlock (char *msg, int lck)
{
        int     ret;

        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, READ, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock\n", msg);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void writerlock (char *msg, int lck)
{
        kprintf ("  %s: to acquire lock\n", msg);
        lock (lck, WRITE, DEFAULT_LOCK_PRIO);
        kprintf ("  %s: acquired lock, sleep 10s\n", msg);
        sleep (10);
        kprintf ("  %s: to release lock\n", msg);
        releaseall (1, lck);
}

void testlock ()
{
        int     lck;
        int     rd1, rd2;
        int     wr1;

        kprintf("\nLock Test: test the basic priority inheritence\n");
        lck  = lcreate ();
        assert (lck != SYSERR, "Test for Lock failed");

        rd1 = create(readerlock, 2000, 40, "readerlock", 2, "reader A", lck);
        rd2 = create(readerlock, 2000, 50, "readerlock", 2, "reader B", lck);
        wr1 = create(writerlock, 2000, 30, "writerlock", 2, "writer", lck);

        kprintf("-start writer, then sleep 1s. lock granted to write (prio 30)\n");
        resume(wr1);
        sleep (1);

        kprintf("-start reader A, then sleep 1s. reader A(prio 40) blocked on the lock\n");
        resume(rd1);
        sleep (1);
	assert (getprio(wr1) == 40, "Test for Lock failed");

        kprintf("-start reader B, then sleep 1s. reader B(prio 50) blocked on the lock\n");
        resume (rd2);
	sleep (1);
	assert (getprio(wr1) == 50, "Test for Lock failed");
	
	kprintf("-kill reader B, then sleep 1s\n");
	kill (rd2);
	sleep (1);
	assert (getprio(wr1) == 40, "Test for Lock failed");

	kprintf("-kill reader A, then sleep 1s\n");
	kill (rd1);
	sleep(1);
	assert(getprio(wr1) == 30, "Test for Lock failed");

        sleep (8);
        kprintf ("Test for Lock OK\n");
}


int task1( )
{
    kprintf("\nTesting semaphores of Original XINU Implementation");
	testsemaphore();

    kprintf("\n\nTesting locks of PA3 implementation");
	testlock();
}



