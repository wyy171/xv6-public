#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->priority = 3;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  acquire(&tickslock);
  p->ctime = ticks;  // Initialize creation time
  release(&tickslock);
  

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  if (np->pid == 0) {
      np->ctime = ticks; // Record creation time
  }

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;
  
  acquire(&tickslock);
  curproc->etime = ticks; // Record end time
  curproc->rtime = curproc->etime - curproc->ctime; // Calculate total time
  release(&tickslock);
  
  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
  /* if (myproc()->pid == p->pid) {
     acquire(&tickslock);
    p->etime = ticks; // Record end time
    p->rtime = p->etime - p->ctime; // Calculate total time
     release(&tickslock);
  } */
  
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int
waitx(int *ctime, int *etime)  
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
  
        // Copy process time information 
        *ctime = p->ctime;
        *etime = p->etime;
        
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;

      
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int ps() {

    struct proc *p;

    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process with pid.
    acquire(&ptable.lock);
    cprintf("name \t pid \t state \t \t CREATE_TIME \t RUNTIME \t PRIORITY\n");
  //PID number, process status (running, zombie, wait, etc.), start time, total time, and process name.
    
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    {
        if(p->state == SLEEPING){
          cprintf("%s \t %d \t SLEEPING \t %d \t\t %d  \t %d \n",p->name, p->pid, p->ctime, p->rtime, p->priority);
        }
        else if(p->state == RUNNABLE){
          cprintf("%s \t %d \t RUNNABLE \t %d \t\t %d  \t %d \n",p->name, p->pid, p->ctime, p->rtime, p->priority);
        }
        else if(p->state == RUNNING){
          cprintf("%s \t %d \t RUNNING \t %d \t\t %d  \t %d \n",p->name, p->pid, p->ctime, p->rtime, p->priority);
        }
        else if(p->state == ZOMBIE){
          cprintf("%s \t %d \t ZOMBIE \t %d \t\t %d  \t %d \n",p->name, p->pid, p->ctime, p->rtime, p->priority);
        }
    }

    release(&ptable.lock);

    return 25;
}

// Change priority
int
setpr(int pid, int priority)
{
    struct proc *p;

    acquire(&ptable.lock);
    acquire(&tickslock);
    for(p=ptable.proc; p<&ptable.proc[NPROC]; p++)
    {
        if(p->pid == pid)
        {
            p->priority = priority;
            break;
        }
    }
    release(&ptable.lock);
    release(&tickslock);

    return pid;
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

void
scheduler(void)
{
  struct proc *p, *p1;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();
    struct proc *highP;
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      highP = p;
      //choose one with highest priority
      for(p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++){
	if(p1->state != RUNNABLE)
	  continue;
	if(highP->priority > p1->priority)   //larger value, lower priority
	  highP = p1;
      }
      p = highP;
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}

/*
void
scheduler(void) //works
{
  struct proc *p;
  struct proc *p1;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();
    struct proc *highP;
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      highP = p;
      for(p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++){
        if(p1->state != RUNNABLE)
          continue;
        if(highP->priority > p1->priority)
          highP = p1;
      }

      p = highP; 
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);

  }
}
*/
/* Kai
void
scheduler(void)
{
  struct cpu *c = mycpu();
  c->proc = 0;

  //cprintf("%s", "\n");
  //cprintf("%s", "out of for(;;) loop \n");
  //cprintf("%s", "\n");

  for(;;){

    //cprintf("%s", "inside of for(;;) loop \n");

    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    #ifdef DEFAULT
    // cprintf("%s","DEFAULT Running...");
    struct proc *p;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;
    //   Switch to chosen process.  It is the process's job
    //   to release ptable.lock and then reacquire it
    //   before jumping back to us.

    c->proc = p;
    switchuvm(p);
    p->state = RUNNING;

    uint sched_time = time_scheduled(p->pid);
    if (sched_time == -1)
    {
        //print error if we want
        p->rtime = 0;
    }

    p->ctime = p->ctime + p->rtime;

    // uint ct = p->ctime;
    // uint rt = p->rtime;
    // int pppid = p -> pid;

    p->rtime = 0;

    //cprintf("DEFAULT: The start time of current process is %d \n", p->ctime);

    swtch(&(c->scheduler), p->context);
    switchkvm();

    acquire(&tickslock);
    p->rtime = ticks;
    release(&tickslock);

    // cprintf("Process ID: %d -- ", pppid);
    // cprintf("Run time: %d -- ", rt);
    // cprintf("Total run time: %d\n", ct);


    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;
    
    }

    #else

    #ifdef FIFO
    // cprintf("%s","FIFO Running...");
    struct proc* mst = 0, * p = 0;
    // go through process table
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state != RUNNABLE )
            continue;

        // ignore init and sh processes from FCFS
        if(mst != 0){
            // here I find the process with the lowest creation time (the first one that was created)
            if(mst->start >= p->start)
            {
                mst = p;
            }
        }
        else
        {
           mst = p;
        }
    }  

    if(mst != 0){
        // Switch process
        //cprintf("FIFO: The start time of current process is %d \n", mst->start);
        c->proc = mst;
        switchuvm(mst);
        mst->state = RUNNING;

        acquire(&tickslock);
        p->rtime = ticks;
        release(&tickslock);

        uint sched_time = time_scheduled(p->pid);
        if (sched_time == -1)
        {
            //print error if we want
            p->rtime = 0;
        }
        p->ctime = p->ctime + p->rtime;

        // p->stime = 0;

        // uint ct = p->ctime;
        // uint rt = p->rtime;
        // // uint st = p->stime;
        // int pppid = p -> pid;

        // cprintf("cpu %d, pname %s, pid %d, rtime %d\n", c->apicid, mst->name, mst->pid, mst->rtime);
        swtch(&(c->scheduler), mst->context);
        switchkvm();

        cprintf("Process ID: %d -- ", p->pid);
        // cprintf("Start time: %d -- ", p->stime);
        // cprintf("Concurrent time: %d -- ", ct);
        cprintf("Run time: %d -- ", p->rtime);


        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        // release(&ptable.lock);
    }
    #else

    #ifdef PRIORITY
    struct proc* maxprio;
    struct proc* p, * r;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch process
      maxprio = p;
      for (r = ptable.proc; r < &ptable.proc[NPROC]; r++) {
          if (r->state != RUNNABLE)
              continue;
          //get the highest value of priority
          if (r->priority <= maxprio->priority)
              maxprio = r;
      }
      p = maxprio;
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;

      uint sched_time = time_scheduled(p->pid);
      if (sched_time == -1)
      {
        //print error if we want
        p->rtime = 0;
      }

      p->ctime = p->ctime + p->rtime;

      uint ct = p->ctime;
      uint rt = p->rtime;
      int pppid = p -> pid;


      p->rtime = 0;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      acquire(&tickslock);
      p->rtime = ticks;
      release(&tickslock);

      cprintf("Process ID: %d -- ", pppid);
      cprintf("Run time: %d -- ", rt);
      cprintf("Total run time: %d\n", ct);


      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }

    #endif
    #endif
    #endif

    release(&ptable.lock);

  }
}

*/
/*
void
scheduler(void)
{
  struct proc *p = 0;
  struct cpu *c = mycpu();
  c->proc = 0;
 
  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

      //------- DEFAULT Round-Robin scheduler  --------
      #ifdef DEFAULT
        if(p->state != RUNNABLE)
          continue;
      //-------   end default scheduler -----------------

      #else
      #ifdef FCFS
      //--------   FCFS SCHEDULER START   -------------
        // struct proc *minP = 0;

        if(p->state != RUNNABLE)
          continue;

        // ignore init and sh processes from FCFS
        if(p->pid > 2)
        {
          if (minP != 0){
            // here I find the process with the lowest creation time (the first one that was created)
            if(p->ctime < minP->ctime){
              minP = p;
            }
          }
          else{
            minP = p;
          }
        }

        // If I found the process which I created first and it is runnable I run it
        //(in the real FCFS I should not check if it is runnable, but for testing purposes I have to make this control, otherwise every time I launch
        // a process which does I/0 operation (every simple command) everything will be blocked
        if(minP != 0 && minP->state == RUNNABLE){
          p = minP;
        }

      // --------------   FCFS SCHEDULER END   ---------------
      #else
      #ifdef PRIORITY
      // -----------   PRIORITY SCEHDULER START ---------------
        // struct proc *highP = 0;
        // struct proc *p1 = 0;

        if(p->state != RUNNABLE)
          continue;
        // Choose the process with highest priority (among RUNNABLEs)
        highP = p;
        for(p1 = ptable.proc; p1 < &ptable.proc[NPROC]; p1++){
          if((p1->state == RUNNABLE) && (highP->priority > p1->priority))
            highP = p1;
        }

        if(highP != 0){
          p = highP;
        }
      // -----------   PRIORITY SCHEDULER END -----------------
      #endif
      #endif
      #endif

      if(p != 0){
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;

        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
    }
    release(&ptable.lock);

  }
}
*/

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      
      acquire(&tickslock);
      p->etime = ticks;
      release(&tickslock);
      
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

void
uniq(void) {
    cprintf("Uniq command is getting executed in kernel mode");
}

void
head(void) {
   cprintf("Head command is getting executed in kernel mode");
}
