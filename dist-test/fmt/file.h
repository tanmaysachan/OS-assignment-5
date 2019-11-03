4250 struct file {
4251   enum { FD_NONE, FD_PIPE, FD_INODE } type;
4252   int ref; // reference count
4253   char readable;
4254   char writable;
4255   struct pipe *pipe;
4256   struct inode *ip;
4257   uint off;
4258 };
4259 
4260 
4261 // in-memory copy of an inode
4262 struct inode {
4263   uint dev;           // Device number
4264   uint inum;          // Inode number
4265   int ref;            // Reference count
4266   struct sleeplock lock; // protects everything below here
4267   int valid;          // inode has been read from disk?
4268 
4269   short type;         // copy of disk inode
4270   short major;
4271   short minor;
4272   short nlink;
4273   uint size;
4274   uint addrs[NDIRECT+1];
4275 };
4276 
4277 // table mapping major device number to
4278 // device functions
4279 struct devsw {
4280   int (*read)(struct inode*, char*, int);
4281   int (*write)(struct inode*, char*, int);
4282 };
4283 
4284 extern struct devsw devsw[];
4285 
4286 #define CONSOLE 1
4287 
4288 
4289 
4290 
4291 
4292 
4293 
4294 
4295 
4296 
4297 
4298 
4299 
