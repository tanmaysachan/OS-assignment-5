7000 // Multiprocessor support
7001 // Search memory for MP description structures.
7002 // http://developer.intel.com/design/pentium/datashts/24201606.pdf
7003 
7004 #include "types.h"
7005 #include "defs.h"
7006 #include "param.h"
7007 #include "memlayout.h"
7008 #include "mp.h"
7009 #include "x86.h"
7010 #include "mmu.h"
7011 #include "proc.h"
7012 
7013 struct cpu cpus[NCPU];
7014 int ncpu;
7015 uchar ioapicid;
7016 
7017 static uchar
7018 sum(uchar *addr, int len)
7019 {
7020   int i, sum;
7021 
7022   sum = 0;
7023   for(i=0; i<len; i++)
7024     sum += addr[i];
7025   return sum;
7026 }
7027 
7028 // Look for an MP structure in the len bytes at addr.
7029 static struct mp*
7030 mpsearch1(uint a, int len)
7031 {
7032   uchar *e, *p, *addr;
7033 
7034   addr = P2V(a);
7035   e = addr+len;
7036   for(p = addr; p < e; p += sizeof(struct mp))
7037     if(memcmp(p, "_MP_", 4) == 0 && sum(p, sizeof(struct mp)) == 0)
7038       return (struct mp*)p;
7039   return 0;
7040 }
7041 
7042 
7043 
7044 
7045 
7046 
7047 
7048 
7049 
7050 // Search for the MP Floating Pointer Structure, which according to the
7051 // spec is in one of the following three locations:
7052 // 1) in the first KB of the EBDA;
7053 // 2) in the last KB of system base memory;
7054 // 3) in the BIOS ROM between 0xE0000 and 0xFFFFF.
7055 static struct mp*
7056 mpsearch(void)
7057 {
7058   uchar *bda;
7059   uint p;
7060   struct mp *mp;
7061 
7062   bda = (uchar *) P2V(0x400);
7063   if((p = ((bda[0x0F]<<8)| bda[0x0E]) << 4)){
7064     if((mp = mpsearch1(p, 1024)))
7065       return mp;
7066   } else {
7067     p = ((bda[0x14]<<8)|bda[0x13])*1024;
7068     if((mp = mpsearch1(p-1024, 1024)))
7069       return mp;
7070   }
7071   return mpsearch1(0xF0000, 0x10000);
7072 }
7073 
7074 // Search for an MP configuration table.  For now,
7075 // don't accept the default configurations (physaddr == 0).
7076 // Check for correct signature, calculate the checksum and,
7077 // if correct, check the version.
7078 // To do: check extended table checksum.
7079 static struct mpconf*
7080 mpconfig(struct mp **pmp)
7081 {
7082   struct mpconf *conf;
7083   struct mp *mp;
7084 
7085   if((mp = mpsearch()) == 0 || mp->physaddr == 0)
7086     return 0;
7087   conf = (struct mpconf*) P2V((uint) mp->physaddr);
7088   if(memcmp(conf, "PCMP", 4) != 0)
7089     return 0;
7090   if(conf->version != 1 && conf->version != 4)
7091     return 0;
7092   if(sum((uchar*)conf, conf->length) != 0)
7093     return 0;
7094   *pmp = mp;
7095   return conf;
7096 }
7097 
7098 
7099 
7100 void
7101 mpinit(void)
7102 {
7103   uchar *p, *e;
7104   int ismp;
7105   struct mp *mp;
7106   struct mpconf *conf;
7107   struct mpproc *proc;
7108   struct mpioapic *ioapic;
7109 
7110   if((conf = mpconfig(&mp)) == 0)
7111     panic("Expect to run on an SMP");
7112   ismp = 1;
7113   lapic = (uint*)conf->lapicaddr;
7114   for(p=(uchar*)(conf+1), e=(uchar*)conf+conf->length; p<e; ){
7115     switch(*p){
7116     case MPPROC:
7117       proc = (struct mpproc*)p;
7118       if(ncpu < NCPU) {
7119         cpus[ncpu].apicid = proc->apicid;  // apicid may differ from ncpu
7120         ncpu++;
7121       }
7122       p += sizeof(struct mpproc);
7123       continue;
7124     case MPIOAPIC:
7125       ioapic = (struct mpioapic*)p;
7126       ioapicid = ioapic->apicno;
7127       p += sizeof(struct mpioapic);
7128       continue;
7129     case MPBUS:
7130     case MPIOINTR:
7131     case MPLINTR:
7132       p += 8;
7133       continue;
7134     default:
7135       ismp = 0;
7136       break;
7137     }
7138   }
7139   if(!ismp)
7140     panic("Didn't find a suitable machine");
7141 
7142   if(mp->imcrp){
7143     // Bochs doesn't support IMCR, so this doesn't run on Bochs.
7144     // But it would on real hardware.
7145     outb(0x22, 0x70);   // Select IMCR
7146     outb(0x23, inb(0x23) | 1);  // Mask external interrupts.
7147   }
7148 }
7149 
