// scheduler.cc 
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would 
//	end up calling FindNextToRun(), and that would put us in an 
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1996 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "debug.h"
#include "scheduler.h"
#include "main.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads.
//	Initially, no ready threads.
//----------------------------------------------------------------------


//<TODO>
// Declare sorting rule of SortedList
// Hint: Funtion Type should be "static int"
int SJFcmp (Thread* a, Thread* b) {
    if (a->getPredictedBurstTime() == b->getPredictedBurstTime()) {
        if (a->getID() < b->getID()) return -1;
        else if (a->getID() > b->getID()) return 1;
        else return 0;
    }
    else if (a->getPredictedBurstTime() < b->getPredictedBurstTime()) {return -1;}
    else {return 1;}
}
//<TODO>


//<TODO>
// Initialize ReadyQueue
Scheduler::Scheduler()
{
    // schedulerType = type;
	// readyList = new List<Thread*>;
    setPreviousBT(0);
    readyQueue = new SortedList<Thread *>(SJFcmp);
    // lastBTime = 0;
	toBeDestroyed = NULL;
}
//<TODO>

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

//<TODO>
// Remove readyQueue
Scheduler::~Scheduler()
{ 
    // delete readyList; 
    delete readyQueue;
} 
//<TODO>

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

//<TODO>
// Hint: readyQueue is preemptive SJF(Shortest Job First).
// When putting a new thread into readyQueue, you need to check whether preemption or not.
void
Scheduler::ReadyToRun (Thread *thread)
{
	ASSERT(kernel->interrupt->getLevel() == IntOff);
    // victoria
    DEBUG(dbgSJF, "Ready to run" << kernel->currentThread->getID() << "->" << thread->getID());
    
    if (kernel->currentThread->getID() == 0) {
        thread->setPredictedBurstTime(0);
        DEBUG(dbgSJF, "[" <<  kernel->currentThread->getID() << "] init ");
    }
    else {
        thread->setPredictedBurstTime(0.5 * BurstTime + 0.5 * PreviousBurstTime);
        kernel->scheduler->setPreviousBT(thread->getPredictedBurstTime());
        // DEBUG(dbgSJF, "[" << lastThread->getID() << "] current thread T/start/end " << lastThread->getT() << " / " << lastThread->getstartTime() << " / " << lastThread->getendTime());
        DEBUG(dbgSJF, "[" << thread->getID() << "] : predicted burst time = " << thread->getPredictedBurstTime());
        DEBUG(dbgSJF, "previous burst time = " << PreviousBurstTime);
        DEBUG(dbgSJF, "burst time = " << BurstTime);
    }
    

    // DEBUG(dbgSJF, "Preempppppp : " << kernel->currentThread->getPredictedBurstTime() << " , " << thread->getPredictedBurstTime());

   if (kernel->currentThread->getID() != 0 && kernel->currentThread->getID() != thread->getID() && SJFcmp(thread, kernel->currentThread) < 0) {
        DEBUG(dbgSJF, "preempt happens : " << kernel->currentThread->getID() << " -> " << thread->getID());
        kernel->currentThread->setendTime(kernel->stats->totalTicks);
        // this->lastThread = kernel->currentThread;
        DEBUG(dbgSJF, "[" << kernel->currentThread->getID() << "] PREEMPT setendTime: " << kernel->stats->totalTicks);
        kernel->scheduler->ReadyToRun(kernel->currentThread);
        kernel->scheduler->Run(thread, FALSE);
    }
    else {
        DEBUG(dbgThread, "Putting thread on ready list: " << thread->getName());
        DEBUG(dbgSJF, "Putting thread on ready list: " << thread->getID());
        thread->setStatus(READY);
        readyQueue->Insert(thread);
    }
}
//<TODO>

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

//<TODO>
// a.k.a. Find Next (Thread in ReadyQueue) to Run
Thread *
Scheduler::FindNextToRun ()
{
	ASSERT(kernel->interrupt->getLevel() == IntOff);

	if (readyQueue->IsEmpty()) {
		return NULL;
	}
	else {
		return readyQueue->RemoveFront();
	}
}
//<TODO>

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable kernel->currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//	"finishing" is set if the current thread is to be deleted
//		once we're no longer running on its stack
//		(when the next thread starts running)
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread, bool finishing)
{
    

    Thread *oldThread = kernel->currentThread;
    // DEBUG(dbgSJF, "Run old: " << oldThread->getID() << " new: " << nextThread->getID());
	// cout << "Current Thread" <<oldThread->getName() << "    Next Thread"<<nextThread->getName()<<endl;
   
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    if (finishing) {	// mark that we need to delete current thread
         ASSERT(toBeDestroyed == NULL);
	     toBeDestroyed = oldThread;
    }
   
#ifdef USER_PROGRAM			// ignore until running user programs 
    if (oldThread->space != NULL) {	// if this thread is a user program,

        oldThread->SaveUserState(); 	// save the user's CPU registers
	    oldThread->space->SaveState();
    }
#endif
    
    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    kernel->currentThread = nextThread;  // switch to the next thread
    nextThread->setStatus(RUNNING);      // nextThread is now running
    
    // DEBUG(dbgThread, "Switching from: " << oldThread->getName() << " to: " << nextThread->getName());
    
    // This is a machine-dependent assembly language routine defined 
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    cout << "Switching from: " << oldThread->getID() << " to: " << nextThread->getID() << endl;
    nextThread->setstartTime(kernel->stats->totalTicks);
    DEBUG(dbgSJF, "[" << nextThread->getID() << "]" << " RUN setstartTime: " << kernel->stats->totalTicks);
    SWITCH(oldThread, nextThread);    

    // we're back, running oldThread
      
    // interrupts are off when we return from switch!
    ASSERT(kernel->interrupt->getLevel() == IntOff);

    DEBUG(dbgThread, "Now in thread: " << kernel->currentThread->getID());
    DEBUG(dbgSJF, "Now in thread: " << kernel->currentThread->getID());

    CheckToBeDestroyed();		// check if thread we were running
					// before this one has finished
					// and needs to be cleaned up
    
#ifdef USER_PROGRAM
    if (oldThread->space != NULL) {	    // if there is an address space
        oldThread->RestoreUserState();     // to restore, do it.
	    oldThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::CheckToBeDestroyed
// 	If the old thread gave up the processor because it was finishing,
// 	we need to delete its carcass.  Note we cannot delete the thread
// 	before now (for example, in Thread::Finish()), because up to this
// 	point, we were still running on the old thread's stack!
//----------------------------------------------------------------------

void
Scheduler::CheckToBeDestroyed()
{
    if (toBeDestroyed != NULL) {
        DEBUG(dbgThread, "toBeDestroyed->getID(): " << toBeDestroyed->getID());
        delete toBeDestroyed;
	    toBeDestroyed = NULL;
    }
}
 
//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------

//<TODO>
void
Scheduler::Print()
{
    cout << "Ready list contents:\n";
    readyQueue->Apply(ThreadPrint);
}
//<TODO>

//<TODO>
//Function definition of sorting rule of readyQueue

// <TODO>