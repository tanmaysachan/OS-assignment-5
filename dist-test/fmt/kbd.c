7650 #include "types.h"
7651 #include "x86.h"
7652 #include "defs.h"
7653 #include "kbd.h"
7654 
7655 int
7656 kbdgetc(void)
7657 {
7658   static uint shift;
7659   static uchar *charcode[4] = {
7660     normalmap, shiftmap, ctlmap, ctlmap
7661   };
7662   uint st, data, c;
7663 
7664   st = inb(KBSTATP);
7665   if((st & KBS_DIB) == 0)
7666     return -1;
7667   data = inb(KBDATAP);
7668 
7669   if(data == 0xE0){
7670     shift |= E0ESC;
7671     return 0;
7672   } else if(data & 0x80){
7673     // Key released
7674     data = (shift & E0ESC ? data : data & 0x7F);
7675     shift &= ~(shiftcode[data] | E0ESC);
7676     return 0;
7677   } else if(shift & E0ESC){
7678     // Last character was an E0 escape; or with 0x80
7679     data |= 0x80;
7680     shift &= ~E0ESC;
7681   }
7682 
7683   shift |= shiftcode[data];
7684   shift ^= togglecode[data];
7685   c = charcode[shift & (CTL | SHIFT)][data];
7686   if(shift & CAPSLOCK){
7687     if('a' <= c && c <= 'z')
7688       c += 'A' - 'a';
7689     else if('A' <= c && c <= 'Z')
7690       c += 'a' - 'A';
7691   }
7692   return c;
7693 }
7694 
7695 void
7696 kbdintr(void)
7697 {
7698   consoleintr(kbdgetc);
7699 }
