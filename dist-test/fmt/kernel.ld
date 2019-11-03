9000 /* Simple linker script for the JOS kernel.
9001    See the GNU ld 'info' manual ("info ld") to learn the syntax. */
9002 
9003 OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")
9004 OUTPUT_ARCH(i386)
9005 ENTRY(_start)
9006 
9007 SECTIONS
9008 {
9009 	/* Link the kernel at this address: "." means the current address */
9010         /* Must be equal to KERNLINK */
9011 	. = 0x80100000;
9012 
9013 	.text : AT(0x100000) {
9014 		*(.text .stub .text.* .gnu.linkonce.t.*)
9015 	}
9016 
9017 	PROVIDE(etext = .);	/* Define the 'etext' symbol to this value */
9018 
9019 	.rodata : {
9020 		*(.rodata .rodata.* .gnu.linkonce.r.*)
9021 	}
9022 
9023 	/* Include debugging information in kernel memory */
9024 	.stab : {
9025 		PROVIDE(__STAB_BEGIN__ = .);
9026 		*(.stab);
9027 		PROVIDE(__STAB_END__ = .);
9028 		BYTE(0)		/* Force the linker to allocate space
9029 				   for this section */
9030 	}
9031 
9032 	.stabstr : {
9033 		PROVIDE(__STABSTR_BEGIN__ = .);
9034 		*(.stabstr);
9035 		PROVIDE(__STABSTR_END__ = .);
9036 		BYTE(0)		/* Force the linker to allocate space
9037 				   for this section */
9038 	}
9039 
9040 	/* Adjust the address for the data segment to the next page */
9041 	. = ALIGN(0x1000);
9042 
9043 	/* Conventionally, Unix linkers provide pseudo-symbols
9044 	 * etext, edata, and end, at the end of the text, data, and bss.
9045 	 * For the kernel mapping, we need the address at the beginning
9046 	 * of the data section, but that's not one of the conventional
9047 	 * symbols, because the convention started before there was a
9048 	 * read-only rodata section between text and data. */
9049 	PROVIDE(data = .);
9050 	/* The data segment */
9051 	.data : {
9052 		*(.data)
9053 	}
9054 
9055 	PROVIDE(edata = .);
9056 
9057 	.bss : {
9058 		*(.bss)
9059 	}
9060 
9061 	PROVIDE(end = .);
9062 
9063 	/DISCARD/ : {
9064 		*(.eh_frame .note.GNU-stack)
9065 	}
9066 }
9067 
9068 
9069 
9070 
9071 
9072 
9073 
9074 
9075 
9076 
9077 
9078 
9079 
9080 
9081 
9082 
9083 
9084 
9085 
9086 
9087 
9088 
9089 
9090 
9091 
9092 
9093 
9094 
9095 
9096 
9097 
9098 
9099 
