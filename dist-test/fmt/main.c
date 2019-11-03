1200 #include "types.h"
1201 #include "defs.h"
1202 #include "param.h"
1203 #include "memlayout.h"
1204 #include "mmu.h"
1205 #include "proc.h"
1206 #include "x86.h"
1207 
1208 static void startothers(void);
1209 static void mpmain(void)  __attribute__((noreturn));
1210 extern pde_t *kpgdir;
1211 extern char end[]; // first address after kernel loaded from ELF file
1212 
1213 // Bootstrap processor starts running C code here.
1214 // Allocate a real stack and switch to it, first
1215 // doing some setup required for memory allocator to work.
1216 int
1217 main(void)
1218 {
1219   kinit1(end, P2V(4*1024*1024)); // phys page allocator
1220   kvmalloc();      // kernel page table
1221   mpinit();        // detect other processors
1222   lapicinit();     // interrupt controller
1223   seginit();       // segment descriptors
1224   picinit();       // disable pic
1225   ioapicinit();    // another interrupt controller
1226   consoleinit();   // console hardware
1227   uartinit();      // serial port
1228   pinit();         // process table
1229   tvinit();        // trap vectors
1230   binit();         // buffer cache
1231   fileinit();      // file table
1232   ideinit();       // disk
1233   startothers();   // start other processors
1234   kinit2(P2V(4*1024*1024), P2V(PHYSTOP)); // must come after startothers()
1235   userinit();      // first user process
1236   mpmain();        // finish this processor's setup
1237 }
1238 
1239 // Other CPUs jump here from entryother.S.
1240 static void
1241 mpenter(void)
1242 {
1243   switchkvm();
1244   seginit();
1245   lapicinit();
1246   mpmain();
1247 }
1248 
1249 
1250 // Common CPU setup code.
1251 static void
1252 mpmain(void)
1253 {
1254   cprintf("cpu%d: starting %d\n", cpuid(), cpuid());
1255   idtinit();       // load idt register
1256   xchg(&(mycpu()->started), 1); // tell startothers() we're up
1257   scheduler();     // start running processes
1258 }
1259 
1260 pde_t entrypgdir[];  // For entry.S
1261 
1262 // Start the non-boot (AP) processors.
1263 static void
1264 startothers(void)
1265 {
1266   extern uchar _binary_entryother_start[], _binary_entryother_size[];
1267   uchar *code;
1268   struct cpu *c;
1269   char *stack;
1270 
1271   // Write entry code to unused memory at 0x7000.
1272   // The linker has placed the image of entryother.S in
1273   // _binary_entryother_start.
1274   code = P2V(0x7000);
1275   memmove(code, _binary_entryother_start, (uint)_binary_entryother_size);
1276 
1277   for(c = cpus; c < cpus+ncpu; c++){
1278     if(c == mycpu())  // We've started already.
1279       continue;
1280 
1281     // Tell entryother.S what stack to use, where to enter, and what
1282     // pgdir to use. We cannot use kpgdir yet, because the AP processor
1283     // is running in low  memory, so we use entrypgdir for the APs too.
1284     stack = kalloc();
1285     *(void**)(code-4) = stack + KSTACKSIZE;
1286     *(void(**)(void))(code-8) = mpenter;
1287     *(int**)(code-12) = (void *) V2P(entrypgdir);
1288 
1289     lapicstartap(c->apicid, V2P(code));
1290 
1291     // wait for cpu to finish mpmain()
1292     while(c->started == 0)
1293       ;
1294   }
1295 }
1296 
1297 
1298 
1299 
1300 // The boot page table used in entry.S and entryother.S.
1301 // Page directories (and page tables) must start on page boundaries,
1302 // hence the __aligned__ attribute.
1303 // PTE_PS in a page directory entry enables 4Mbyte pages.
1304 
1305 __attribute__((__aligned__(PGSIZE)))
1306 pde_t entrypgdir[NPDENTRIES] = {
1307   // Map VA's [0, 4MB) to PA's [0, 4MB)
1308   [0] = (0) | PTE_P | PTE_W | PTE_PS,
1309   // Map VA's [KERNBASE, KERNBASE+4MB) to PA's [0, 4MB)
1310   [KERNBASE>>PDXSHIFT] = (0) | PTE_P | PTE_W | PTE_PS,
1311 };
1312 
1313 // Blank page.
1314 // Blank page.
1315 // Blank page.
1316 
1317 
1318 
1319 
1320 
1321 
1322 
1323 
1324 
1325 
1326 
1327 
1328 
1329 
1330 
1331 
1332 
1333 
1334 
1335 
1336 
1337 
1338 
1339 
1340 
1341 
1342 
1343 
1344 
1345 
1346 
1347 
1348 
1349 
