7700 // Console input and output.
7701 // Input is from the keyboard or serial port.
7702 // Output is written to the screen and serial port.
7703 
7704 #include "types.h"
7705 #include "defs.h"
7706 #include "param.h"
7707 #include "traps.h"
7708 #include "spinlock.h"
7709 #include "sleeplock.h"
7710 #include "fs.h"
7711 #include "file.h"
7712 #include "memlayout.h"
7713 #include "mmu.h"
7714 #include "proc.h"
7715 #include "x86.h"
7716 
7717 static void consputc(int);
7718 
7719 static int panicked = 0;
7720 
7721 static struct {
7722   struct spinlock lock;
7723   int locking;
7724 } cons;
7725 
7726 static void
7727 printint(int xx, int base, int sign)
7728 {
7729   static char digits[] = "0123456789abcdef";
7730   char buf[16];
7731   int i;
7732   uint x;
7733 
7734   if(sign && (sign = xx < 0))
7735     x = -xx;
7736   else
7737     x = xx;
7738 
7739   i = 0;
7740   do{
7741     buf[i++] = digits[x % base];
7742   }while((x /= base) != 0);
7743 
7744   if(sign)
7745     buf[i++] = '-';
7746 
7747   while(--i >= 0)
7748     consputc(buf[i]);
7749 }
7750 // Print to the console. only understands %d, %x, %p, %s.
7751 void
7752 cprintf(char *fmt, ...)
7753 {
7754   int i, c, locking;
7755   uint *argp;
7756   char *s;
7757 
7758   locking = cons.locking;
7759   if(locking)
7760     acquire(&cons.lock);
7761 
7762   if (fmt == 0)
7763     panic("null fmt");
7764 
7765   argp = (uint*)(void*)(&fmt + 1);
7766   for(i = 0; (c = fmt[i] & 0xff) != 0; i++){
7767     if(c != '%'){
7768       consputc(c);
7769       continue;
7770     }
7771     c = fmt[++i] & 0xff;
7772     if(c == 0)
7773       break;
7774     switch(c){
7775     case 'd':
7776       printint(*argp++, 10, 1);
7777       break;
7778     case 'x':
7779     case 'p':
7780       printint(*argp++, 16, 0);
7781       break;
7782     case 's':
7783       if((s = (char*)*argp++) == 0)
7784         s = "(null)";
7785       for(; *s; s++)
7786         consputc(*s);
7787       break;
7788     case '%':
7789       consputc('%');
7790       break;
7791     default:
7792       // Print unknown % sequence to draw attention.
7793       consputc('%');
7794       consputc(c);
7795       break;
7796     }
7797   }
7798 
7799 
7800   if(locking)
7801     release(&cons.lock);
7802 }
7803 
7804 void
7805 panic(char *s)
7806 {
7807   int i;
7808   uint pcs[10];
7809 
7810   cli();
7811   cons.locking = 0;
7812   // use lapiccpunum so that we can call panic from mycpu()
7813   cprintf("lapicid %d: panic: ", lapicid());
7814   cprintf(s);
7815   cprintf("\n");
7816   getcallerpcs(&s, pcs);
7817   for(i=0; i<10; i++)
7818     cprintf(" %p", pcs[i]);
7819   panicked = 1; // freeze other CPU
7820   for(;;)
7821     ;
7822 }
7823 
7824 #define BACKSPACE 0x100
7825 #define CRTPORT 0x3d4
7826 static ushort *crt = (ushort*)P2V(0xb8000);  // CGA memory
7827 
7828 static void
7829 cgaputc(int c)
7830 {
7831   int pos;
7832 
7833   // Cursor position: col + 80*row.
7834   outb(CRTPORT, 14);
7835   pos = inb(CRTPORT+1) << 8;
7836   outb(CRTPORT, 15);
7837   pos |= inb(CRTPORT+1);
7838 
7839   if(c == '\n')
7840     pos += 80 - pos%80;
7841   else if(c == BACKSPACE){
7842     if(pos > 0) --pos;
7843   } else
7844     crt[pos++] = (c&0xff) | 0x0700;  // black on white
7845 
7846   if(pos < 0 || pos > 25*80)
7847     panic("pos under/overflow");
7848 
7849 
7850   if((pos/80) >= 24){  // Scroll up.
7851     memmove(crt, crt+80, sizeof(crt[0])*23*80);
7852     pos -= 80;
7853     memset(crt+pos, 0, sizeof(crt[0])*(24*80 - pos));
7854   }
7855 
7856   outb(CRTPORT, 14);
7857   outb(CRTPORT+1, pos>>8);
7858   outb(CRTPORT, 15);
7859   outb(CRTPORT+1, pos);
7860   crt[pos] = ' ' | 0x0700;
7861 }
7862 
7863 void
7864 consputc(int c)
7865 {
7866   if(panicked){
7867     cli();
7868     for(;;)
7869       ;
7870   }
7871 
7872   if(c == BACKSPACE){
7873     uartputc('\b'); uartputc(' '); uartputc('\b');
7874   } else
7875     uartputc(c);
7876   cgaputc(c);
7877 }
7878 
7879 #define INPUT_BUF 128
7880 struct {
7881   char buf[INPUT_BUF];
7882   uint r;  // Read index
7883   uint w;  // Write index
7884   uint e;  // Edit index
7885 } input;
7886 
7887 #define C(x)  ((x)-'@')  // Control-x
7888 
7889 void
7890 consoleintr(int (*getc)(void))
7891 {
7892   int c, doprocdump = 0;
7893 
7894   acquire(&cons.lock);
7895   while((c = getc()) >= 0){
7896     switch(c){
7897     case C('P'):  // Process listing.
7898       // procdump() locks cons.lock indirectly; invoke later
7899       doprocdump = 1;
7900       break;
7901     case C('U'):  // Kill line.
7902       while(input.e != input.w &&
7903             input.buf[(input.e-1) % INPUT_BUF] != '\n'){
7904         input.e--;
7905         consputc(BACKSPACE);
7906       }
7907       break;
7908     case C('H'): case '\x7f':  // Backspace
7909       if(input.e != input.w){
7910         input.e--;
7911         consputc(BACKSPACE);
7912       }
7913       break;
7914     default:
7915       if(c != 0 && input.e-input.r < INPUT_BUF){
7916         c = (c == '\r') ? '\n' : c;
7917         input.buf[input.e++ % INPUT_BUF] = c;
7918         consputc(c);
7919         if(c == '\n' || c == C('D') || input.e == input.r+INPUT_BUF){
7920           input.w = input.e;
7921           wakeup(&input.r);
7922         }
7923       }
7924       break;
7925     }
7926   }
7927   release(&cons.lock);
7928   if(doprocdump) {
7929     procdump();  // now call procdump() wo. cons.lock held
7930   }
7931 }
7932 
7933 int
7934 consoleread(struct inode *ip, char *dst, int n)
7935 {
7936   uint target;
7937   int c;
7938 
7939   iunlock(ip);
7940   target = n;
7941   acquire(&cons.lock);
7942   while(n > 0){
7943     while(input.r == input.w){
7944       if(myproc()->killed){
7945         release(&cons.lock);
7946         ilock(ip);
7947         return -1;
7948       }
7949       sleep(&input.r, &cons.lock);
7950     }
7951     c = input.buf[input.r++ % INPUT_BUF];
7952     if(c == C('D')){  // EOF
7953       if(n < target){
7954         // Save ^D for next time, to make sure
7955         // caller gets a 0-byte result.
7956         input.r--;
7957       }
7958       break;
7959     }
7960     *dst++ = c;
7961     --n;
7962     if(c == '\n')
7963       break;
7964   }
7965   release(&cons.lock);
7966   ilock(ip);
7967 
7968   return target - n;
7969 }
7970 
7971 int
7972 consolewrite(struct inode *ip, char *buf, int n)
7973 {
7974   int i;
7975 
7976   iunlock(ip);
7977   acquire(&cons.lock);
7978   for(i = 0; i < n; i++)
7979     consputc(buf[i] & 0xff);
7980   release(&cons.lock);
7981   ilock(ip);
7982 
7983   return n;
7984 }
7985 
7986 void
7987 consoleinit(void)
7988 {
7989   initlock(&cons.lock, "console");
7990 
7991   devsw[CONSOLE].write = consolewrite;
7992   devsw[CONSOLE].read = consoleread;
7993   cons.locking = 1;
7994 
7995   ioapicenable(IRQ_KBD, 0);
7996 }
7997 
7998 
7999 
