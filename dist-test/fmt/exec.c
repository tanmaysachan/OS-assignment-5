6450 #include "types.h"
6451 #include "param.h"
6452 #include "memlayout.h"
6453 #include "mmu.h"
6454 #include "proc.h"
6455 #include "defs.h"
6456 #include "x86.h"
6457 #include "elf.h"
6458 
6459 int
6460 exec(char *path, char **argv)
6461 {
6462   char *s, *last;
6463   int i, off;
6464   uint argc, sz, sp, ustack[3+MAXARG+1];
6465   struct elfhdr elf;
6466   struct inode *ip;
6467   struct proghdr ph;
6468   pde_t *pgdir, *oldpgdir;
6469   struct proc *curproc = myproc();
6470 
6471   begin_op();
6472 
6473   if((ip = namei(path)) == 0){
6474     end_op();
6475     cprintf("exec: fail\n");
6476     return -1;
6477   }
6478   ilock(ip);
6479   pgdir = 0;
6480 
6481   // Check ELF header
6482   if(readi(ip, (char*)&elf, 0, sizeof(elf)) != sizeof(elf))
6483     goto bad;
6484   if(elf.magic != ELF_MAGIC)
6485     goto bad;
6486 
6487   if((pgdir = setupkvm()) == 0)
6488     goto bad;
6489 
6490   // Load program into memory.
6491   sz = 0;
6492   for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
6493     if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
6494       goto bad;
6495     if(ph.type != ELF_PROG_LOAD)
6496       continue;
6497     if(ph.memsz < ph.filesz)
6498       goto bad;
6499     if(ph.vaddr + ph.memsz < ph.vaddr)
6500       goto bad;
6501     if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
6502       goto bad;
6503     if(ph.vaddr % PGSIZE != 0)
6504       goto bad;
6505     if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
6506       goto bad;
6507   }
6508   iunlockput(ip);
6509   end_op();
6510   ip = 0;
6511 
6512   // Allocate two pages at the next page boundary.
6513   // Make the first inaccessible.  Use the second as the user stack.
6514   sz = PGROUNDUP(sz);
6515   if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
6516     goto bad;
6517   clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
6518   sp = sz;
6519 
6520   // Push argument strings, prepare rest of stack in ustack.
6521   for(argc = 0; argv[argc]; argc++) {
6522     if(argc >= MAXARG)
6523       goto bad;
6524     sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
6525     if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
6526       goto bad;
6527     ustack[3+argc] = sp;
6528   }
6529   ustack[3+argc] = 0;
6530 
6531   ustack[0] = 0xffffffff;  // fake return PC
6532   ustack[1] = argc;
6533   ustack[2] = sp - (argc+1)*4;  // argv pointer
6534 
6535   sp -= (3+argc+1) * 4;
6536   if(copyout(pgdir, sp, ustack, (3+argc+1)*4) < 0)
6537     goto bad;
6538 
6539   // Save program name for debugging.
6540   for(last=s=path; *s; s++)
6541     if(*s == '/')
6542       last = s+1;
6543   safestrcpy(curproc->name, last, sizeof(curproc->name));
6544 
6545   // Commit to the user image.
6546   oldpgdir = curproc->pgdir;
6547   curproc->pgdir = pgdir;
6548   curproc->sz = sz;
6549   curproc->tf->eip = elf.entry;  // main
6550   curproc->tf->esp = sp;
6551   switchuvm(curproc);
6552   freevm(oldpgdir);
6553   return 0;
6554 
6555  bad:
6556   if(pgdir)
6557     freevm(pgdir);
6558   if(ip){
6559     iunlockput(ip);
6560     end_op();
6561   }
6562   return -1;
6563 }
6564 
6565 
6566 
6567 
6568 
6569 
6570 
6571 
6572 
6573 
6574 
6575 
6576 
6577 
6578 
6579 
6580 
6581 
6582 
6583 
6584 
6585 
6586 
6587 
6588 
6589 
6590 
6591 
6592 
6593 
6594 
6595 
6596 
6597 
6598 
6599 
