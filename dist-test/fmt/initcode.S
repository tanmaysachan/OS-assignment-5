8100 # Initial process execs /init.
8101 # This code runs in user space.
8102 
8103 #include "syscall.h"
8104 #include "traps.h"
8105 
8106 
8107 # exec(init, argv)
8108 .globl start
8109 start:
8110   pushl $argv
8111   pushl $init
8112   pushl $0  // where caller pc would be
8113   movl $SYS_exec, %eax
8114   int $T_SYSCALL
8115 
8116 # for(;;) exit();
8117 exit:
8118   movl $SYS_exit, %eax
8119   int $T_SYSCALL
8120   jmp exit
8121 
8122 # char init[] = "/init\0";
8123 init:
8124   .string "/init\0"
8125 
8126 # char *argv[] = { init, 0 };
8127 .p2align 2
8128 argv:
8129   .long init
8130   .long 0
8131 
8132 
8133 
8134 
8135 
8136 
8137 
8138 
8139 
8140 
8141 
8142 
8143 
8144 
8145 
8146 
8147 
8148 
8149 
