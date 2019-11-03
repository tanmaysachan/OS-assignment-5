3100 // Physical memory allocator, intended to allocate
3101 // memory for user processes, kernel stacks, page table pages,
3102 // and pipe buffers. Allocates 4096-byte pages.
3103 
3104 #include "types.h"
3105 #include "defs.h"
3106 #include "param.h"
3107 #include "memlayout.h"
3108 #include "mmu.h"
3109 #include "spinlock.h"
3110 
3111 void freerange(void *vstart, void *vend);
3112 extern char end[]; // first address after kernel loaded from ELF file
3113                    // defined by the kernel linker script in kernel.ld
3114 
3115 struct run {
3116   struct run *next;
3117 };
3118 
3119 struct {
3120   struct spinlock lock;
3121   int use_lock;
3122   struct run *freelist;
3123 } kmem;
3124 
3125 // Initialization happens in two phases.
3126 // 1. main() calls kinit1() while still using entrypgdir to place just
3127 // the pages mapped by entrypgdir on free list.
3128 // 2. main() calls kinit2() with the rest of the physical pages
3129 // after installing a full page table that maps them on all cores.
3130 void
3131 kinit1(void *vstart, void *vend)
3132 {
3133   initlock(&kmem.lock, "kmem");
3134   kmem.use_lock = 0;
3135   freerange(vstart, vend);
3136 }
3137 
3138 void
3139 kinit2(void *vstart, void *vend)
3140 {
3141   freerange(vstart, vend);
3142   kmem.use_lock = 1;
3143 }
3144 
3145 
3146 
3147 
3148 
3149 
3150 void
3151 freerange(void *vstart, void *vend)
3152 {
3153   char *p;
3154   p = (char*)PGROUNDUP((uint)vstart);
3155   for(; p + PGSIZE <= (char*)vend; p += PGSIZE)
3156     kfree(p);
3157 }
3158 // Free the page of physical memory pointed at by v,
3159 // which normally should have been returned by a
3160 // call to kalloc().  (The exception is when
3161 // initializing the allocator; see kinit above.)
3162 void
3163 kfree(char *v)
3164 {
3165   struct run *r;
3166 
3167   if((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
3168     panic("kfree");
3169 
3170   // Fill with junk to catch dangling refs.
3171   memset(v, 1, PGSIZE);
3172 
3173   if(kmem.use_lock)
3174     acquire(&kmem.lock);
3175   r = (struct run*)v;
3176   r->next = kmem.freelist;
3177   kmem.freelist = r;
3178   if(kmem.use_lock)
3179     release(&kmem.lock);
3180 }
3181 
3182 // Allocate one 4096-byte page of physical memory.
3183 // Returns a pointer that the kernel can use.
3184 // Returns 0 if the memory cannot be allocated.
3185 char*
3186 kalloc(void)
3187 {
3188   struct run *r;
3189 
3190   if(kmem.use_lock)
3191     acquire(&kmem.lock);
3192   r = kmem.freelist;
3193   if(r)
3194     kmem.freelist = r->next;
3195   if(kmem.use_lock)
3196     release(&kmem.lock);
3197   return (char*)r;
3198 }
3199 
