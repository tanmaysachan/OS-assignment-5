2100 #include "types.h"
2101 #include "defs.h"
2102 #include "param.h"
2103 #include "memlayout.h"
2104 #include "mmu.h"
2105 #include "x86.h"
2106 #include "proc.h"
2107 #include "spinlock.h"
2108 
2109 struct {
2110   struct spinlock lock;
2111   struct proc proc[NPROC];
2112 } ptable;
2113 
2114 struct proc *queue_0[NPROC];
2115 struct proc *queue_1[NPROC];
2116 struct proc *queue_2[NPROC];
2117 struct proc *queue_3[NPROC];
2118 struct proc *queue_4[NPROC];
2119 
2120 int heads[5] = {0, 0, 0, 0, 0};
2121 int tails[5] = {0, 0, 0, 0, 0};
2122 int sizes[5] = {0, 0, 0, 0, 0};
2123 
2124 static struct proc *initproc;
2125 
2126 int nextpid = 1;
2127 extern void forkret(void);
2128 extern void trapret(void);
2129 
2130 static void wakeup1(void *chan);
2131 
2132 void
2133 add_to_queue(struct proc *p, int qno)
2134 {
2135   struct proc **queue;
2136   switch(qno){
2137     case 0:
2138       queue = queue_0;
2139       break;
2140     case 1:
2141       queue = queue_1;
2142       break;
2143     case 2:
2144       queue = queue_2;
2145       break;
2146     case 3:
2147       queue = queue_3;
2148       break;
2149     case 4:
2150       queue = queue_4;
2151       break;
2152     default:
2153       panic("invalid queue requested");
2154   }
2155   queue[tails[qno]] = p;
2156   tails[qno] = (tails[qno] + 1) % NPROC;
2157   sizes[qno]++;
2158   p->cur_queue = qno;
2159 }
2160 
2161 void
2162 pop_queue(int qno){
2163   heads[qno] = (heads[qno] + 1) % NPROC;
2164   sizes[qno]--;
2165 }
2166 
2167 void
2168 remove_from_queue(struct proc *p)
2169 {
2170   struct proc **queue;
2171   int qno = p->cur_queue;
2172   switch(qno){
2173     case 0:
2174       queue = queue_0;
2175       break;
2176     case 1:
2177       queue = queue_1;
2178       break;
2179     case 2:
2180       queue = queue_2;
2181       break;
2182     case 3:
2183       queue = queue_3;
2184       break;
2185     case 4:
2186       queue = queue_4;
2187       break;
2188     default:
2189       panic("invalid queue requested");
2190   }
2191 
2192   int i = heads[qno];
2193   struct proc *tmp;
2194   for(; i != tails[qno]; i = (i+1)%NPROC){
2195     tmp = queue[i];
2196     if(tmp->pid == p->pid){
2197       queue[i] = 0;
2198       sizes[qno]--;
2199       break;
2200     }
2201   }
2202   clean_queue(qno);
2203 }
2204 
2205 void
2206 clean_queue(int qno)
2207 {
2208   struct proc **queue;
2209   switch(qno){
2210     case 0:
2211       queue = queue_0;
2212       break;
2213     case 1:
2214       queue = queue_1;
2215       break;
2216     case 2:
2217       queue = queue_2;
2218       break;
2219     case 3:
2220       queue = queue_3;
2221       break;
2222     case 4:
2223       queue = queue_4;
2224       break;
2225     default:
2226       panic("invalid queue requested");
2227   }
2228 
2229   int i;
2230   for(i = heads[qno]; i != tails[qno]; i = (i+1)%NPROC){
2231     if(queue[i] == 0){
2232       int j;
2233       struct proc *tmp;
2234       struct proc *old;
2235       tmp = queue[heads[qno]];
2236       for(j = heads[qno]; j != i; j = (j+1)%NPROC){
2237         old = queue[(j+1)%NPROC];
2238         queue[(j+1)%NPROC] = tmp;
2239         tmp = old;
2240       }
2241       heads[qno]++;
2242     }
2243   }
2244 }
2245 
2246 
2247 
2248 
2249 
2250 void
2251 pinit(void)
2252 {
2253   initlock(&ptable.lock, "ptable");
2254 }
2255 
2256 // Must be called with interrupts disabled
2257 int
2258 cpuid() {
2259   return mycpu()-cpus;
2260 }
2261 
2262 // Must be called with interrupts disabled to avoid the caller being
2263 // rescheduled between reading lapicid and running through the loop.
2264 struct cpu*
2265 mycpu(void)
2266 {
2267   int apicid, i;
2268 
2269   if(readeflags()&FL_IF)
2270     panic("mycpu called with interrupts enabled\n");
2271 
2272   apicid = lapicid();
2273   // APIC IDs are not guaranteed to be contiguous. Maybe we should have
2274   // a reverse map, or reserve a register to store &cpus[i].
2275   for (i = 0; i < ncpu; ++i) {
2276     if (cpus[i].apicid == apicid)
2277       return &cpus[i];
2278   }
2279   panic("unknown apicid\n");
2280 }
2281 
2282 // Disable interrupts so that we are not rescheduled
2283 // while reading proc from the cpu structure
2284 struct proc*
2285 myproc(void) {
2286   struct cpu *c;
2287   struct proc *p;
2288   pushcli();
2289   c = mycpu();
2290   p = c->proc;
2291   popcli();
2292   return p;
2293 }
2294 
2295 
2296 
2297 
2298 
2299 
2300 // Look in the process table for an UNUSED proc.
2301 // If found, change state to EMBRYO and initialize
2302 // state required to run in the kernel.
2303 // Otherwise return 0.
2304 static struct proc*
2305 allocproc(void)
2306 {
2307   struct proc *p;
2308   char *sp;
2309 
2310   acquire(&ptable.lock);
2311 
2312   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2313     if(p->state == UNUSED)
2314       goto found;
2315 
2316   release(&ptable.lock);
2317   return 0;
2318 
2319 found:
2320   p->state = EMBRYO;
2321   p->pid = nextpid++;
2322   p->ctime = ticks;
2323   p->rtime = 0;
2324   p->last_check = ticks;
2325   p->priority = 60;
2326 
2327   release(&ptable.lock);
2328 
2329   // Allocate kernel stack.
2330   if((p->kstack = kalloc()) == 0){
2331     p->state = UNUSED;
2332     return 0;
2333   }
2334   sp = p->kstack + KSTACKSIZE;
2335 
2336   // Leave room for trap frame.
2337   sp -= sizeof *p->tf;
2338   p->tf = (struct trapframe*)sp;
2339 
2340   // Set up new context to start executing at forkret,
2341   // which returns to trapret.
2342   sp -= 4;
2343   *(uint*)sp = (uint)trapret;
2344 
2345   sp -= sizeof *p->context;
2346   p->context = (struct context*)sp;
2347   memset(p->context, 0, sizeof *p->context);
2348   p->context->eip = (uint)forkret;
2349 
2350   return p;
2351 }
2352 
2353 // Set up first user process.
2354 void
2355 userinit(void)
2356 {
2357   struct proc *p;
2358   extern char _binary_initcode_start[], _binary_initcode_size[];
2359 
2360   p = allocproc();
2361 
2362   initproc = p;
2363   if((p->pgdir = setupkvm()) == 0)
2364     panic("userinit: out of memory?");
2365   inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
2366   p->sz = PGSIZE;
2367   memset(p->tf, 0, sizeof(*p->tf));
2368   p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
2369   p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
2370   p->tf->es = p->tf->ds;
2371   p->tf->ss = p->tf->ds;
2372   p->tf->eflags = FL_IF;
2373   p->tf->esp = PGSIZE;
2374   p->tf->eip = 0;  // beginning of initcode.S
2375 
2376   safestrcpy(p->name, "initcode", sizeof(p->name));
2377   p->cwd = namei("/");
2378 
2379   // this assignment to p->state lets other cores
2380   // run this process. the acquire forces the above
2381   // writes to be visible, and the lock is also needed
2382   // because the assignment might not be atomic.
2383   acquire(&ptable.lock);
2384 
2385   p->state = RUNNABLE;
2386   p->cur_queue = 0;
2387   add_to_queue(p, 0);
2388   p->ltime = ticks;
2389 
2390   release(&ptable.lock);
2391 }
2392 
2393 
2394 
2395 
2396 
2397 
2398 
2399 
2400 // Grow current process's memory by n bytes.
2401 // Return 0 on success, -1 on failure.
2402 int
2403 growproc(int n)
2404 {
2405   uint sz;
2406   struct proc *curproc = myproc();
2407 
2408   sz = curproc->sz;
2409   if(n > 0){
2410     if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
2411       return -1;
2412   } else if(n < 0){
2413     if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
2414       return -1;
2415   }
2416   curproc->sz = sz;
2417   switchuvm(curproc);
2418   return 0;
2419 }
2420 
2421 // Create a new process copying p as the parent.
2422 // Sets up stack to return as if from system call.
2423 // Caller must set state of returned proc to RUNNABLE.
2424 int
2425 fork(void)
2426 {
2427   int i, pid;
2428   struct proc *np;
2429   struct proc *curproc = myproc();
2430 
2431   // Allocate process.
2432   if((np = allocproc()) == 0){
2433     return -1;
2434   }
2435 
2436   // Copy process state from proc.
2437   if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
2438     kfree(np->kstack);
2439     np->kstack = 0;
2440     np->state = UNUSED;
2441     return -1;
2442   }
2443   np->sz = curproc->sz;
2444   np->parent = curproc;
2445   *np->tf = *curproc->tf;
2446 
2447   // Clear %eax so that fork returns 0 in the child.
2448   np->tf->eax = 0;
2449 
2450   for(i = 0; i < NOFILE; i++)
2451     if(curproc->ofile[i])
2452       np->ofile[i] = filedup(curproc->ofile[i]);
2453   np->cwd = idup(curproc->cwd);
2454 
2455   safestrcpy(np->name, curproc->name, sizeof(curproc->name));
2456 
2457   pid = np->pid;
2458 
2459   acquire(&ptable.lock);
2460 
2461   np->state = RUNNABLE;
2462   add_to_queue(np, 0);
2463   np->cur_queue = 0;
2464   np->ltime = ticks;
2465 
2466   release(&ptable.lock);
2467 
2468   return pid;
2469 }
2470 
2471 // Exit the current process.  Does not return.
2472 // An exited process remains in the zombie state
2473 // until its parent calls wait() to find out it exited.
2474 void
2475 exit(void)
2476 {
2477   struct proc *curproc = myproc();
2478   struct proc *p;
2479   int fd;
2480 
2481   if(curproc == initproc)
2482     panic("init exiting");
2483 
2484   // Close all open files.
2485   for(fd = 0; fd < NOFILE; fd++){
2486     if(curproc->ofile[fd]){
2487       fileclose(curproc->ofile[fd]);
2488       curproc->ofile[fd] = 0;
2489     }
2490   }
2491 
2492   begin_op();
2493   iput(curproc->cwd);
2494   end_op();
2495   curproc->cwd = 0;
2496 
2497   acquire(&ptable.lock);
2498 
2499 
2500   // Parent might be sleeping in wait().
2501   wakeup1(curproc->parent);
2502 
2503   // Pass abandoned children to init.
2504   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2505     if(p->parent == curproc){
2506       p->parent = initproc;
2507       if(p->state == ZOMBIE)
2508         wakeup1(initproc);
2509     }
2510   }
2511 
2512   // Jump into the scheduler, never to return.
2513   curproc->state = ZOMBIE;
2514   curproc->etime = ticks;
2515   sched();
2516   panic("zombie exit");
2517 }
2518 
2519 // Wait for a child process to exit and return its pid.
2520 // Return -1 if this process has no children.
2521 int
2522 wait(void)
2523 {
2524   struct proc *p;
2525   int havekids, pid;
2526   struct proc *curproc = myproc();
2527 
2528   acquire(&ptable.lock);
2529   for(;;){
2530     // Scan through table looking for exited children.
2531     havekids = 0;
2532     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2533       if(p->parent != curproc)
2534         continue;
2535       havekids = 1;
2536       if(p->state == ZOMBIE){
2537         // Found one.
2538         pid = p->pid;
2539         kfree(p->kstack);
2540         p->kstack = 0;
2541         freevm(p->pgdir);
2542         p->pid = 0;
2543         p->parent = 0;
2544         p->name[0] = 0;
2545         p->killed = 0;
2546         p->state = UNUSED;
2547         p->etime = ticks;
2548         release(&ptable.lock);
2549         return pid;
2550       }
2551     }
2552 
2553     // No point waiting if we don't have any children.
2554     if(!havekids || curproc->killed){
2555       release(&ptable.lock);
2556       return -1;
2557     }
2558 
2559     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
2560     sleep(curproc, &ptable.lock);  //DOC: wait-sleep
2561   }
2562 }
2563 
2564 int
2565 waitx(int* wtime, int* rtime)
2566 {
2567   struct proc *curproc = myproc();
2568   struct proc *p;
2569   int havekids, pid;
2570 
2571   acquire(&ptable.lock);
2572   for(;;){
2573     // Scan through table looking for exited children.
2574     havekids = 0;
2575     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2576       if(p->parent != curproc)
2577         continue;
2578       havekids = 1;
2579       if(p->state == ZOMBIE){
2580         // Found one.
2581         pid = p->pid;
2582         kfree(p->kstack);
2583         p->kstack = 0;
2584         freevm(p->pgdir);
2585         p->pid = 0;
2586         p->parent = 0;
2587         p->name[0] = 0;
2588         p->killed = 0;
2589         p->state = UNUSED;
2590         p->etime = ticks;
2591         release(&ptable.lock);
2592         *rtime = p->rtime;
2593         *wtime = ticks - p->ctime - p->rtime;
2594         return pid;
2595       }
2596     }
2597 
2598 
2599 
2600     // No point waiting if we don't have any children.
2601     if(!havekids || curproc->killed){
2602       release(&ptable.lock);
2603       *rtime = -1;
2604       *wtime = -1;
2605       return -1;
2606     }
2607 
2608     // Wait for children to exit.  (See wakeup1 call in proc_exit.)
2609     sleep(curproc, &ptable.lock);  //DOC: wait-sleep
2610   }
2611 }
2612 
2613 int
2614 setpriority(int priority)
2615 {
2616   struct proc *curproc = myproc();
2617   if(priority > 100 || priority < 0)
2618     return -1;
2619   int old_p = curproc->priority;
2620   curproc->priority = priority;
2621   if(priority < old_p){
2622     yield();
2623   }
2624   return old_p;
2625 }
2626 
2627 // update process rtime and last_check time
2628 void
2629 update_times(struct proc* p)
2630 {
2631   if(p->state == RUNNING)
2632     p->rtime += ticks - p->last_check;
2633   p->last_check = ticks;
2634 }
2635 
2636 
2637 
2638 
2639 
2640 
2641 
2642 
2643 
2644 
2645 
2646 
2647 
2648 
2649 
2650 // Per-CPU process scheduler.
2651 // Each CPU calls scheduler() after setting itself up.
2652 // Scheduler never returns.  It loops, doing:
2653 //  - choose a process to run
2654 //  - swtch to start running that process
2655 //  - eventually that process transfers control
2656 //      via swtch back to the scheduler.
2657 void
2658 scheduler(void)
2659 {
2660   struct proc *p;
2661   struct cpu *c = mycpu();
2662   c->proc = 0;
2663 
2664   int scheduler_strat = 4;
2665   if(SCHEDFLAG[0] == 'F'){
2666     scheduler_strat = 1;
2667   }
2668   else if(SCHEDFLAG[0] == 'P'){
2669     scheduler_strat = 2;
2670   }
2671   else if(SCHEDFLAG[0] == 'M'){
2672     scheduler_strat = 3;
2673   }
2674 
2675   for(;;){
2676     // Enable interrupts on this processor.
2677     sti();
2678 
2679     // Loop over process table looking for process to run.
2680     acquire(&ptable.lock);
2681 
2682     switch(scheduler_strat){
2683 
2684       case 1:{
2685         // FCFS scheduler
2686         struct proc *to_run = 0;
2687         int min_ctime = ticks + 1000;
2688         for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2689           update_times(p);
2690           if(p->state != RUNNABLE)
2691             continue;
2692 
2693           if(p->ctime < min_ctime){
2694             min_ctime = p->ctime;
2695             to_run = p;
2696           }
2697         }
2698         if(to_run){
2699           c->proc = to_run;
2700           switchuvm(to_run);
2701           to_run->state = RUNNING;
2702 
2703           swtch(&(c->scheduler), to_run->context);
2704           switchkvm();
2705 
2706           c->proc = 0;
2707         }
2708       }
2709       break;
2710 
2711       case 2:{
2712         // Priority based scheduler
2713         int priority_chosen = 101;
2714         for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2715           update_times(p);
2716           if(p->state != RUNNABLE)
2717             continue;
2718           if(p->priority < priority_chosen)
2719             priority_chosen = p->priority;
2720         }
2721         for(p = ptable.proc; p < &ptable.proc[NPROC] && priority_chosen != 101; p++){
2722           update_times(p);
2723           if(p->state != RUNNABLE)
2724             continue;
2725           if(p->priority == priority_chosen){
2726             c->proc = p;
2727             switchuvm(p);
2728             p->state = RUNNING;
2729 
2730             swtch(&(c->scheduler), p->context);
2731             switchkvm();
2732 
2733             c->proc = 0;
2734           }
2735         }
2736       }
2737       break;
2738 
2739       case 3:{
2740         // MLFQ scheduler
2741         struct proc *to_run = 0;
2742         if(sizes[0] != 0){
2743           to_run = queue_0[heads[0]];
2744           pop_queue(0);
2745         }
2746         else if(sizes[1] != 0){
2747           to_run = queue_1[heads[1]];
2748           pop_queue(1);
2749         }
2750         else if(sizes[2] != 0){
2751           to_run = queue_2[heads[2]];
2752           pop_queue(2);
2753         }
2754         else if(sizes[3] != 0){
2755           to_run = queue_3[heads[3]];
2756           pop_queue(3);
2757         }
2758         else if(sizes[4] != 0){
2759           to_run = queue_4[heads[4]];
2760           pop_queue(4);
2761         }
2762         if(to_run != 0){
2763           while(1){
2764             c->proc = to_run;
2765             switchuvm(to_run);
2766             to_run->state = RUNNING;
2767             to_run->ltime = ticks;
2768 
2769             swtch(&(c->scheduler), to_run->context);
2770             switchkvm();
2771 
2772             c->proc = 0;
2773 
2774             if(to_run->state == RUNNABLE && !to_run->slice_exhausted){
2775               continue;
2776             }
2777             if(to_run->state == RUNNABLE && to_run->slice_exhausted){
2778               if(to_run->cur_queue != 4){
2779                 to_run->cur_queue++;
2780                 add_to_queue(to_run, to_run->cur_queue+1);
2781               }
2782               else{
2783                 add_to_queue(to_run, to_run->cur_queue);
2784               }
2785               break;
2786             }
2787           }
2788         }
2789         for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2790           update_times(p);
2791           if(p->state != RUNNABLE)
2792             continue;
2793 
2794           if(ticks - p->ltime > 50 && p->cur_queue != 0){
2795             p->ltime = ticks;
2796             remove_from_queue(p);
2797             add_to_queue(p, p->cur_queue-1);
2798           }
2799         }
2800       }
2801       break;
2802 
2803       case 4:{
2804         // original round-robin based scheduler
2805         for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2806           update_times(p);
2807           if(p->state != RUNNABLE)
2808             continue;
2809 
2810           // Switch to chosen process.  It is the process's job
2811           // to release ptable.lock and then reacquire it
2812           // before jumping back to us.
2813           c->proc = p;
2814           switchuvm(p);
2815           p->state = RUNNING;
2816 
2817           swtch(&(c->scheduler), p->context);
2818           switchkvm();
2819 
2820           // Process is done running for now.
2821           // It should have changed its p->state before coming back.
2822           c->proc = 0;
2823         }
2824       }
2825       break;
2826       default:
2827         panic("invalid scheduling");
2828     }
2829     release(&ptable.lock);
2830   }
2831 }
2832 
2833 // Enter scheduler.  Must hold only ptable.lock
2834 // and have changed proc->state. Saves and restores
2835 // intena because intena is a property of this
2836 // kernel thread, not this CPU. It should
2837 // be proc->intena and proc->ncli, but that would
2838 // break in the few places where a lock is held but
2839 // there's no process.
2840 void
2841 sched(void)
2842 {
2843   int intena;
2844   struct proc *p = myproc();
2845 
2846   if(!holding(&ptable.lock))
2847     panic("sched ptable.lock");
2848   if(mycpu()->ncli != 1)
2849     panic("sched locks");
2850   if(p->state == RUNNING)
2851     panic("sched running");
2852   if(readeflags()&FL_IF)
2853     panic("sched interruptible");
2854   intena = mycpu()->intena;
2855   swtch(&p->context, mycpu()->scheduler);
2856   mycpu()->intena = intena;
2857 }
2858 
2859 // Give up the CPU for one scheduling round.
2860 void
2861 yield(void)
2862 {
2863   acquire(&ptable.lock);  //DOC: yieldlock
2864   myproc()->state = RUNNABLE;
2865   sched();
2866   release(&ptable.lock);
2867 }
2868 
2869 // A fork child's very first scheduling by scheduler()
2870 // will swtch here.  "Return" to user space.
2871 void
2872 forkret(void)
2873 {
2874   static int first = 1;
2875   // Still holding ptable.lock from scheduler.
2876   release(&ptable.lock);
2877 
2878   if (first) {
2879     // Some initialization functions must be run in the context
2880     // of a regular process (e.g., they call sleep), and thus cannot
2881     // be run from main().
2882     first = 0;
2883     iinit(ROOTDEV);
2884     initlog(ROOTDEV);
2885   }
2886 
2887   // Return to "caller", actually trapret (see allocproc).
2888 }
2889 
2890 
2891 
2892 
2893 
2894 
2895 
2896 
2897 
2898 
2899 
2900 // Atomically release lock and sleep on chan.
2901 // Reacquires lock when awakened.
2902 void
2903 sleep(void *chan, struct spinlock *lk)
2904 {
2905   struct proc *p = myproc();
2906 
2907   if(p == 0)
2908     panic("sleep");
2909 
2910   if(lk == 0)
2911     panic("sleep without lk");
2912 
2913   // Must acquire ptable.lock in order to
2914   // change p->state and then call sched.
2915   // Once we hold ptable.lock, we can be
2916   // guaranteed that we won't miss any wakeup
2917   // (wakeup runs with ptable.lock locked),
2918   // so it's okay to release lk.
2919   if(lk != &ptable.lock){  //DOC: sleeplock0
2920     acquire(&ptable.lock);  //DOC: sleeplock1
2921     release(lk);
2922   }
2923   // Go to sleep.
2924   p->chan = chan;
2925   p->state = SLEEPING;
2926   remove_from_queue(p);
2927 
2928   sched();
2929 
2930   // Tidy up.
2931   p->chan = 0;
2932 
2933   // Reacquire original lock.
2934   if(lk != &ptable.lock){  //DOC: sleeplock2
2935     release(&ptable.lock);
2936     acquire(lk);
2937   }
2938 }
2939 
2940 
2941 
2942 
2943 
2944 
2945 
2946 
2947 
2948 
2949 
2950 // Wake up all processes sleeping on chan.
2951 // The ptable lock must be held.
2952 static void
2953 wakeup1(void *chan)
2954 {
2955   struct proc *p;
2956 
2957   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
2958     if(p->state == SLEEPING && p->chan == chan){
2959       p->state = RUNNABLE;
2960       add_to_queue(p, p->cur_queue);
2961     }
2962 }
2963 
2964 // Wake up all processes sleeping on chan.
2965 void
2966 wakeup(void *chan)
2967 {
2968   acquire(&ptable.lock);
2969   wakeup1(chan);
2970   release(&ptable.lock);
2971 }
2972 
2973 // Kill the process with the given pid.
2974 // Process won't exit until it returns
2975 // to user space (see trap in trap.c).
2976 int
2977 kill(int pid)
2978 {
2979   struct proc *p;
2980 
2981   acquire(&ptable.lock);
2982   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
2983     if(p->pid == pid){
2984       p->killed = 1;
2985       // Wake process from sleep if necessary.
2986       if(p->state == SLEEPING)
2987         p->state = RUNNABLE;
2988       release(&ptable.lock);
2989       return 0;
2990     }
2991   }
2992   release(&ptable.lock);
2993   return -1;
2994 }
2995 
2996 
2997 
2998 
2999 
3000 // Print a process listing to console.  For debugging.
3001 // Runs when user types ^P on console.
3002 // No lock to avoid wedging a stuck machine further.
3003 void
3004 procdump(void)
3005 {
3006   static char *states[] = {
3007   [UNUSED]    "unused",
3008   [EMBRYO]    "embryo",
3009   [SLEEPING]  "sleep ",
3010   [RUNNABLE]  "runble",
3011   [RUNNING]   "run   ",
3012   [ZOMBIE]    "zombie"
3013   };
3014   int i;
3015   struct proc *p;
3016   char *state;
3017   uint pc[10];
3018 
3019   for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
3020     if(p->state == UNUSED)
3021       continue;
3022     if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
3023       state = states[p->state];
3024     else
3025       state = "???";
3026     cprintf("%d %s %s", p->pid, state, p->name);
3027     if(p->state == SLEEPING){
3028       getcallerpcs((uint*)p->context->ebp+2, pc);
3029       for(i=0; i<10 && pc[i] != 0; i++)
3030         cprintf(" %p", pc[i]);
3031     }
3032     cprintf("\n");
3033   }
3034 }
3035 
3036 
3037 
3038 
3039 
3040 
3041 
3042 
3043 
3044 
3045 
3046 
3047 
3048 
3049 
