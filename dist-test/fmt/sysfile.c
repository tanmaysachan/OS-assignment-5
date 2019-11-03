5950 //
5951 // File-system system calls.
5952 // Mostly argument checking, since we don't trust
5953 // user code, and calls into file.c and fs.c.
5954 //
5955 
5956 #include "types.h"
5957 #include "defs.h"
5958 #include "param.h"
5959 #include "stat.h"
5960 #include "mmu.h"
5961 #include "proc.h"
5962 #include "fs.h"
5963 #include "spinlock.h"
5964 #include "sleeplock.h"
5965 #include "file.h"
5966 #include "fcntl.h"
5967 
5968 // Fetch the nth word-sized system call argument as a file descriptor
5969 // and return both the descriptor and the corresponding struct file.
5970 static int
5971 argfd(int n, int *pfd, struct file **pf)
5972 {
5973   int fd;
5974   struct file *f;
5975 
5976   if(argint(n, &fd) < 0)
5977     return -1;
5978   if(fd < 0 || fd >= NOFILE || (f=myproc()->ofile[fd]) == 0)
5979     return -1;
5980   if(pfd)
5981     *pfd = fd;
5982   if(pf)
5983     *pf = f;
5984   return 0;
5985 }
5986 
5987 
5988 
5989 
5990 
5991 
5992 
5993 
5994 
5995 
5996 
5997 
5998 
5999 
6000 // Allocate a file descriptor for the given file.
6001 // Takes over file reference from caller on success.
6002 static int
6003 fdalloc(struct file *f)
6004 {
6005   int fd;
6006   struct proc *curproc = myproc();
6007 
6008   for(fd = 0; fd < NOFILE; fd++){
6009     if(curproc->ofile[fd] == 0){
6010       curproc->ofile[fd] = f;
6011       return fd;
6012     }
6013   }
6014   return -1;
6015 }
6016 
6017 int
6018 sys_dup(void)
6019 {
6020   struct file *f;
6021   int fd;
6022 
6023   if(argfd(0, 0, &f) < 0)
6024     return -1;
6025   if((fd=fdalloc(f)) < 0)
6026     return -1;
6027   filedup(f);
6028   return fd;
6029 }
6030 
6031 int
6032 sys_read(void)
6033 {
6034   struct file *f;
6035   int n;
6036   char *p;
6037 
6038   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
6039     return -1;
6040   return fileread(f, p, n);
6041 }
6042 
6043 
6044 
6045 
6046 
6047 
6048 
6049 
6050 int
6051 sys_write(void)
6052 {
6053   struct file *f;
6054   int n;
6055   char *p;
6056 
6057   if(argfd(0, 0, &f) < 0 || argint(2, &n) < 0 || argptr(1, &p, n) < 0)
6058     return -1;
6059   return filewrite(f, p, n);
6060 }
6061 
6062 int
6063 sys_close(void)
6064 {
6065   int fd;
6066   struct file *f;
6067 
6068   if(argfd(0, &fd, &f) < 0)
6069     return -1;
6070   myproc()->ofile[fd] = 0;
6071   fileclose(f);
6072   return 0;
6073 }
6074 
6075 int
6076 sys_fstat(void)
6077 {
6078   struct file *f;
6079   struct stat *st;
6080 
6081   if(argfd(0, 0, &f) < 0 || argptr(1, (void*)&st, sizeof(*st)) < 0)
6082     return -1;
6083   return filestat(f, st);
6084 }
6085 
6086 
6087 
6088 
6089 
6090 
6091 
6092 
6093 
6094 
6095 
6096 
6097 
6098 
6099 
6100 // Create the path new as a link to the same inode as old.
6101 int
6102 sys_link(void)
6103 {
6104   char name[DIRSIZ], *new, *old;
6105   struct inode *dp, *ip;
6106 
6107   if(argstr(0, &old) < 0 || argstr(1, &new) < 0)
6108     return -1;
6109 
6110   begin_op();
6111   if((ip = namei(old)) == 0){
6112     end_op();
6113     return -1;
6114   }
6115 
6116   ilock(ip);
6117   if(ip->type == T_DIR){
6118     iunlockput(ip);
6119     end_op();
6120     return -1;
6121   }
6122 
6123   ip->nlink++;
6124   iupdate(ip);
6125   iunlock(ip);
6126 
6127   if((dp = nameiparent(new, name)) == 0)
6128     goto bad;
6129   ilock(dp);
6130   if(dp->dev != ip->dev || dirlink(dp, name, ip->inum) < 0){
6131     iunlockput(dp);
6132     goto bad;
6133   }
6134   iunlockput(dp);
6135   iput(ip);
6136 
6137   end_op();
6138 
6139   return 0;
6140 
6141 bad:
6142   ilock(ip);
6143   ip->nlink--;
6144   iupdate(ip);
6145   iunlockput(ip);
6146   end_op();
6147   return -1;
6148 }
6149 
6150 // Is the directory dp empty except for "." and ".." ?
6151 static int
6152 isdirempty(struct inode *dp)
6153 {
6154   int off;
6155   struct dirent de;
6156 
6157   for(off=2*sizeof(de); off<dp->size; off+=sizeof(de)){
6158     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6159       panic("isdirempty: readi");
6160     if(de.inum != 0)
6161       return 0;
6162   }
6163   return 1;
6164 }
6165 
6166 int
6167 sys_unlink(void)
6168 {
6169   struct inode *ip, *dp;
6170   struct dirent de;
6171   char name[DIRSIZ], *path;
6172   uint off;
6173 
6174   if(argstr(0, &path) < 0)
6175     return -1;
6176 
6177   begin_op();
6178   if((dp = nameiparent(path, name)) == 0){
6179     end_op();
6180     return -1;
6181   }
6182 
6183   ilock(dp);
6184 
6185   // Cannot unlink "." or "..".
6186   if(namecmp(name, ".") == 0 || namecmp(name, "..") == 0)
6187     goto bad;
6188 
6189   if((ip = dirlookup(dp, name, &off)) == 0)
6190     goto bad;
6191   ilock(ip);
6192 
6193   if(ip->nlink < 1)
6194     panic("unlink: nlink < 1");
6195   if(ip->type == T_DIR && !isdirempty(ip)){
6196     iunlockput(ip);
6197     goto bad;
6198   }
6199 
6200   memset(&de, 0, sizeof(de));
6201   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
6202     panic("unlink: writei");
6203   if(ip->type == T_DIR){
6204     dp->nlink--;
6205     iupdate(dp);
6206   }
6207   iunlockput(dp);
6208 
6209   ip->nlink--;
6210   iupdate(ip);
6211   iunlockput(ip);
6212 
6213   end_op();
6214 
6215   return 0;
6216 
6217 bad:
6218   iunlockput(dp);
6219   end_op();
6220   return -1;
6221 }
6222 
6223 static struct inode*
6224 create(char *path, short type, short major, short minor)
6225 {
6226   struct inode *ip, *dp;
6227   char name[DIRSIZ];
6228 
6229   if((dp = nameiparent(path, name)) == 0)
6230     return 0;
6231   ilock(dp);
6232 
6233   if((ip = dirlookup(dp, name, 0)) != 0){
6234     iunlockput(dp);
6235     ilock(ip);
6236     if(type == T_FILE && ip->type == T_FILE)
6237       return ip;
6238     iunlockput(ip);
6239     return 0;
6240   }
6241 
6242   if((ip = ialloc(dp->dev, type)) == 0)
6243     panic("create: ialloc");
6244 
6245   ilock(ip);
6246   ip->major = major;
6247   ip->minor = minor;
6248   ip->nlink = 1;
6249   iupdate(ip);
6250   if(type == T_DIR){  // Create . and .. entries.
6251     dp->nlink++;  // for ".."
6252     iupdate(dp);
6253     // No ip->nlink++ for ".": avoid cyclic ref count.
6254     if(dirlink(ip, ".", ip->inum) < 0 || dirlink(ip, "..", dp->inum) < 0)
6255       panic("create dots");
6256   }
6257 
6258   if(dirlink(dp, name, ip->inum) < 0)
6259     panic("create: dirlink");
6260 
6261   iunlockput(dp);
6262 
6263   return ip;
6264 }
6265 
6266 int
6267 sys_open(void)
6268 {
6269   char *path;
6270   int fd, omode;
6271   struct file *f;
6272   struct inode *ip;
6273 
6274   if(argstr(0, &path) < 0 || argint(1, &omode) < 0)
6275     return -1;
6276 
6277   begin_op();
6278 
6279   if(omode & O_CREATE){
6280     ip = create(path, T_FILE, 0, 0);
6281     if(ip == 0){
6282       end_op();
6283       return -1;
6284     }
6285   } else {
6286     if((ip = namei(path)) == 0){
6287       end_op();
6288       return -1;
6289     }
6290     ilock(ip);
6291     if(ip->type == T_DIR && omode != O_RDONLY){
6292       iunlockput(ip);
6293       end_op();
6294       return -1;
6295     }
6296   }
6297 
6298 
6299 
6300   if((f = filealloc()) == 0 || (fd = fdalloc(f)) < 0){
6301     if(f)
6302       fileclose(f);
6303     iunlockput(ip);
6304     end_op();
6305     return -1;
6306   }
6307   iunlock(ip);
6308   end_op();
6309 
6310   f->type = FD_INODE;
6311   f->ip = ip;
6312   f->off = 0;
6313   f->readable = !(omode & O_WRONLY);
6314   f->writable = (omode & O_WRONLY) || (omode & O_RDWR);
6315   return fd;
6316 }
6317 
6318 int
6319 sys_mkdir(void)
6320 {
6321   char *path;
6322   struct inode *ip;
6323 
6324   begin_op();
6325   if(argstr(0, &path) < 0 || (ip = create(path, T_DIR, 0, 0)) == 0){
6326     end_op();
6327     return -1;
6328   }
6329   iunlockput(ip);
6330   end_op();
6331   return 0;
6332 }
6333 
6334 int
6335 sys_mknod(void)
6336 {
6337   struct inode *ip;
6338   char *path;
6339   int major, minor;
6340 
6341   begin_op();
6342   if((argstr(0, &path)) < 0 ||
6343      argint(1, &major) < 0 ||
6344      argint(2, &minor) < 0 ||
6345      (ip = create(path, T_DEV, major, minor)) == 0){
6346     end_op();
6347     return -1;
6348   }
6349   iunlockput(ip);
6350   end_op();
6351   return 0;
6352 }
6353 
6354 int
6355 sys_chdir(void)
6356 {
6357   char *path;
6358   struct inode *ip;
6359   struct proc *curproc = myproc();
6360 
6361   begin_op();
6362   if(argstr(0, &path) < 0 || (ip = namei(path)) == 0){
6363     end_op();
6364     return -1;
6365   }
6366   ilock(ip);
6367   if(ip->type != T_DIR){
6368     iunlockput(ip);
6369     end_op();
6370     return -1;
6371   }
6372   iunlock(ip);
6373   iput(curproc->cwd);
6374   end_op();
6375   curproc->cwd = ip;
6376   return 0;
6377 }
6378 
6379 int
6380 sys_exec(void)
6381 {
6382   char *path, *argv[MAXARG];
6383   int i;
6384   uint uargv, uarg;
6385 
6386   if(argstr(0, &path) < 0 || argint(1, (int*)&uargv) < 0){
6387     return -1;
6388   }
6389   memset(argv, 0, sizeof(argv));
6390   for(i=0;; i++){
6391     if(i >= NELEM(argv))
6392       return -1;
6393     if(fetchint(uargv+4*i, (int*)&uarg) < 0)
6394       return -1;
6395     if(uarg == 0){
6396       argv[i] = 0;
6397       break;
6398     }
6399     if(fetchstr(uarg, &argv[i]) < 0)
6400       return -1;
6401   }
6402   return exec(path, argv);
6403 }
6404 
6405 int
6406 sys_pipe(void)
6407 {
6408   int *fd;
6409   struct file *rf, *wf;
6410   int fd0, fd1;
6411 
6412   if(argptr(0, (void*)&fd, 2*sizeof(fd[0])) < 0)
6413     return -1;
6414   if(pipealloc(&rf, &wf) < 0)
6415     return -1;
6416   fd0 = -1;
6417   if((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0){
6418     if(fd0 >= 0)
6419       myproc()->ofile[fd0] = 0;
6420     fileclose(rf);
6421     fileclose(wf);
6422     return -1;
6423   }
6424   fd[0] = fd0;
6425   fd[1] = fd1;
6426   return 0;
6427 }
6428 
6429 
6430 
6431 
6432 
6433 
6434 
6435 
6436 
6437 
6438 
6439 
6440 
6441 
6442 
6443 
6444 
6445 
6446 
6447 
6448 
6449 
