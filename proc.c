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

struct proc *queue_0[NPROC];
struct proc *queue_1[NPROC];
struct proc *queue_2[NPROC];
struct proc *queue_3[NPROC];
struct proc *queue_4[NPROC];

int heads[5] = {0, 0, 0, 0, 0};
int tails[5] = {0, 0, 0, 0, 0};
int sizes[5] = {0, 0, 0, 0, 0};

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
add_to_queue(struct proc *p, int qno)
{
  struct proc **queue;
  switch(qno){
    case 0:
      queue = queue_0;
      break;
    case 1:
      queue = queue_1;
      break;
    case 2:
      queue = queue_2;
      break;
    case 3:
      queue = queue_3;
      break;
    case 4:
      queue = queue_4;
      break;
    default:
      panic("invalid queue requested in add_to_queue");
  }
  queue[tails[qno]] = p;
  tails[qno] = (tails[qno] + 1) % NPROC;
  sizes[qno]++;
  p->cur_queue = qno;
  cprintf("Process with pid %d and name \"%s\" entered queue %d\n", p->pid, p->name, p->cur_queue);
}

void
pop_queue(int qno){
  heads[qno] = (heads[qno] + 1) % NPROC;
  sizes[qno]--;
}

void
remove_from_queue(struct proc *p)
{
  struct proc **queue;
  int qno = p->cur_queue;
  switch(qno){
    case 0:
      queue = queue_0;
      break;
    case 1:
      queue = queue_1;
      break;
    case 2:
      queue = queue_2;
      break;
    case 3:
      queue = queue_3;
      break;
    case 4:
      queue = queue_4;
      break;
    default:
      panic("invalid queue requested in remove_from_queue");
  }

  int i = heads[qno];
  struct proc *tmp;
  for(; i != tails[qno]; i = (i+1)%NPROC){
    tmp = queue[i];
    if(tmp->pid == p->pid){
  cprintf("Process with pid %d and name \"%s\" removed from queue %d\n", p->pid, p->name, p->cur_queue);
      queue[i] = 0;
      sizes[qno]--;
      break;
    }
  }
  clean_queue(qno);
}

void
clean_queue(int qno)
{
  struct proc **queue;
  switch(qno){
    case 0:
      queue = queue_0;
      break;
    case 1:
      queue = queue_1;
      break;
    case 2:
      queue = queue_2;
      break;
    case 3:
      queue = queue_3;
      break;
    case 4:
      queue = queue_4;
      break;
    default:
      panic("invalid queue requested in clean_queue");
  }

  int i;
  for(i = heads[qno]; i != tails[qno]; i = (i+1)%NPROC){
    if(queue[i] == 0){
      int j;
      struct proc *tmp;
      struct proc *old;
      tmp = queue[heads[qno]];
      for(j = heads[qno]; j != i; j = (j+1)%NPROC){
        old = queue[(j+1)%NPROC];
        queue[(j+1)%NPROC] = tmp;
        tmp = old;
      }
      heads[qno]++;
    }
  }
  cprintf("Queue %d has been cleaned\n", qno);
}

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
  p->ctime = ticks;
  p->rtime = 0;
  p->last_check = ticks;
  p->priority = 60;

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
  p->cur_queue = 0;
  add_to_queue(p, 0);
  cprintf("lol added to queue\n");
  p->ltime = ticks;

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
  add_to_queue(np, 0);
  np->cur_queue = 0;
  np->ltime = ticks;

  release(&ptable.lock);

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
  curproc->etime = ticks;
  sched();
  panic("zombie exit");
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
        p->etime = ticks;
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
waitx(int* wtime, int* rtime)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int havekids, pid;

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
        p->etime = ticks;
        release(&ptable.lock);
        *rtime = p->rtime;
        *wtime = ticks - p->ctime - p->rtime;
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      *rtime = -1;
      *wtime = -1;
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int
setpriority(int priority)
{
  struct proc *curproc = myproc();
  if(priority > 100 || priority < 0)
    return -1;
  int old_p = curproc->priority;
  curproc->priority = priority;
  if(priority < old_p){
    yield();
  }
  return old_p;
}

// update process rtime and last_check time
void
update_times(struct proc* p)
{
  if(p->state == RUNNING)
    p->rtime += ticks - p->last_check;
  p->last_check = ticks;
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
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;

  int scheduler_strat = 4;
  if(SCHEDFLAG[0] == 'F'){
    scheduler_strat = 1;
  }
  else if(SCHEDFLAG[0] == 'P'){
    scheduler_strat = 2;
  }
  else if(SCHEDFLAG[0] == 'M'){
    scheduler_strat = 3;
  }

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);

    switch(scheduler_strat){

      case 1:{
        // FCFS scheduler
        struct proc *to_run = 0;
        int min_ctime = ticks + 1000;
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
          update_times(p);
          if(p->state != RUNNABLE)
            continue;
          
          if(p->ctime < min_ctime){
            min_ctime = p->ctime;
            to_run = p;
          }
        }
        if(to_run){
          c->proc = to_run;
          switchuvm(to_run);
          to_run->state = RUNNING;

          swtch(&(c->scheduler), to_run->context);
          switchkvm();

          c->proc = 0;
        }
      }
      break;

      case 2:{
        // Priority based scheduler
        int priority_chosen = 101;
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
          update_times(p);
          if(p->state != RUNNABLE)
            continue;
          if(p->priority < priority_chosen)
            priority_chosen = p->priority;
        }
        for(p = ptable.proc; p < &ptable.proc[NPROC] && priority_chosen != 101; p++){
          update_times(p);
          if(p->state != RUNNABLE)
            continue;
          if(p->priority == priority_chosen){
            c->proc = p;
            switchuvm(p);
            p->state = RUNNING;

            swtch(&(c->scheduler), p->context);
            switchkvm();

            c->proc = 0;
          }
        }
      }
      break;

      case 3:{
        // MLFQ scheduler
        struct proc *to_run = 0;
        if(sizes[0] != 0){
          to_run = queue_0[heads[0]];
          pop_queue(0);
        }
        else if(sizes[1] != 0){
          to_run = queue_1[heads[1]];
          pop_queue(1);
        }
        else if(sizes[2] != 0){
          to_run = queue_2[heads[2]];
          pop_queue(2);
        }
        else if(sizes[3] != 0){
          to_run = queue_3[heads[3]];
          pop_queue(3);
        }
        else if(sizes[4] != 0){
          to_run = queue_4[heads[4]];
          pop_queue(4);
        }
  /* cprintf("Process with pid %d and name \"%s\" was selected to be run from queue%d\n", to_run->pid, to_run->name, to_run->cur_queue); */
        if(to_run != 0){
          while(1){
            c->proc = to_run;
            switchuvm(to_run);
            to_run->state = RUNNING;
            to_run->ltime = ticks;

            swtch(&(c->scheduler), to_run->context);
            switchkvm();

            c->proc = 0;

            if(to_run->state == RUNNABLE && !to_run->slice_exhausted){
              continue;
            }
            if(to_run->state == RUNNABLE && to_run->slice_exhausted){
              if(to_run->cur_queue != 4){
                add_to_queue(to_run, to_run->cur_queue+1);
              }
              else{
                add_to_queue(to_run, to_run->cur_queue);
              }
              break;
            }
            if(to_run->state != RUNNABLE){
              break;
            }
          }
        }
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
          update_times(p);
          if(p->state != RUNNABLE)
            continue;

          if(ticks - p->ltime > 50 && p->cur_queue != 0){
            p->ltime = ticks;
            remove_from_queue(p);
            add_to_queue(p, p->cur_queue-1);
          }
        }
      }
      break;

      case 4:{
        // original round-robin based scheduler
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
          update_times(p);
          if(p->state != RUNNABLE)
            continue;

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
      break;
      default:
        panic("invalid scheduling");
    }
    release(&ptable.lock);
  }
}

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
  remove_from_queue(p);

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
    if(p->state == SLEEPING && p->chan == chan){
      p->state = RUNNABLE;
      add_to_queue(p, p->cur_queue);
    }
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
