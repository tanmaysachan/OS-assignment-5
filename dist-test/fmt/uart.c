8000 // Intel 8250 serial port (UART).
8001 
8002 #include "types.h"
8003 #include "defs.h"
8004 #include "param.h"
8005 #include "traps.h"
8006 #include "spinlock.h"
8007 #include "sleeplock.h"
8008 #include "fs.h"
8009 #include "file.h"
8010 #include "mmu.h"
8011 #include "proc.h"
8012 #include "x86.h"
8013 
8014 #define COM1    0x3f8
8015 
8016 static int uart;    // is there a uart?
8017 
8018 void
8019 uartinit(void)
8020 {
8021   char *p;
8022 
8023   // Turn off the FIFO
8024   outb(COM1+2, 0);
8025 
8026   // 9600 baud, 8 data bits, 1 stop bit, parity off.
8027   outb(COM1+3, 0x80);    // Unlock divisor
8028   outb(COM1+0, 115200/9600);
8029   outb(COM1+1, 0);
8030   outb(COM1+3, 0x03);    // Lock divisor, 8 data bits.
8031   outb(COM1+4, 0);
8032   outb(COM1+1, 0x01);    // Enable receive interrupts.
8033 
8034   // If status is 0xFF, no serial port.
8035   if(inb(COM1+5) == 0xFF)
8036     return;
8037   uart = 1;
8038 
8039   // Acknowledge pre-existing interrupt conditions;
8040   // enable interrupts.
8041   inb(COM1+2);
8042   inb(COM1+0);
8043   ioapicenable(IRQ_COM1, 0);
8044 
8045   // Announce that we're here.
8046   for(p="xv6...\n"; *p; p++)
8047     uartputc(*p);
8048 }
8049 
8050 void
8051 uartputc(int c)
8052 {
8053   int i;
8054 
8055   if(!uart)
8056     return;
8057   for(i = 0; i < 128 && !(inb(COM1+5) & 0x20); i++)
8058     microdelay(10);
8059   outb(COM1+0, c);
8060 }
8061 
8062 static int
8063 uartgetc(void)
8064 {
8065   if(!uart)
8066     return -1;
8067   if(!(inb(COM1+5) & 0x01))
8068     return -1;
8069   return inb(COM1+0);
8070 }
8071 
8072 void
8073 uartintr(void)
8074 {
8075   consoleintr(uartgetc);
8076 }
8077 
8078 
8079 
8080 
8081 
8082 
8083 
8084 
8085 
8086 
8087 
8088 
8089 
8090 
8091 
8092 
8093 
8094 
8095 
8096 
8097 
8098 
8099 
