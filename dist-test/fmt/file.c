5750 //
5751 // File descriptors
5752 //
5753 
5754 #include "types.h"
5755 #include "defs.h"
5756 #include "param.h"
5757 #include "fs.h"
5758 #include "spinlock.h"
5759 #include "sleeplock.h"
5760 #include "file.h"
5761 
5762 struct devsw devsw[NDEV];
5763 struct {
5764   struct spinlock lock;
5765   struct file file[NFILE];
5766 } ftable;
5767 
5768 void
5769 fileinit(void)
5770 {
5771   initlock(&ftable.lock, "ftable");
5772 }
5773 
5774 // Allocate a file structure.
5775 struct file*
5776 filealloc(void)
5777 {
5778   struct file *f;
5779 
5780   acquire(&ftable.lock);
5781   for(f = ftable.file; f < ftable.file + NFILE; f++){
5782     if(f->ref == 0){
5783       f->ref = 1;
5784       release(&ftable.lock);
5785       return f;
5786     }
5787   }
5788   release(&ftable.lock);
5789   return 0;
5790 }
5791 
5792 
5793 
5794 
5795 
5796 
5797 
5798 
5799 
5800 // Increment ref count for file f.
5801 struct file*
5802 filedup(struct file *f)
5803 {
5804   acquire(&ftable.lock);
5805   if(f->ref < 1)
5806     panic("filedup");
5807   f->ref++;
5808   release(&ftable.lock);
5809   return f;
5810 }
5811 
5812 // Close file f.  (Decrement ref count, close when reaches 0.)
5813 void
5814 fileclose(struct file *f)
5815 {
5816   struct file ff;
5817 
5818   acquire(&ftable.lock);
5819   if(f->ref < 1)
5820     panic("fileclose");
5821   if(--f->ref > 0){
5822     release(&ftable.lock);
5823     return;
5824   }
5825   ff = *f;
5826   f->ref = 0;
5827   f->type = FD_NONE;
5828   release(&ftable.lock);
5829 
5830   if(ff.type == FD_PIPE)
5831     pipeclose(ff.pipe, ff.writable);
5832   else if(ff.type == FD_INODE){
5833     begin_op();
5834     iput(ff.ip);
5835     end_op();
5836   }
5837 }
5838 
5839 
5840 
5841 
5842 
5843 
5844 
5845 
5846 
5847 
5848 
5849 
5850 // Get metadata about file f.
5851 int
5852 filestat(struct file *f, struct stat *st)
5853 {
5854   if(f->type == FD_INODE){
5855     ilock(f->ip);
5856     stati(f->ip, st);
5857     iunlock(f->ip);
5858     return 0;
5859   }
5860   return -1;
5861 }
5862 
5863 // Read from file f.
5864 int
5865 fileread(struct file *f, char *addr, int n)
5866 {
5867   int r;
5868 
5869   if(f->readable == 0)
5870     return -1;
5871   if(f->type == FD_PIPE)
5872     return piperead(f->pipe, addr, n);
5873   if(f->type == FD_INODE){
5874     ilock(f->ip);
5875     if((r = readi(f->ip, addr, f->off, n)) > 0)
5876       f->off += r;
5877     iunlock(f->ip);
5878     return r;
5879   }
5880   panic("fileread");
5881 }
5882 
5883 // Write to file f.
5884 int
5885 filewrite(struct file *f, char *addr, int n)
5886 {
5887   int r;
5888 
5889   if(f->writable == 0)
5890     return -1;
5891   if(f->type == FD_PIPE)
5892     return pipewrite(f->pipe, addr, n);
5893   if(f->type == FD_INODE){
5894     // write a few blocks at a time to avoid exceeding
5895     // the maximum log transaction size, including
5896     // i-node, indirect block, allocation blocks,
5897     // and 2 blocks of slop for non-aligned writes.
5898     // this really belongs lower down, since writei()
5899     // might be writing a device like the console.
5900     int max = ((MAXOPBLOCKS-1-1-2) / 2) * 512;
5901     int i = 0;
5902     while(i < n){
5903       int n1 = n - i;
5904       if(n1 > max)
5905         n1 = max;
5906 
5907       begin_op();
5908       ilock(f->ip);
5909       if ((r = writei(f->ip, addr + i, f->off, n1)) > 0)
5910         f->off += r;
5911       iunlock(f->ip);
5912       end_op();
5913 
5914       if(r < 0)
5915         break;
5916       if(r != n1)
5917         panic("short filewrite");
5918       i += r;
5919     }
5920     return i == n ? n : -1;
5921   }
5922   panic("filewrite");
5923 }
5924 
5925 
5926 
5927 
5928 
5929 
5930 
5931 
5932 
5933 
5934 
5935 
5936 
5937 
5938 
5939 
5940 
5941 
5942 
5943 
5944 
5945 
5946 
5947 
5948 
5949 
