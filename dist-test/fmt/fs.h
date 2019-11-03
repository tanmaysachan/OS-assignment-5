4150 // On-disk file system format.
4151 // Both the kernel and user programs use this header file.
4152 
4153 
4154 #define ROOTINO 1  // root i-number
4155 #define BSIZE 512  // block size
4156 
4157 // Disk layout:
4158 // [ boot block | super block | log | inode blocks |
4159 //                                          free bit map | data blocks]
4160 //
4161 // mkfs computes the super block and builds an initial file system. The
4162 // super block describes the disk layout:
4163 struct superblock {
4164   uint size;         // Size of file system image (blocks)
4165   uint nblocks;      // Number of data blocks
4166   uint ninodes;      // Number of inodes.
4167   uint nlog;         // Number of log blocks
4168   uint logstart;     // Block number of first log block
4169   uint inodestart;   // Block number of first inode block
4170   uint bmapstart;    // Block number of first free map block
4171 };
4172 
4173 #define NDIRECT 12
4174 #define NINDIRECT (BSIZE / sizeof(uint))
4175 #define MAXFILE (NDIRECT + NINDIRECT)
4176 
4177 // On-disk inode structure
4178 struct dinode {
4179   short type;           // File type
4180   short major;          // Major device number (T_DEV only)
4181   short minor;          // Minor device number (T_DEV only)
4182   short nlink;          // Number of links to inode in file system
4183   uint size;            // Size of file (bytes)
4184   uint addrs[NDIRECT+1];   // Data block addresses
4185 };
4186 
4187 
4188 
4189 
4190 
4191 
4192 
4193 
4194 
4195 
4196 
4197 
4198 
4199 
4200 // Inodes per block.
4201 #define IPB           (BSIZE / sizeof(struct dinode))
4202 
4203 // Block containing inode i
4204 #define IBLOCK(i, sb)     ((i) / IPB + sb.inodestart)
4205 
4206 // Bitmap bits per block
4207 #define BPB           (BSIZE*8)
4208 
4209 // Block of free map containing bit for block b
4210 #define BBLOCK(b, sb) (b/BPB + sb.bmapstart)
4211 
4212 // Directory is a file containing a sequence of dirent structures.
4213 #define DIRSIZ 14
4214 
4215 struct dirent {
4216   ushort inum;
4217   char name[DIRSIZ];
4218 };
4219 
4220 
4221 
4222 
4223 
4224 
4225 
4226 
4227 
4228 
4229 
4230 
4231 
4232 
4233 
4234 
4235 
4236 
4237 
4238 
4239 
4240 
4241 
4242 
4243 
4244 
4245 
4246 
4247 
4248 
4249 
