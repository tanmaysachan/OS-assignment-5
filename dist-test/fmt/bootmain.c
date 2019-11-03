8900 // Boot loader.
8901 //
8902 // Part of the boot block, along with bootasm.S, which calls bootmain().
8903 // bootasm.S has put the processor into protected 32-bit mode.
8904 // bootmain() loads an ELF kernel image from the disk starting at
8905 // sector 1 and then jumps to the kernel entry routine.
8906 
8907 #include "types.h"
8908 #include "elf.h"
8909 #include "x86.h"
8910 #include "memlayout.h"
8911 
8912 #define SECTSIZE  512
8913 
8914 void readseg(uchar*, uint, uint);
8915 
8916 void
8917 bootmain(void)
8918 {
8919   struct elfhdr *elf;
8920   struct proghdr *ph, *eph;
8921   void (*entry)(void);
8922   uchar* pa;
8923 
8924   elf = (struct elfhdr*)0x10000;  // scratch space
8925 
8926   // Read 1st page off disk
8927   readseg((uchar*)elf, 4096, 0);
8928 
8929   // Is this an ELF executable?
8930   if(elf->magic != ELF_MAGIC)
8931     return;  // let bootasm.S handle error
8932 
8933   // Load each program segment (ignores ph flags).
8934   ph = (struct proghdr*)((uchar*)elf + elf->phoff);
8935   eph = ph + elf->phnum;
8936   for(; ph < eph; ph++){
8937     pa = (uchar*)ph->paddr;
8938     readseg(pa, ph->filesz, ph->off);
8939     if(ph->memsz > ph->filesz)
8940       stosb(pa + ph->filesz, 0, ph->memsz - ph->filesz);
8941   }
8942 
8943   // Call the entry point from the ELF header.
8944   // Does not return!
8945   entry = (void(*)(void))(elf->entry);
8946   entry();
8947 }
8948 
8949 
8950 void
8951 waitdisk(void)
8952 {
8953   // Wait for disk ready.
8954   while((inb(0x1F7) & 0xC0) != 0x40)
8955     ;
8956 }
8957 
8958 // Read a single sector at offset into dst.
8959 void
8960 readsect(void *dst, uint offset)
8961 {
8962   // Issue command.
8963   waitdisk();
8964   outb(0x1F2, 1);   // count = 1
8965   outb(0x1F3, offset);
8966   outb(0x1F4, offset >> 8);
8967   outb(0x1F5, offset >> 16);
8968   outb(0x1F6, (offset >> 24) | 0xE0);
8969   outb(0x1F7, 0x20);  // cmd 0x20 - read sectors
8970 
8971   // Read data.
8972   waitdisk();
8973   insl(0x1F0, dst, SECTSIZE/4);
8974 }
8975 
8976 // Read 'count' bytes at 'offset' from kernel into physical address 'pa'.
8977 // Might copy more than asked.
8978 void
8979 readseg(uchar* pa, uint count, uint offset)
8980 {
8981   uchar* epa;
8982 
8983   epa = pa + count;
8984 
8985   // Round down to sector boundary.
8986   pa -= offset % SECTSIZE;
8987 
8988   // Translate from bytes to sectors; kernel starts at sector 1.
8989   offset = (offset / SECTSIZE) + 1;
8990 
8991   // If this is too slow, we could read lots of sectors at a time.
8992   // We'd write more to memory than asked, but it doesn't matter --
8993   // we load in increasing order.
8994   for(; pa < epa; pa += SECTSIZE, offset++)
8995     readsect(pa, offset);
8996 }
8997 
8998 
8999 
