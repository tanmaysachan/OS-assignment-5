7150 // The local APIC manages internal (non-I/O) interrupts.
7151 // See Chapter 8 & Appendix C of Intel processor manual volume 3.
7152 
7153 #include "param.h"
7154 #include "types.h"
7155 #include "defs.h"
7156 #include "date.h"
7157 #include "memlayout.h"
7158 #include "traps.h"
7159 #include "mmu.h"
7160 #include "x86.h"
7161 
7162 // Local APIC registers, divided by 4 for use as uint[] indices.
7163 #define ID      (0x0020/4)   // ID
7164 #define VER     (0x0030/4)   // Version
7165 #define TPR     (0x0080/4)   // Task Priority
7166 #define EOI     (0x00B0/4)   // EOI
7167 #define SVR     (0x00F0/4)   // Spurious Interrupt Vector
7168   #define ENABLE     0x00000100   // Unit Enable
7169 #define ESR     (0x0280/4)   // Error Status
7170 #define ICRLO   (0x0300/4)   // Interrupt Command
7171   #define INIT       0x00000500   // INIT/RESET
7172   #define STARTUP    0x00000600   // Startup IPI
7173   #define DELIVS     0x00001000   // Delivery status
7174   #define ASSERT     0x00004000   // Assert interrupt (vs deassert)
7175   #define DEASSERT   0x00000000
7176   #define LEVEL      0x00008000   // Level triggered
7177   #define BCAST      0x00080000   // Send to all APICs, including self.
7178   #define BUSY       0x00001000
7179   #define FIXED      0x00000000
7180 #define ICRHI   (0x0310/4)   // Interrupt Command [63:32]
7181 #define TIMER   (0x0320/4)   // Local Vector Table 0 (TIMER)
7182   #define X1         0x0000000B   // divide counts by 1
7183   #define PERIODIC   0x00020000   // Periodic
7184 #define PCINT   (0x0340/4)   // Performance Counter LVT
7185 #define LINT0   (0x0350/4)   // Local Vector Table 1 (LINT0)
7186 #define LINT1   (0x0360/4)   // Local Vector Table 2 (LINT1)
7187 #define ERROR   (0x0370/4)   // Local Vector Table 3 (ERROR)
7188   #define MASKED     0x00010000   // Interrupt masked
7189 #define TICR    (0x0380/4)   // Timer Initial Count
7190 #define TCCR    (0x0390/4)   // Timer Current Count
7191 #define TDCR    (0x03E0/4)   // Timer Divide Configuration
7192 
7193 volatile uint *lapic;  // Initialized in mp.c
7194 
7195 static void
7196 lapicw(int index, int value)
7197 {
7198   lapic[index] = value;
7199   lapic[ID];  // wait for write to finish, by reading
7200 }
7201 
7202 void
7203 lapicinit(void)
7204 {
7205   if(!lapic)
7206     return;
7207 
7208   // Enable local APIC; set spurious interrupt vector.
7209   lapicw(SVR, ENABLE | (T_IRQ0 + IRQ_SPURIOUS));
7210 
7211   // The timer repeatedly counts down at bus frequency
7212   // from lapic[TICR] and then issues an interrupt.
7213   // If xv6 cared more about precise timekeeping,
7214   // TICR would be calibrated using an external time source.
7215   lapicw(TDCR, X1);
7216   lapicw(TIMER, PERIODIC | (T_IRQ0 + IRQ_TIMER));
7217   lapicw(TICR, 10000000);
7218 
7219   // Disable logical interrupt lines.
7220   lapicw(LINT0, MASKED);
7221   lapicw(LINT1, MASKED);
7222 
7223   // Disable performance counter overflow interrupts
7224   // on machines that provide that interrupt entry.
7225   if(((lapic[VER]>>16) & 0xFF) >= 4)
7226     lapicw(PCINT, MASKED);
7227 
7228   // Map error interrupt to IRQ_ERROR.
7229   lapicw(ERROR, T_IRQ0 + IRQ_ERROR);
7230 
7231   // Clear error status register (requires back-to-back writes).
7232   lapicw(ESR, 0);
7233   lapicw(ESR, 0);
7234 
7235   // Ack any outstanding interrupts.
7236   lapicw(EOI, 0);
7237 
7238   // Send an Init Level De-Assert to synchronise arbitration ID's.
7239   lapicw(ICRHI, 0);
7240   lapicw(ICRLO, BCAST | INIT | LEVEL);
7241   while(lapic[ICRLO] & DELIVS)
7242     ;
7243 
7244   // Enable interrupts on the APIC (but not on the processor).
7245   lapicw(TPR, 0);
7246 }
7247 
7248 
7249 
7250 int
7251 lapicid(void)
7252 {
7253   if (!lapic)
7254     return 0;
7255   return lapic[ID] >> 24;
7256 }
7257 
7258 // Acknowledge interrupt.
7259 void
7260 lapiceoi(void)
7261 {
7262   if(lapic)
7263     lapicw(EOI, 0);
7264 }
7265 
7266 // Spin for a given number of microseconds.
7267 // On real hardware would want to tune this dynamically.
7268 void
7269 microdelay(int us)
7270 {
7271 }
7272 
7273 #define CMOS_PORT    0x70
7274 #define CMOS_RETURN  0x71
7275 
7276 // Start additional processor running entry code at addr.
7277 // See Appendix B of MultiProcessor Specification.
7278 void
7279 lapicstartap(uchar apicid, uint addr)
7280 {
7281   int i;
7282   ushort *wrv;
7283 
7284   // "The BSP must initialize CMOS shutdown code to 0AH
7285   // and the warm reset vector (DWORD based at 40:67) to point at
7286   // the AP startup code prior to the [universal startup algorithm]."
7287   outb(CMOS_PORT, 0xF);  // offset 0xF is shutdown code
7288   outb(CMOS_PORT+1, 0x0A);
7289   wrv = (ushort*)P2V((0x40<<4 | 0x67));  // Warm reset vector
7290   wrv[0] = 0;
7291   wrv[1] = addr >> 4;
7292 
7293   // "Universal startup algorithm."
7294   // Send INIT (level-triggered) interrupt to reset other CPU.
7295   lapicw(ICRHI, apicid<<24);
7296   lapicw(ICRLO, INIT | LEVEL | ASSERT);
7297   microdelay(200);
7298   lapicw(ICRLO, INIT | LEVEL);
7299   microdelay(100);    // should be 10ms, but too slow in Bochs!
7300   // Send startup IPI (twice!) to enter code.
7301   // Regular hardware is supposed to only accept a STARTUP
7302   // when it is in the halted state due to an INIT.  So the second
7303   // should be ignored, but it is part of the official Intel algorithm.
7304   // Bochs complains about the second one.  Too bad for Bochs.
7305   for(i = 0; i < 2; i++){
7306     lapicw(ICRHI, apicid<<24);
7307     lapicw(ICRLO, STARTUP | (addr>>12));
7308     microdelay(200);
7309   }
7310 }
7311 
7312 #define CMOS_STATA   0x0a
7313 #define CMOS_STATB   0x0b
7314 #define CMOS_UIP    (1 << 7)        // RTC update in progress
7315 
7316 #define SECS    0x00
7317 #define MINS    0x02
7318 #define HOURS   0x04
7319 #define DAY     0x07
7320 #define MONTH   0x08
7321 #define YEAR    0x09
7322 
7323 static uint
7324 cmos_read(uint reg)
7325 {
7326   outb(CMOS_PORT,  reg);
7327   microdelay(200);
7328 
7329   return inb(CMOS_RETURN);
7330 }
7331 
7332 static void
7333 fill_rtcdate(struct rtcdate *r)
7334 {
7335   r->second = cmos_read(SECS);
7336   r->minute = cmos_read(MINS);
7337   r->hour   = cmos_read(HOURS);
7338   r->day    = cmos_read(DAY);
7339   r->month  = cmos_read(MONTH);
7340   r->year   = cmos_read(YEAR);
7341 }
7342 
7343 
7344 
7345 
7346 
7347 
7348 
7349 
7350 // qemu seems to use 24-hour GWT and the values are BCD encoded
7351 void
7352 cmostime(struct rtcdate *r)
7353 {
7354   struct rtcdate t1, t2;
7355   int sb, bcd;
7356 
7357   sb = cmos_read(CMOS_STATB);
7358 
7359   bcd = (sb & (1 << 2)) == 0;
7360 
7361   // make sure CMOS doesn't modify time while we read it
7362   for(;;) {
7363     fill_rtcdate(&t1);
7364     if(cmos_read(CMOS_STATA) & CMOS_UIP)
7365         continue;
7366     fill_rtcdate(&t2);
7367     if(memcmp(&t1, &t2, sizeof(t1)) == 0)
7368       break;
7369   }
7370 
7371   // convert
7372   if(bcd) {
7373 #define    CONV(x)     (t1.x = ((t1.x >> 4) * 10) + (t1.x & 0xf))
7374     CONV(second);
7375     CONV(minute);
7376     CONV(hour  );
7377     CONV(day   );
7378     CONV(month );
7379     CONV(year  );
7380 #undef     CONV
7381   }
7382 
7383   *r = t1;
7384   r->year += 2000;
7385 }
7386 
7387 
7388 
7389 
7390 
7391 
7392 
7393 
7394 
7395 
7396 
7397 
7398 
7399 
