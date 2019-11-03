1100 #include "asm.h"
1101 #include "memlayout.h"
1102 #include "mmu.h"
1103 
1104 # Each non-boot CPU ("AP") is started up in response to a STARTUP
1105 # IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
1106 # Specification says that the AP will start in real mode with CS:IP
1107 # set to XY00:0000, where XY is an 8-bit value sent with the
1108 # STARTUP. Thus this code must start at a 4096-byte boundary.
1109 #
1110 # Because this code sets DS to zero, it must sit
1111 # at an address in the low 2^16 bytes.
1112 #
1113 # Startothers (in main.c) sends the STARTUPs one at a time.
1114 # It copies this code (start) at 0x7000.  It puts the address of
1115 # a newly allocated per-core stack in start-4,the address of the
1116 # place to jump to (mpenter) in start-8, and the physical address
1117 # of entrypgdir in start-12.
1118 #
1119 # This code combines elements of bootasm.S and entry.S.
1120 
1121 .code16
1122 .globl start
1123 start:
1124   cli
1125 
1126   # Zero data segment registers DS, ES, and SS.
1127   xorw    %ax,%ax
1128   movw    %ax,%ds
1129   movw    %ax,%es
1130   movw    %ax,%ss
1131 
1132   # Switch from real to protected mode.  Use a bootstrap GDT that makes
1133   # virtual addresses map directly to physical addresses so that the
1134   # effective memory map doesn't change during the transition.
1135   lgdt    gdtdesc
1136   movl    %cr0, %eax
1137   orl     $CR0_PE, %eax
1138   movl    %eax, %cr0
1139 
1140   # Complete the transition to 32-bit protected mode by using a long jmp
1141   # to reload %cs and %eip.  The segment descriptors are set up with no
1142   # translation, so that the mapping is still the identity mapping.
1143   ljmpl    $(SEG_KCODE<<3), $(start32)
1144 
1145 .code32  # Tell assembler to generate 32-bit code now.
1146 start32:
1147   # Set up the protected-mode data segment registers
1148   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
1149   movw    %ax, %ds                # -> DS: Data Segment
1150   movw    %ax, %es                # -> ES: Extra Segment
1151   movw    %ax, %ss                # -> SS: Stack Segment
1152   movw    $0, %ax                 # Zero segments not ready for use
1153   movw    %ax, %fs                # -> FS
1154   movw    %ax, %gs                # -> GS
1155 
1156   # Turn on page size extension for 4Mbyte pages
1157   movl    %cr4, %eax
1158   orl     $(CR4_PSE), %eax
1159   movl    %eax, %cr4
1160   # Use entrypgdir as our initial page table
1161   movl    (start-12), %eax
1162   movl    %eax, %cr3
1163   # Turn on paging.
1164   movl    %cr0, %eax
1165   orl     $(CR0_PE|CR0_PG|CR0_WP), %eax
1166   movl    %eax, %cr0
1167 
1168   # Switch to the stack allocated by startothers()
1169   movl    (start-4), %esp
1170   # Call mpenter()
1171   call	 *(start-8)
1172 
1173   movw    $0x8a00, %ax
1174   movw    %ax, %dx
1175   outw    %ax, %dx
1176   movw    $0x8ae0, %ax
1177   outw    %ax, %dx
1178 spin:
1179   jmp     spin
1180 
1181 .p2align 2
1182 gdt:
1183   SEG_NULLASM
1184   SEG_ASM(STA_X|STA_R, 0, 0xffffffff)
1185   SEG_ASM(STA_W, 0, 0xffffffff)
1186 
1187 
1188 gdtdesc:
1189   .word   (gdtdesc - gdt - 1)
1190   .long   gdt
1191 
1192 
1193 
1194 
1195 
1196 
1197 
1198 
1199 
