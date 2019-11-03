6600 #include "types.h"
6601 #include "defs.h"
6602 #include "param.h"
6603 #include "mmu.h"
6604 #include "proc.h"
6605 #include "fs.h"
6606 #include "spinlock.h"
6607 #include "sleeplock.h"
6608 #include "file.h"
6609 
6610 #define PIPESIZE 512
6611 
6612 struct pipe {
6613   struct spinlock lock;
6614   char data[PIPESIZE];
6615   uint nread;     // number of bytes read
6616   uint nwrite;    // number of bytes written
6617   int readopen;   // read fd is still open
6618   int writeopen;  // write fd is still open
6619 };
6620 
6621 int
6622 pipealloc(struct file **f0, struct file **f1)
6623 {
6624   struct pipe *p;
6625 
6626   p = 0;
6627   *f0 = *f1 = 0;
6628   if((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
6629     goto bad;
6630   if((p = (struct pipe*)kalloc()) == 0)
6631     goto bad;
6632   p->readopen = 1;
6633   p->writeopen = 1;
6634   p->nwrite = 0;
6635   p->nread = 0;
6636   initlock(&p->lock, "pipe");
6637   (*f0)->type = FD_PIPE;
6638   (*f0)->readable = 1;
6639   (*f0)->writable = 0;
6640   (*f0)->pipe = p;
6641   (*f1)->type = FD_PIPE;
6642   (*f1)->readable = 0;
6643   (*f1)->writable = 1;
6644   (*f1)->pipe = p;
6645   return 0;
6646 
6647 
6648 
6649 
6650  bad:
6651   if(p)
6652     kfree((char*)p);
6653   if(*f0)
6654     fileclose(*f0);
6655   if(*f1)
6656     fileclose(*f1);
6657   return -1;
6658 }
6659 
6660 void
6661 pipeclose(struct pipe *p, int writable)
6662 {
6663   acquire(&p->lock);
6664   if(writable){
6665     p->writeopen = 0;
6666     wakeup(&p->nread);
6667   } else {
6668     p->readopen = 0;
6669     wakeup(&p->nwrite);
6670   }
6671   if(p->readopen == 0 && p->writeopen == 0){
6672     release(&p->lock);
6673     kfree((char*)p);
6674   } else
6675     release(&p->lock);
6676 }
6677 
6678 int
6679 pipewrite(struct pipe *p, char *addr, int n)
6680 {
6681   int i;
6682 
6683   acquire(&p->lock);
6684   for(i = 0; i < n; i++){
6685     while(p->nwrite == p->nread + PIPESIZE){  //DOC: pipewrite-full
6686       if(p->readopen == 0 || myproc()->killed){
6687         release(&p->lock);
6688         return -1;
6689       }
6690       wakeup(&p->nread);
6691       sleep(&p->nwrite, &p->lock);  //DOC: pipewrite-sleep
6692     }
6693     p->data[p->nwrite++ % PIPESIZE] = addr[i];
6694   }
6695   wakeup(&p->nread);  //DOC: pipewrite-wakeup1
6696   release(&p->lock);
6697   return n;
6698 }
6699 
6700 int
6701 piperead(struct pipe *p, char *addr, int n)
6702 {
6703   int i;
6704 
6705   acquire(&p->lock);
6706   while(p->nread == p->nwrite && p->writeopen){  //DOC: pipe-empty
6707     if(myproc()->killed){
6708       release(&p->lock);
6709       return -1;
6710     }
6711     sleep(&p->nread, &p->lock); //DOC: piperead-sleep
6712   }
6713   for(i = 0; i < n; i++){  //DOC: piperead-copy
6714     if(p->nread == p->nwrite)
6715       break;
6716     addr[i] = p->data[p->nread++ % PIPESIZE];
6717   }
6718   wakeup(&p->nwrite);  //DOC: piperead-wakeup
6719   release(&p->lock);
6720   return i;
6721 }
6722 
6723 
6724 
6725 
6726 
6727 
6728 
6729 
6730 
6731 
6732 
6733 
6734 
6735 
6736 
6737 
6738 
6739 
6740 
6741 
6742 
6743 
6744 
6745 
6746 
6747 
6748 
6749 
