4500 // Buffer cache.
4501 //
4502 // The buffer cache is a linked list of buf structures holding
4503 // cached copies of disk block contents.  Caching disk blocks
4504 // in memory reduces the number of disk reads and also provides
4505 // a synchronization point for disk blocks used by multiple processes.
4506 //
4507 // Interface:
4508 // * To get a buffer for a particular disk block, call bread.
4509 // * After changing buffer data, call bwrite to write it to disk.
4510 // * When done with the buffer, call brelse.
4511 // * Do not use the buffer after calling brelse.
4512 // * Only one process at a time can use a buffer,
4513 //     so do not keep them longer than necessary.
4514 //
4515 // The implementation uses two state flags internally:
4516 // * B_VALID: the buffer data has been read from the disk.
4517 // * B_DIRTY: the buffer data has been modified
4518 //     and needs to be written to disk.
4519 
4520 #include "types.h"
4521 #include "defs.h"
4522 #include "param.h"
4523 #include "spinlock.h"
4524 #include "sleeplock.h"
4525 #include "fs.h"
4526 #include "buf.h"
4527 
4528 struct {
4529   struct spinlock lock;
4530   struct buf buf[NBUF];
4531 
4532   // Linked list of all buffers, through prev/next.
4533   // head.next is most recently used.
4534   struct buf head;
4535 } bcache;
4536 
4537 void
4538 binit(void)
4539 {
4540   struct buf *b;
4541 
4542   initlock(&bcache.lock, "bcache");
4543 
4544   // Create linked list of buffers
4545   bcache.head.prev = &bcache.head;
4546   bcache.head.next = &bcache.head;
4547   for(b = bcache.buf; b < bcache.buf+NBUF; b++){
4548     b->next = bcache.head.next;
4549     b->prev = &bcache.head;
4550     initsleeplock(&b->lock, "buffer");
4551     bcache.head.next->prev = b;
4552     bcache.head.next = b;
4553   }
4554 }
4555 
4556 // Look through buffer cache for block on device dev.
4557 // If not found, allocate a buffer.
4558 // In either case, return locked buffer.
4559 static struct buf*
4560 bget(uint dev, uint blockno)
4561 {
4562   struct buf *b;
4563 
4564   acquire(&bcache.lock);
4565 
4566   // Is the block already cached?
4567   for(b = bcache.head.next; b != &bcache.head; b = b->next){
4568     if(b->dev == dev && b->blockno == blockno){
4569       b->refcnt++;
4570       release(&bcache.lock);
4571       acquiresleep(&b->lock);
4572       return b;
4573     }
4574   }
4575 
4576   // Not cached; recycle an unused buffer.
4577   // Even if refcnt==0, B_DIRTY indicates a buffer is in use
4578   // because log.c has modified it but not yet committed it.
4579   for(b = bcache.head.prev; b != &bcache.head; b = b->prev){
4580     if(b->refcnt == 0 && (b->flags & B_DIRTY) == 0) {
4581       b->dev = dev;
4582       b->blockno = blockno;
4583       b->flags = 0;
4584       b->refcnt = 1;
4585       release(&bcache.lock);
4586       acquiresleep(&b->lock);
4587       return b;
4588     }
4589   }
4590   panic("bget: no buffers");
4591 }
4592 
4593 
4594 
4595 
4596 
4597 
4598 
4599 
4600 // Return a locked buf with the contents of the indicated block.
4601 struct buf*
4602 bread(uint dev, uint blockno)
4603 {
4604   struct buf *b;
4605 
4606   b = bget(dev, blockno);
4607   if((b->flags & B_VALID) == 0) {
4608     iderw(b);
4609   }
4610   return b;
4611 }
4612 
4613 // Write b's contents to disk.  Must be locked.
4614 void
4615 bwrite(struct buf *b)
4616 {
4617   if(!holdingsleep(&b->lock))
4618     panic("bwrite");
4619   b->flags |= B_DIRTY;
4620   iderw(b);
4621 }
4622 
4623 // Release a locked buffer.
4624 // Move to the head of the MRU list.
4625 void
4626 brelse(struct buf *b)
4627 {
4628   if(!holdingsleep(&b->lock))
4629     panic("brelse");
4630 
4631   releasesleep(&b->lock);
4632 
4633   acquire(&bcache.lock);
4634   b->refcnt--;
4635   if (b->refcnt == 0) {
4636     // no one is waiting for it.
4637     b->next->prev = b->prev;
4638     b->prev->next = b->next;
4639     b->next = bcache.head.next;
4640     b->prev = &bcache.head;
4641     bcache.head.next->prev = b;
4642     bcache.head.next = b;
4643   }
4644 
4645   release(&bcache.lock);
4646 }
4647 // Blank page.
4648 
4649 
