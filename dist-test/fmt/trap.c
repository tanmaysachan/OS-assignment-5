3350 #include "types.h"
3351 #include "defs.h"
3352 #include "param.h"
3353 #include "memlayout.h"
3354 #include "mmu.h"
3355 #include "proc.h"
3356 #include "x86.h"
3357 #include "traps.h"
3358 #include "spinlock.h"
3359 
3360 // Interrupt descriptor table (shared by all CPUs).
3361 struct gatedesc idt[256];
3362 extern uint vectors[];  // in vectors.S: array of 256 entry pointers
3363 struct spinlock tickslock;
3364 uint ticks;
3365 uint cnt_to_yield = 0;
3366 
3367 void
3368 tvinit(void)
3369 {
3370   int i;
3371 
3372   for(i = 0; i < 256; i++)
3373     SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0);
3374   SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER);
3375 
3376   initlock(&tickslock, "time");
3377 }
3378 
3379 void
3380 idtinit(void)
3381 {
3382   lidt(idt, sizeof(idt));
3383 }
3384 
3385 void
3386 trap(struct trapframe *tf)
3387 {
3388   if(tf->trapno == T_SYSCALL){
3389     if(myproc()->killed)
3390       exit();
3391     myproc()->tf = tf;
3392     syscall();
3393     if(myproc()->killed)
3394       exit();
3395     return;
3396   }
3397 
3398   switch(tf->trapno){
3399   case T_IRQ0 + IRQ_TIMER:
3400     if(cpuid() == 0){
3401       acquire(&tickslock);
3402       ticks++;
3403       wakeup(&ticks);
3404       release(&tickslock);
3405     }
3406     lapiceoi();
3407     break;
3408   case T_IRQ0 + IRQ_IDE:
3409     ideintr();
3410     lapiceoi();
3411     break;
3412   case T_IRQ0 + IRQ_IDE+1:
3413     // Bochs generates spurious IDE1 interrupts.
3414     break;
3415   case T_IRQ0 + IRQ_KBD:
3416     kbdintr();
3417     lapiceoi();
3418     break;
3419   case T_IRQ0 + IRQ_COM1:
3420     uartintr();
3421     lapiceoi();
3422     break;
3423   case T_IRQ0 + 7:
3424   case T_IRQ0 + IRQ_SPURIOUS:
3425     cprintf("cpu%d: spurious interrupt at %x:%x\n",
3426             cpuid(), tf->cs, tf->eip);
3427     lapiceoi();
3428     break;
3429 
3430   default:
3431     if(myproc() == 0 || (tf->cs&3) == 0){
3432       // In kernel, it must be our mistake.
3433       cprintf("unexpected trap %d from cpu %d eip %x (cr2=0x%x)\n",
3434               tf->trapno, cpuid(), tf->eip, rcr2());
3435       panic("trap");
3436     }
3437     // In user space, assume process misbehaved.
3438     cprintf("pid %d %s: trap %d err %d on cpu %d "
3439             "eip 0x%x addr 0x%x--kill proc\n",
3440             myproc()->pid, myproc()->name, tf->trapno,
3441             tf->err, cpuid(), tf->eip, rcr2());
3442     myproc()->killed = 1;
3443   }
3444 
3445   // Force process exit if it has been killed and is in user space.
3446   // (If it is still executing in the kernel, let it keep running
3447   // until it gets to the regular system call return.)
3448   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
3449     exit();
3450   // Force process to give up CPU on clock tick.
3451   // If interrupts were on while locks held, would need to check nlock.
3452   if(myproc() && myproc()->state == RUNNING &&
3453      tf->trapno == T_IRQ0+IRQ_TIMER && SCHEDFLAG[0] != 'F' && SCHEDFLAG[0] != 'M')
3454     yield();
3455 
3456   if(SCHEDFLAG[0] == 'M'){
3457     if(myproc()->cur_queue == 0){
3458       if(myproc() && myproc()->state == RUNNING &&
3459           tf->trapno == T_IRQ0+IRQ_TIMER){
3460         myproc()->slice_exhausted = 1;
3461         yield();
3462       }
3463     }
3464     else if(myproc()->cur_queue == 1){
3465       if(cnt_to_yield == 1){
3466         myproc()->slice_exhausted = 1;
3467         cnt_to_yield = 0;
3468         yield();
3469       }
3470       if(myproc() && myproc()->state == RUNNING &&
3471           tf->trapno == T_IRQ0+IRQ_TIMER)
3472         cnt_to_yield++;
3473     }
3474     else if(myproc()->cur_queue == 2){
3475       if(cnt_to_yield == 3){
3476         myproc()->slice_exhausted = 1;
3477         cnt_to_yield = 0;
3478         yield();
3479       }
3480       if(myproc() && myproc()->state == RUNNING &&
3481           tf->trapno == T_IRQ0+IRQ_TIMER)
3482         cnt_to_yield++;
3483     }
3484     else if(myproc()->cur_queue == 3){
3485       if(cnt_to_yield == 7){
3486         myproc()->slice_exhausted = 1;
3487         cnt_to_yield = 0;
3488         yield();
3489       }
3490       if(myproc() && myproc()->state == RUNNING &&
3491           tf->trapno == T_IRQ0+IRQ_TIMER)
3492         cnt_to_yield++;
3493     }
3494     else if(myproc()->cur_queue == 4){
3495       if(cnt_to_yield == 15){
3496         myproc()->slice_exhausted = 1;
3497         cnt_to_yield = 0;
3498         yield();
3499       }
3500       if(myproc() && myproc()->state == RUNNING &&
3501           tf->trapno == T_IRQ0+IRQ_TIMER)
3502         cnt_to_yield++;
3503     }
3504   }
3505 
3506   // Check if the process has been killed since we yielded
3507   if(myproc() && myproc()->killed && (tf->cs&3) == DPL_USER)
3508     exit();
3509 }
3510 
3511 
3512 
3513 
3514 
3515 
3516 
3517 
3518 
3519 
3520 
3521 
3522 
3523 
3524 
3525 
3526 
3527 
3528 
3529 
3530 
3531 
3532 
3533 
3534 
3535 
3536 
3537 
3538 
3539 
3540 
3541 
3542 
3543 
3544 
3545 
3546 
3547 
3548 
3549 
