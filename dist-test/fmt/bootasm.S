8800 #include "asm.h"
8801 #include "memlayout.h"
8802 #include "mmu.h"
8803 
8804 # Start the first CPU: switch to 32-bit protected mode, jump into C.
8805 # The BIOS loads this code from the first sector of the hard disk into
8806 # memory at physical address 0x7c00 and starts executing in real mode
8807 # with %cs=0 %ip=7c00.
8808 
8809 .code16                       # Assemble for 16-bit mode
8810 .globl start
8811 start:
8812   cli                         # BIOS enabled interrupts; disable
8813 
8814   # Zero data segment registers DS, ES, and SS.
8815   xorw    %ax,%ax             # Set %ax to zero
8816   movw    %ax,%ds             # -> Data Segment
8817   movw    %ax,%es             # -> Extra Segment
8818   movw    %ax,%ss             # -> Stack Segment
8819 
8820   # Physical address line A20 is tied to zero so that the first PCs
8821   # with 2 MB would run software that assumed 1 MB.  Undo that.
8822 seta20.1:
8823   inb     $0x64,%al               # Wait for not busy
8824   testb   $0x2,%al
8825   jnz     seta20.1
8826 
8827   movb    $0xd1,%al               # 0xd1 -> port 0x64
8828   outb    %al,$0x64
8829 
8830 seta20.2:
8831   inb     $0x64,%al               # Wait for not busy
8832   testb   $0x2,%al
8833   jnz     seta20.2
8834 
8835   movb    $0xdf,%al               # 0xdf -> port 0x60
8836   outb    %al,$0x60
8837 
8838   # Switch from real to protected mode.  Use a bootstrap GDT that makes
8839   # virtual addresses map directly to physical addresses so that the
8840   # effective memory map doesn't change during the transition.
8841   lgdt    gdtdesc
8842   movl    %cr0, %eax
8843   orl     $CR0_PE, %eax
8844   movl    %eax, %cr0
8845 
8846   # Complete the transition to 32-bit protected mode by using a long jmp
8847   # to reload %cs and %eip.  The segment descriptors are set up with no
8848   # translation, so that the mapping is still the identity mapping.
8849   ljmp    $(SEG_KCODE<<3), $start32
8850 .code32  # Tell assembler to generate 32-bit code now.
8851 start32:
8852   # Set up the protected-mode data segment registers
8853   movw    $(SEG_KDATA<<3), %ax    # Our data segment selector
8854   movw    %ax, %ds                # -> DS: Data Segment
8855   movw    %ax, %es                # -> ES: Extra Segment
8856   movw    %ax, %ss                # -> SS: Stack Segment
8857   movw    $0, %ax                 # Zero segments not ready for use
8858   movw    %ax, %fs                # -> FS
8859   movw    %ax, %gs                # -> GS
8860 
8861   # Set up the stack pointer and call into C.
8862   movl    $start, %esp
8863   call    bootmain
8864 
8865   # If bootmain returns (it shouldn't), trigger a Bochs
8866   # breakpoint if running under Bochs, then loop.
8867   movw    $0x8a00, %ax            # 0x8a00 -> port 0x8a00
8868   movw    %ax, %dx
8869   outw    %ax, %dx
8870   movw    $0x8ae0, %ax            # 0x8ae0 -> port 0x8a00
8871   outw    %ax, %dx
8872 spin:
8873   jmp     spin
8874 
8875 # Bootstrap GDT
8876 .p2align 2                                # force 4 byte alignment
8877 gdt:
8878   SEG_NULLASM                             # null seg
8879   SEG_ASM(STA_X|STA_R, 0x0, 0xffffffff)   # code seg
8880   SEG_ASM(STA_W, 0x0, 0xffffffff)         # data seg
8881 
8882 gdtdesc:
8883   .word   (gdtdesc - gdt - 1)             # sizeof(gdt) - 1
8884   .long   gdt                             # address gdt
8885 
8886 
8887 
8888 
8889 
8890 
8891 
8892 
8893 
8894 
8895 
8896 
8897 
8898 
8899 
