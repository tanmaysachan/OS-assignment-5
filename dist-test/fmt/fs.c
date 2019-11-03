5000 // File system implementation.  Five layers:
5001 //   + Blocks: allocator for raw disk blocks.
5002 //   + Log: crash recovery for multi-step updates.
5003 //   + Files: inode allocator, reading, writing, metadata.
5004 //   + Directories: inode with special contents (list of other inodes!)
5005 //   + Names: paths like /usr/rtm/xv6/fs.c for convenient naming.
5006 //
5007 // This file contains the low-level file system manipulation
5008 // routines.  The (higher-level) system call implementations
5009 // are in sysfile.c.
5010 
5011 #include "types.h"
5012 #include "defs.h"
5013 #include "param.h"
5014 #include "stat.h"
5015 #include "mmu.h"
5016 #include "proc.h"
5017 #include "spinlock.h"
5018 #include "sleeplock.h"
5019 #include "fs.h"
5020 #include "buf.h"
5021 #include "file.h"
5022 
5023 #define min(a, b) ((a) < (b) ? (a) : (b))
5024 static void itrunc(struct inode*);
5025 // there should be one superblock per disk device, but we run with
5026 // only one device
5027 struct superblock sb;
5028 
5029 // Read the super block.
5030 void
5031 readsb(int dev, struct superblock *sb)
5032 {
5033   struct buf *bp;
5034 
5035   bp = bread(dev, 1);
5036   memmove(sb, bp->data, sizeof(*sb));
5037   brelse(bp);
5038 }
5039 
5040 
5041 
5042 
5043 
5044 
5045 
5046 
5047 
5048 
5049 
5050 // Zero a block.
5051 static void
5052 bzero(int dev, int bno)
5053 {
5054   struct buf *bp;
5055 
5056   bp = bread(dev, bno);
5057   memset(bp->data, 0, BSIZE);
5058   log_write(bp);
5059   brelse(bp);
5060 }
5061 
5062 // Blocks.
5063 
5064 // Allocate a zeroed disk block.
5065 static uint
5066 balloc(uint dev)
5067 {
5068   int b, bi, m;
5069   struct buf *bp;
5070 
5071   bp = 0;
5072   for(b = 0; b < sb.size; b += BPB){
5073     bp = bread(dev, BBLOCK(b, sb));
5074     for(bi = 0; bi < BPB && b + bi < sb.size; bi++){
5075       m = 1 << (bi % 8);
5076       if((bp->data[bi/8] & m) == 0){  // Is block free?
5077         bp->data[bi/8] |= m;  // Mark block in use.
5078         log_write(bp);
5079         brelse(bp);
5080         bzero(dev, b + bi);
5081         return b + bi;
5082       }
5083     }
5084     brelse(bp);
5085   }
5086   panic("balloc: out of blocks");
5087 }
5088 
5089 
5090 
5091 
5092 
5093 
5094 
5095 
5096 
5097 
5098 
5099 
5100 // Free a disk block.
5101 static void
5102 bfree(int dev, uint b)
5103 {
5104   struct buf *bp;
5105   int bi, m;
5106 
5107   bp = bread(dev, BBLOCK(b, sb));
5108   bi = b % BPB;
5109   m = 1 << (bi % 8);
5110   if((bp->data[bi/8] & m) == 0)
5111     panic("freeing free block");
5112   bp->data[bi/8] &= ~m;
5113   log_write(bp);
5114   brelse(bp);
5115 }
5116 
5117 // Inodes.
5118 //
5119 // An inode describes a single unnamed file.
5120 // The inode disk structure holds metadata: the file's type,
5121 // its size, the number of links referring to it, and the
5122 // list of blocks holding the file's content.
5123 //
5124 // The inodes are laid out sequentially on disk at
5125 // sb.startinode. Each inode has a number, indicating its
5126 // position on the disk.
5127 //
5128 // The kernel keeps a cache of in-use inodes in memory
5129 // to provide a place for synchronizing access
5130 // to inodes used by multiple processes. The cached
5131 // inodes include book-keeping information that is
5132 // not stored on disk: ip->ref and ip->valid.
5133 //
5134 // An inode and its in-memory representation go through a
5135 // sequence of states before they can be used by the
5136 // rest of the file system code.
5137 //
5138 // * Allocation: an inode is allocated if its type (on disk)
5139 //   is non-zero. ialloc() allocates, and iput() frees if
5140 //   the reference and link counts have fallen to zero.
5141 //
5142 // * Referencing in cache: an entry in the inode cache
5143 //   is free if ip->ref is zero. Otherwise ip->ref tracks
5144 //   the number of in-memory pointers to the entry (open
5145 //   files and current directories). iget() finds or
5146 //   creates a cache entry and increments its ref; iput()
5147 //   decrements ref.
5148 //
5149 // * Valid: the information (type, size, &c) in an inode
5150 //   cache entry is only correct when ip->valid is 1.
5151 //   ilock() reads the inode from
5152 //   the disk and sets ip->valid, while iput() clears
5153 //   ip->valid if ip->ref has fallen to zero.
5154 //
5155 // * Locked: file system code may only examine and modify
5156 //   the information in an inode and its content if it
5157 //   has first locked the inode.
5158 //
5159 // Thus a typical sequence is:
5160 //   ip = iget(dev, inum)
5161 //   ilock(ip)
5162 //   ... examine and modify ip->xxx ...
5163 //   iunlock(ip)
5164 //   iput(ip)
5165 //
5166 // ilock() is separate from iget() so that system calls can
5167 // get a long-term reference to an inode (as for an open file)
5168 // and only lock it for short periods (e.g., in read()).
5169 // The separation also helps avoid deadlock and races during
5170 // pathname lookup. iget() increments ip->ref so that the inode
5171 // stays cached and pointers to it remain valid.
5172 //
5173 // Many internal file system functions expect the caller to
5174 // have locked the inodes involved; this lets callers create
5175 // multi-step atomic operations.
5176 //
5177 // The icache.lock spin-lock protects the allocation of icache
5178 // entries. Since ip->ref indicates whether an entry is free,
5179 // and ip->dev and ip->inum indicate which i-node an entry
5180 // holds, one must hold icache.lock while using any of those fields.
5181 //
5182 // An ip->lock sleep-lock protects all ip-> fields other than ref,
5183 // dev, and inum.  One must hold ip->lock in order to
5184 // read or write that inode's ip->valid, ip->size, ip->type, &c.
5185 
5186 struct {
5187   struct spinlock lock;
5188   struct inode inode[NINODE];
5189 } icache;
5190 
5191 void
5192 iinit(int dev)
5193 {
5194   int i = 0;
5195 
5196   initlock(&icache.lock, "icache");
5197   for(i = 0; i < NINODE; i++) {
5198     initsleeplock(&icache.inode[i].lock, "inode");
5199   }
5200   readsb(dev, &sb);
5201   cprintf("sb: size %d nblocks %d ninodes %d nlog %d logstart %d\
5202  inodestart %d bmap start %d\n", sb.size, sb.nblocks,
5203           sb.ninodes, sb.nlog, sb.logstart, sb.inodestart,
5204           sb.bmapstart);
5205 }
5206 
5207 static struct inode* iget(uint dev, uint inum);
5208 
5209 // Allocate an inode on device dev.
5210 // Mark it as allocated by  giving it type type.
5211 // Returns an unlocked but allocated and referenced inode.
5212 struct inode*
5213 ialloc(uint dev, short type)
5214 {
5215   int inum;
5216   struct buf *bp;
5217   struct dinode *dip;
5218 
5219   for(inum = 1; inum < sb.ninodes; inum++){
5220     bp = bread(dev, IBLOCK(inum, sb));
5221     dip = (struct dinode*)bp->data + inum%IPB;
5222     if(dip->type == 0){  // a free inode
5223       memset(dip, 0, sizeof(*dip));
5224       dip->type = type;
5225       log_write(bp);   // mark it allocated on the disk
5226       brelse(bp);
5227       return iget(dev, inum);
5228     }
5229     brelse(bp);
5230   }
5231   panic("ialloc: no inodes");
5232 }
5233 
5234 // Copy a modified in-memory inode to disk.
5235 // Must be called after every change to an ip->xxx field
5236 // that lives on disk, since i-node cache is write-through.
5237 // Caller must hold ip->lock.
5238 void
5239 iupdate(struct inode *ip)
5240 {
5241   struct buf *bp;
5242   struct dinode *dip;
5243 
5244   bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5245   dip = (struct dinode*)bp->data + ip->inum%IPB;
5246   dip->type = ip->type;
5247   dip->major = ip->major;
5248   dip->minor = ip->minor;
5249   dip->nlink = ip->nlink;
5250   dip->size = ip->size;
5251   memmove(dip->addrs, ip->addrs, sizeof(ip->addrs));
5252   log_write(bp);
5253   brelse(bp);
5254 }
5255 
5256 // Find the inode with number inum on device dev
5257 // and return the in-memory copy. Does not lock
5258 // the inode and does not read it from disk.
5259 static struct inode*
5260 iget(uint dev, uint inum)
5261 {
5262   struct inode *ip, *empty;
5263 
5264   acquire(&icache.lock);
5265 
5266   // Is the inode already cached?
5267   empty = 0;
5268   for(ip = &icache.inode[0]; ip < &icache.inode[NINODE]; ip++){
5269     if(ip->ref > 0 && ip->dev == dev && ip->inum == inum){
5270       ip->ref++;
5271       release(&icache.lock);
5272       return ip;
5273     }
5274     if(empty == 0 && ip->ref == 0)    // Remember empty slot.
5275       empty = ip;
5276   }
5277 
5278   // Recycle an inode cache entry.
5279   if(empty == 0)
5280     panic("iget: no inodes");
5281 
5282   ip = empty;
5283   ip->dev = dev;
5284   ip->inum = inum;
5285   ip->ref = 1;
5286   ip->valid = 0;
5287   release(&icache.lock);
5288 
5289   return ip;
5290 }
5291 
5292 
5293 
5294 
5295 
5296 
5297 
5298 
5299 
5300 // Increment reference count for ip.
5301 // Returns ip to enable ip = idup(ip1) idiom.
5302 struct inode*
5303 idup(struct inode *ip)
5304 {
5305   acquire(&icache.lock);
5306   ip->ref++;
5307   release(&icache.lock);
5308   return ip;
5309 }
5310 
5311 // Lock the given inode.
5312 // Reads the inode from disk if necessary.
5313 void
5314 ilock(struct inode *ip)
5315 {
5316   struct buf *bp;
5317   struct dinode *dip;
5318 
5319   if(ip == 0 || ip->ref < 1)
5320     panic("ilock");
5321 
5322   acquiresleep(&ip->lock);
5323 
5324   if(ip->valid == 0){
5325     bp = bread(ip->dev, IBLOCK(ip->inum, sb));
5326     dip = (struct dinode*)bp->data + ip->inum%IPB;
5327     ip->type = dip->type;
5328     ip->major = dip->major;
5329     ip->minor = dip->minor;
5330     ip->nlink = dip->nlink;
5331     ip->size = dip->size;
5332     memmove(ip->addrs, dip->addrs, sizeof(ip->addrs));
5333     brelse(bp);
5334     ip->valid = 1;
5335     if(ip->type == 0)
5336       panic("ilock: no type");
5337   }
5338 }
5339 
5340 // Unlock the given inode.
5341 void
5342 iunlock(struct inode *ip)
5343 {
5344   if(ip == 0 || !holdingsleep(&ip->lock) || ip->ref < 1)
5345     panic("iunlock");
5346 
5347   releasesleep(&ip->lock);
5348 }
5349 
5350 // Drop a reference to an in-memory inode.
5351 // If that was the last reference, the inode cache entry can
5352 // be recycled.
5353 // If that was the last reference and the inode has no links
5354 // to it, free the inode (and its content) on disk.
5355 // All calls to iput() must be inside a transaction in
5356 // case it has to free the inode.
5357 void
5358 iput(struct inode *ip)
5359 {
5360   acquiresleep(&ip->lock);
5361   if(ip->valid && ip->nlink == 0){
5362     acquire(&icache.lock);
5363     int r = ip->ref;
5364     release(&icache.lock);
5365     if(r == 1){
5366       // inode has no links and no other references: truncate and free.
5367       itrunc(ip);
5368       ip->type = 0;
5369       iupdate(ip);
5370       ip->valid = 0;
5371     }
5372   }
5373   releasesleep(&ip->lock);
5374 
5375   acquire(&icache.lock);
5376   ip->ref--;
5377   release(&icache.lock);
5378 }
5379 
5380 // Common idiom: unlock, then put.
5381 void
5382 iunlockput(struct inode *ip)
5383 {
5384   iunlock(ip);
5385   iput(ip);
5386 }
5387 
5388 
5389 
5390 
5391 
5392 
5393 
5394 
5395 
5396 
5397 
5398 
5399 
5400 // Inode content
5401 //
5402 // The content (data) associated with each inode is stored
5403 // in blocks on the disk. The first NDIRECT block numbers
5404 // are listed in ip->addrs[].  The next NINDIRECT blocks are
5405 // listed in block ip->addrs[NDIRECT].
5406 
5407 // Return the disk block address of the nth block in inode ip.
5408 // If there is no such block, bmap allocates one.
5409 static uint
5410 bmap(struct inode *ip, uint bn)
5411 {
5412   uint addr, *a;
5413   struct buf *bp;
5414 
5415   if(bn < NDIRECT){
5416     if((addr = ip->addrs[bn]) == 0)
5417       ip->addrs[bn] = addr = balloc(ip->dev);
5418     return addr;
5419   }
5420   bn -= NDIRECT;
5421 
5422   if(bn < NINDIRECT){
5423     // Load indirect block, allocating if necessary.
5424     if((addr = ip->addrs[NDIRECT]) == 0)
5425       ip->addrs[NDIRECT] = addr = balloc(ip->dev);
5426     bp = bread(ip->dev, addr);
5427     a = (uint*)bp->data;
5428     if((addr = a[bn]) == 0){
5429       a[bn] = addr = balloc(ip->dev);
5430       log_write(bp);
5431     }
5432     brelse(bp);
5433     return addr;
5434   }
5435 
5436   panic("bmap: out of range");
5437 }
5438 
5439 
5440 
5441 
5442 
5443 
5444 
5445 
5446 
5447 
5448 
5449 
5450 // Truncate inode (discard contents).
5451 // Only called when the inode has no links
5452 // to it (no directory entries referring to it)
5453 // and has no in-memory reference to it (is
5454 // not an open file or current directory).
5455 static void
5456 itrunc(struct inode *ip)
5457 {
5458   int i, j;
5459   struct buf *bp;
5460   uint *a;
5461 
5462   for(i = 0; i < NDIRECT; i++){
5463     if(ip->addrs[i]){
5464       bfree(ip->dev, ip->addrs[i]);
5465       ip->addrs[i] = 0;
5466     }
5467   }
5468 
5469   if(ip->addrs[NDIRECT]){
5470     bp = bread(ip->dev, ip->addrs[NDIRECT]);
5471     a = (uint*)bp->data;
5472     for(j = 0; j < NINDIRECT; j++){
5473       if(a[j])
5474         bfree(ip->dev, a[j]);
5475     }
5476     brelse(bp);
5477     bfree(ip->dev, ip->addrs[NDIRECT]);
5478     ip->addrs[NDIRECT] = 0;
5479   }
5480 
5481   ip->size = 0;
5482   iupdate(ip);
5483 }
5484 
5485 // Copy stat information from inode.
5486 // Caller must hold ip->lock.
5487 void
5488 stati(struct inode *ip, struct stat *st)
5489 {
5490   st->dev = ip->dev;
5491   st->ino = ip->inum;
5492   st->type = ip->type;
5493   st->nlink = ip->nlink;
5494   st->size = ip->size;
5495 }
5496 
5497 
5498 
5499 
5500 // Read data from inode.
5501 // Caller must hold ip->lock.
5502 int
5503 readi(struct inode *ip, char *dst, uint off, uint n)
5504 {
5505   uint tot, m;
5506   struct buf *bp;
5507 
5508   if(ip->type == T_DEV){
5509     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].read)
5510       return -1;
5511     return devsw[ip->major].read(ip, dst, n);
5512   }
5513 
5514   if(off > ip->size || off + n < off)
5515     return -1;
5516   if(off + n > ip->size)
5517     n = ip->size - off;
5518 
5519   for(tot=0; tot<n; tot+=m, off+=m, dst+=m){
5520     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5521     m = min(n - tot, BSIZE - off%BSIZE);
5522     memmove(dst, bp->data + off%BSIZE, m);
5523     brelse(bp);
5524   }
5525   return n;
5526 }
5527 
5528 // Write data to inode.
5529 // Caller must hold ip->lock.
5530 int
5531 writei(struct inode *ip, char *src, uint off, uint n)
5532 {
5533   uint tot, m;
5534   struct buf *bp;
5535 
5536   if(ip->type == T_DEV){
5537     if(ip->major < 0 || ip->major >= NDEV || !devsw[ip->major].write)
5538       return -1;
5539     return devsw[ip->major].write(ip, src, n);
5540   }
5541 
5542   if(off > ip->size || off + n < off)
5543     return -1;
5544   if(off + n > MAXFILE*BSIZE)
5545     return -1;
5546 
5547 
5548 
5549 
5550   for(tot=0; tot<n; tot+=m, off+=m, src+=m){
5551     bp = bread(ip->dev, bmap(ip, off/BSIZE));
5552     m = min(n - tot, BSIZE - off%BSIZE);
5553     memmove(bp->data + off%BSIZE, src, m);
5554     log_write(bp);
5555     brelse(bp);
5556   }
5557 
5558   if(n > 0 && off > ip->size){
5559     ip->size = off;
5560     iupdate(ip);
5561   }
5562   return n;
5563 }
5564 
5565 // Directories
5566 
5567 int
5568 namecmp(const char *s, const char *t)
5569 {
5570   return strncmp(s, t, DIRSIZ);
5571 }
5572 
5573 // Look for a directory entry in a directory.
5574 // If found, set *poff to byte offset of entry.
5575 struct inode*
5576 dirlookup(struct inode *dp, char *name, uint *poff)
5577 {
5578   uint off, inum;
5579   struct dirent de;
5580 
5581   if(dp->type != T_DIR)
5582     panic("dirlookup not DIR");
5583 
5584   for(off = 0; off < dp->size; off += sizeof(de)){
5585     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5586       panic("dirlookup read");
5587     if(de.inum == 0)
5588       continue;
5589     if(namecmp(name, de.name) == 0){
5590       // entry matches path element
5591       if(poff)
5592         *poff = off;
5593       inum = de.inum;
5594       return iget(dp->dev, inum);
5595     }
5596   }
5597 
5598   return 0;
5599 }
5600 // Write a new directory entry (name, inum) into the directory dp.
5601 int
5602 dirlink(struct inode *dp, char *name, uint inum)
5603 {
5604   int off;
5605   struct dirent de;
5606   struct inode *ip;
5607 
5608   // Check that name is not present.
5609   if((ip = dirlookup(dp, name, 0)) != 0){
5610     iput(ip);
5611     return -1;
5612   }
5613 
5614   // Look for an empty dirent.
5615   for(off = 0; off < dp->size; off += sizeof(de)){
5616     if(readi(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5617       panic("dirlink read");
5618     if(de.inum == 0)
5619       break;
5620   }
5621 
5622   strncpy(de.name, name, DIRSIZ);
5623   de.inum = inum;
5624   if(writei(dp, (char*)&de, off, sizeof(de)) != sizeof(de))
5625     panic("dirlink");
5626 
5627   return 0;
5628 }
5629 
5630 // Paths
5631 
5632 // Copy the next path element from path into name.
5633 // Return a pointer to the element following the copied one.
5634 // The returned path has no leading slashes,
5635 // so the caller can check *path=='\0' to see if the name is the last one.
5636 // If no name to remove, return 0.
5637 //
5638 // Examples:
5639 //   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
5640 //   skipelem("///a//bb", name) = "bb", setting name = "a"
5641 //   skipelem("a", name) = "", setting name = "a"
5642 //   skipelem("", name) = skipelem("////", name) = 0
5643 //
5644 static char*
5645 skipelem(char *path, char *name)
5646 {
5647   char *s;
5648   int len;
5649 
5650   while(*path == '/')
5651     path++;
5652   if(*path == 0)
5653     return 0;
5654   s = path;
5655   while(*path != '/' && *path != 0)
5656     path++;
5657   len = path - s;
5658   if(len >= DIRSIZ)
5659     memmove(name, s, DIRSIZ);
5660   else {
5661     memmove(name, s, len);
5662     name[len] = 0;
5663   }
5664   while(*path == '/')
5665     path++;
5666   return path;
5667 }
5668 
5669 // Look up and return the inode for a path name.
5670 // If parent != 0, return the inode for the parent and copy the final
5671 // path element into name, which must have room for DIRSIZ bytes.
5672 // Must be called inside a transaction since it calls iput().
5673 static struct inode*
5674 namex(char *path, int nameiparent, char *name)
5675 {
5676   struct inode *ip, *next;
5677 
5678   if(*path == '/')
5679     ip = iget(ROOTDEV, ROOTINO);
5680   else
5681     ip = idup(myproc()->cwd);
5682 
5683   while((path = skipelem(path, name)) != 0){
5684     ilock(ip);
5685     if(ip->type != T_DIR){
5686       iunlockput(ip);
5687       return 0;
5688     }
5689     if(nameiparent && *path == '\0'){
5690       // Stop one level early.
5691       iunlock(ip);
5692       return ip;
5693     }
5694     if((next = dirlookup(ip, name, 0)) == 0){
5695       iunlockput(ip);
5696       return 0;
5697     }
5698     iunlockput(ip);
5699     ip = next;
5700   }
5701   if(nameiparent){
5702     iput(ip);
5703     return 0;
5704   }
5705   return ip;
5706 }
5707 
5708 struct inode*
5709 namei(char *path)
5710 {
5711   char name[DIRSIZ];
5712   return namex(path, 0, name);
5713 }
5714 
5715 struct inode*
5716 nameiparent(char *path, char *name)
5717 {
5718   return namex(path, 1, name);
5719 }
5720 
5721 
5722 
5723 
5724 
5725 
5726 
5727 
5728 
5729 
5730 
5731 
5732 
5733 
5734 
5735 
5736 
5737 
5738 
5739 
5740 
5741 
5742 
5743 
5744 
5745 
5746 
5747 
5748 
5749 
