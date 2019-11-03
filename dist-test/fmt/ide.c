4300 // Simple PIO-based (non-DMA) IDE driver code.
4301 
4302 #include "types.h"
4303 #include "defs.h"
4304 #include "param.h"
4305 #include "memlayout.h"
4306 #include "mmu.h"
4307 #include "proc.h"
4308 #include "x86.h"
4309 #include "traps.h"
4310 #include "spinlock.h"
4311 #include "sleeplock.h"
4312 #include "fs.h"
4313 #include "buf.h"
4314 
4315 #define SECTOR_SIZE   512
4316 #define IDE_BSY       0x80
4317 #define IDE_DRDY      0x40
4318 #define IDE_DF        0x20
4319 #define IDE_ERR       0x01
4320 
4321 #define IDE_CMD_READ  0x20
4322 #define IDE_CMD_WRITE 0x30
4323 #define IDE_CMD_RDMUL 0xc4
4324 #define IDE_CMD_WRMUL 0xc5
4325 
4326 // idequeue points to the buf now being read/written to the disk.
4327 // idequeue->qnext points to the next buf to be processed.
4328 // You must hold idelock while manipulating queue.
4329 
4330 static struct spinlock idelock;
4331 static struct buf *idequeue;
4332 
4333 static int havedisk1;
4334 static void idestart(struct buf*);
4335 
4336 // Wait for IDE disk to become ready.
4337 static int
4338 idewait(int checkerr)
4339 {
4340   int r;
4341 
4342   while(((r = inb(0x1f7)) & (IDE_BSY|IDE_DRDY)) != IDE_DRDY)
4343     ;
4344   if(checkerr && (r & (IDE_DF|IDE_ERR)) != 0)
4345     return -1;
4346   return 0;
4347 }
4348 
4349 
4350 void
4351 ideinit(void)
4352 {
4353   int i;
4354 
4355   initlock(&idelock, "ide");
4356   ioapicenable(IRQ_IDE, ncpu - 1);
4357   idewait(0);
4358 
4359   // Check if disk 1 is present
4360   outb(0x1f6, 0xe0 | (1<<4));
4361   for(i=0; i<1000; i++){
4362     if(inb(0x1f7) != 0){
4363       havedisk1 = 1;
4364       break;
4365     }
4366   }
4367 
4368   // Switch back to disk 0.
4369   outb(0x1f6, 0xe0 | (0<<4));
4370 }
4371 
4372 // Start the request for b.  Caller must hold idelock.
4373 static void
4374 idestart(struct buf *b)
4375 {
4376   if(b == 0)
4377     panic("idestart");
4378   if(b->blockno >= FSSIZE)
4379     panic("incorrect blockno");
4380   int sector_per_block =  BSIZE/SECTOR_SIZE;
4381   int sector = b->blockno * sector_per_block;
4382   int read_cmd = (sector_per_block == 1) ? IDE_CMD_READ :  IDE_CMD_RDMUL;
4383   int write_cmd = (sector_per_block == 1) ? IDE_CMD_WRITE : IDE_CMD_WRMUL;
4384 
4385   if (sector_per_block > 7) panic("idestart");
4386 
4387   idewait(0);
4388   outb(0x3f6, 0);  // generate interrupt
4389   outb(0x1f2, sector_per_block);  // number of sectors
4390   outb(0x1f3, sector & 0xff);
4391   outb(0x1f4, (sector >> 8) & 0xff);
4392   outb(0x1f5, (sector >> 16) & 0xff);
4393   outb(0x1f6, 0xe0 | ((b->dev&1)<<4) | ((sector>>24)&0x0f));
4394   if(b->flags & B_DIRTY){
4395     outb(0x1f7, write_cmd);
4396     outsl(0x1f0, b->data, BSIZE/4);
4397   } else {
4398     outb(0x1f7, read_cmd);
4399   }
4400 }
4401 
4402 // Interrupt handler.
4403 void
4404 ideintr(void)
4405 {
4406   struct buf *b;
4407 
4408   // First queued buffer is the active request.
4409   acquire(&idelock);
4410 
4411   if((b = idequeue) == 0){
4412     release(&idelock);
4413     return;
4414   }
4415   idequeue = b->qnext;
4416 
4417   // Read data if needed.
4418   if(!(b->flags & B_DIRTY) && idewait(1) >= 0)
4419     insl(0x1f0, b->data, BSIZE/4);
4420 
4421   // Wake process waiting for this buf.
4422   b->flags |= B_VALID;
4423   b->flags &= ~B_DIRTY;
4424   wakeup(b);
4425 
4426   // Start disk on next buf in queue.
4427   if(idequeue != 0)
4428     idestart(idequeue);
4429 
4430   release(&idelock);
4431 }
4432 
4433 // Sync buf with disk.
4434 // If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
4435 // Else if B_VALID is not set, read buf from disk, set B_VALID.
4436 void
4437 iderw(struct buf *b)
4438 {
4439   struct buf **pp;
4440 
4441   if(!holdingsleep(&b->lock))
4442     panic("iderw: buf not locked");
4443   if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
4444     panic("iderw: nothing to do");
4445   if(b->dev != 0 && !havedisk1)
4446     panic("iderw: ide disk 1 not present");
4447 
4448   acquire(&idelock);  //DOC:acquire-lock
4449 
4450   // Append b to idequeue.
4451   b->qnext = 0;
4452   for(pp=&idequeue; *pp; pp=&(*pp)->qnext)  //DOC:insert-queue
4453     ;
4454   *pp = b;
4455 
4456   // Start disk if necessary.
4457   if(idequeue == b)
4458     idestart(b);
4459 
4460   // Wait for request to finish.
4461   while((b->flags & (B_VALID|B_DIRTY)) != B_VALID){
4462     sleep(b, &idelock);
4463   }
4464 
4465 
4466   release(&idelock);
4467 }
4468 
4469 
4470 
4471 
4472 
4473 
4474 
4475 
4476 
4477 
4478 
4479 
4480 
4481 
4482 
4483 
4484 
4485 
4486 
4487 
4488 
4489 
4490 
4491 
4492 
4493 
4494 
4495 
4496 
4497 
4498 
4499 
