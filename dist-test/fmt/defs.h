0250 struct buf;
0251 struct context;
0252 struct file;
0253 struct inode;
0254 struct pipe;
0255 struct proc;
0256 struct rtcdate;
0257 struct spinlock;
0258 struct sleeplock;
0259 struct stat;
0260 struct superblock;
0261 
0262 // bio.c
0263 void            binit(void);
0264 struct buf*     bread(uint, uint);
0265 void            brelse(struct buf*);
0266 void            bwrite(struct buf*);
0267 
0268 // console.c
0269 void            consoleinit(void);
0270 void            cprintf(char*, ...);
0271 void            consoleintr(int(*)(void));
0272 void            panic(char*) __attribute__((noreturn));
0273 
0274 // exec.c
0275 int             exec(char*, char**);
0276 
0277 // file.c
0278 struct file*    filealloc(void);
0279 void            fileclose(struct file*);
0280 struct file*    filedup(struct file*);
0281 void            fileinit(void);
0282 int             fileread(struct file*, char*, int n);
0283 int             filestat(struct file*, struct stat*);
0284 int             filewrite(struct file*, char*, int n);
0285 
0286 // fs.c
0287 void            readsb(int dev, struct superblock *sb);
0288 int             dirlink(struct inode*, char*, uint);
0289 struct inode*   dirlookup(struct inode*, char*, uint*);
0290 struct inode*   ialloc(uint, short);
0291 struct inode*   idup(struct inode*);
0292 void            iinit(int dev);
0293 void            ilock(struct inode*);
0294 void            iput(struct inode*);
0295 void            iunlock(struct inode*);
0296 void            iunlockput(struct inode*);
0297 void            iupdate(struct inode*);
0298 int             namecmp(const char*, const char*);
0299 struct inode*   namei(char*);
0300 struct inode*   nameiparent(char*, char*);
0301 int             readi(struct inode*, char*, uint, uint);
0302 void            stati(struct inode*, struct stat*);
0303 int             writei(struct inode*, char*, uint, uint);
0304 
0305 // ide.c
0306 void            ideinit(void);
0307 void            ideintr(void);
0308 void            iderw(struct buf*);
0309 
0310 // ioapic.c
0311 void            ioapicenable(int irq, int cpu);
0312 extern uchar    ioapicid;
0313 void            ioapicinit(void);
0314 
0315 // kalloc.c
0316 char*           kalloc(void);
0317 void            kfree(char*);
0318 void            kinit1(void*, void*);
0319 void            kinit2(void*, void*);
0320 
0321 // kbd.c
0322 void            kbdintr(void);
0323 
0324 // lapic.c
0325 void            cmostime(struct rtcdate *r);
0326 int             lapicid(void);
0327 extern volatile uint*    lapic;
0328 void            lapiceoi(void);
0329 void            lapicinit(void);
0330 void            lapicstartap(uchar, uint);
0331 void            microdelay(int);
0332 
0333 // log.c
0334 void            initlog(int dev);
0335 void            log_write(struct buf*);
0336 void            begin_op();
0337 void            end_op();
0338 
0339 // mp.c
0340 extern int      ismp;
0341 void            mpinit(void);
0342 
0343 // picirq.c
0344 void            picenable(int);
0345 void            picinit(void);
0346 
0347 
0348 
0349 
0350 // pipe.c
0351 int             pipealloc(struct file**, struct file**);
0352 void            pipeclose(struct pipe*, int);
0353 int             piperead(struct pipe*, char*, int);
0354 int             pipewrite(struct pipe*, char*, int);
0355 
0356 // proc.c
0357 int             cpuid(void);
0358 void            exit(void);
0359 int             fork(void);
0360 int             growproc(int);
0361 int             kill(int);
0362 struct cpu*     mycpu(void);
0363 struct proc*    myproc();
0364 void            update_times(struct proc*);
0365 void            add_to_queue(struct proc*, int);
0366 void            pop_queue(int);
0367 void            remove_from_queue(struct proc*);
0368 void            clean_queue(int);
0369 void            pinit(void);
0370 void            procdump(void);
0371 void            scheduler(void) __attribute__((noreturn));
0372 void            sched(void);
0373 void            setproc(struct proc*);
0374 void            sleep(void*, struct spinlock*);
0375 void            userinit(void);
0376 int             wait(void);
0377 int             waitx(int*, int*);
0378 int             setpriority(int);
0379 void            update_rtime(void);
0380 void            wakeup(void*);
0381 void            yield(void);
0382 
0383 // swtch.S
0384 void            swtch(struct context**, struct context*);
0385 
0386 // spinlock.c
0387 void            acquire(struct spinlock*);
0388 void            getcallerpcs(void*, uint*);
0389 int             holding(struct spinlock*);
0390 void            initlock(struct spinlock*, char*);
0391 void            release(struct spinlock*);
0392 void            pushcli(void);
0393 void            popcli(void);
0394 
0395 // sleeplock.c
0396 void            acquiresleep(struct sleeplock*);
0397 void            releasesleep(struct sleeplock*);
0398 int             holdingsleep(struct sleeplock*);
0399 void            initsleeplock(struct sleeplock*, char*);
0400 // string.c
0401 int             memcmp(const void*, const void*, uint);
0402 void*           memmove(void*, const void*, uint);
0403 void*           memset(void*, int, uint);
0404 char*           safestrcpy(char*, const char*, int);
0405 int             strlen(const char*);
0406 int             strncmp(const char*, const char*, uint);
0407 char*           strncpy(char*, const char*, int);
0408 
0409 // syscall.c
0410 int             argint(int, int*);
0411 int             argptr(int, char**, int);
0412 int             argstr(int, char**);
0413 int             fetchint(uint, int*);
0414 int             fetchstr(uint, char**);
0415 void            syscall(void);
0416 
0417 // timer.c
0418 void            timerinit(void);
0419 
0420 // trap.c
0421 void            idtinit(void);
0422 extern uint     ticks;
0423 void            tvinit(void);
0424 extern struct spinlock tickslock;
0425 
0426 // uart.c
0427 void            uartinit(void);
0428 void            uartintr(void);
0429 void            uartputc(int);
0430 
0431 // vm.c
0432 void            seginit(void);
0433 void            kvmalloc(void);
0434 pde_t*          setupkvm(void);
0435 char*           uva2ka(pde_t*, char*);
0436 int             allocuvm(pde_t*, uint, uint);
0437 int             deallocuvm(pde_t*, uint, uint);
0438 void            freevm(pde_t*);
0439 void            inituvm(pde_t*, char*, uint);
0440 int             loaduvm(pde_t*, char*, struct inode*, uint, uint);
0441 pde_t*          copyuvm(pde_t*, uint);
0442 void            switchuvm(struct proc*);
0443 void            switchkvm(void);
0444 int             copyout(pde_t*, uint, void*, uint);
0445 void            clearpteu(pde_t *pgdir, char *uva);
0446 
0447 // number of elements in fixed-size array
0448 #define NELEM(x) (sizeof(x)/sizeof((x)[0]))
0449 
