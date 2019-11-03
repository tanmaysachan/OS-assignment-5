2000 // Per-CPU state
2001 struct cpu {
2002   uchar apicid;                // Local APIC ID
2003   struct context *scheduler;   // swtch() here to enter scheduler
2004   struct taskstate ts;         // Used by x86 to find stack for interrupt
2005   struct segdesc gdt[NSEGS];   // x86 global descriptor table
2006   volatile uint started;       // Has the CPU started?
2007   int ncli;                    // Depth of pushcli nesting.
2008   int intena;                  // Were interrupts enabled before pushcli?
2009   struct proc *proc;           // The process running on this cpu or null
2010 };
2011 
2012 extern struct cpu cpus[NCPU];
2013 extern int ncpu;
2014 
2015 // Saved registers for kernel context switches.
2016 // Don't need to save all the segment registers (%cs, etc),
2017 // because they are constant across kernel contexts.
2018 // Don't need to save %eax, %ecx, %edx, because the
2019 // x86 convention is that the caller has saved them.
2020 // Contexts are stored at the bottom of the stack they
2021 // describe; the stack pointer is the address of the context.
2022 // The layout of the context matches the layout of the stack in swtch.S
2023 // at the "Switch stacks" comment. Switch doesn't save eip explicitly,
2024 // but it is on the stack and allocproc() manipulates it.
2025 struct context {
2026   uint edi;
2027   uint esi;
2028   uint ebx;
2029   uint ebp;
2030   uint eip;
2031 };
2032 
2033 enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };
2034 
2035 // Per-process state
2036 struct proc {
2037   uint sz;                     // Size of process memory (bytes)
2038   pde_t* pgdir;                // Page table
2039   char *kstack;                // Bottom of kernel stack for this process
2040   enum procstate state;        // Process state
2041   int pid;                     // Process ID
2042   struct proc *parent;         // Parent process
2043   struct trapframe *tf;        // Trap frame for current syscall
2044   struct context *context;     // swtch() here to run process
2045   void *chan;                  // If non-zero, sleeping on chan
2046   int killed;                  // If non-zero, have been killed
2047   struct file *ofile[NOFILE];  // Open files
2048   struct inode *cwd;           // Current directory
2049   char name[16];               // Process name (debugging)
2050   uint ctime;                  // Process creation time
2051   uint etime;                  // Process end time
2052   uint rtime;                  // Total process run time
2053   uint last_check;             // Time when the process's times were last updated
2054   uint ltime;                  // Time when the process was last running
2055   int priority;                // Scheduling priority of the process
2056   int cur_queue;               // Current queue the process is in for MLFQ scheduling
2057   uint slice_exhausted;        // Tells us if the process consumed the time slice
2058 };
2059 
2060 // Process memory is laid out contiguously, low addresses first:
2061 //   text
2062 //   original data and bss
2063 //   fixed-size stack
2064 //   expandable heap
2065 
2066 
2067 
2068 
2069 
2070 
2071 
2072 
2073 
2074 
2075 
2076 
2077 
2078 
2079 
2080 
2081 
2082 
2083 
2084 
2085 
2086 
2087 
2088 
2089 
2090 
2091 
2092 
2093 
2094 
2095 
2096 
2097 
2098 
2099 
