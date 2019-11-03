6900 // See MultiProcessor Specification Version 1.[14]
6901 
6902 struct mp {             // floating pointer
6903   uchar signature[4];           // "_MP_"
6904   void *physaddr;               // phys addr of MP config table
6905   uchar length;                 // 1
6906   uchar specrev;                // [14]
6907   uchar checksum;               // all bytes must add up to 0
6908   uchar type;                   // MP system config type
6909   uchar imcrp;
6910   uchar reserved[3];
6911 };
6912 
6913 struct mpconf {         // configuration table header
6914   uchar signature[4];           // "PCMP"
6915   ushort length;                // total table length
6916   uchar version;                // [14]
6917   uchar checksum;               // all bytes must add up to 0
6918   uchar product[20];            // product id
6919   uint *oemtable;               // OEM table pointer
6920   ushort oemlength;             // OEM table length
6921   ushort entry;                 // entry count
6922   uint *lapicaddr;              // address of local APIC
6923   ushort xlength;               // extended table length
6924   uchar xchecksum;              // extended table checksum
6925   uchar reserved;
6926 };
6927 
6928 struct mpproc {         // processor table entry
6929   uchar type;                   // entry type (0)
6930   uchar apicid;                 // local APIC id
6931   uchar version;                // local APIC verison
6932   uchar flags;                  // CPU flags
6933     #define MPBOOT 0x02           // This proc is the bootstrap processor.
6934   uchar signature[4];           // CPU signature
6935   uint feature;                 // feature flags from CPUID instruction
6936   uchar reserved[8];
6937 };
6938 
6939 struct mpioapic {       // I/O APIC table entry
6940   uchar type;                   // entry type (2)
6941   uchar apicno;                 // I/O APIC id
6942   uchar version;                // I/O APIC version
6943   uchar flags;                  // I/O APIC flags
6944   uint *addr;                  // I/O APIC address
6945 };
6946 
6947 
6948 
6949 
6950 // Table entry types
6951 #define MPPROC    0x00  // One per processor
6952 #define MPBUS     0x01  // One per bus
6953 #define MPIOAPIC  0x02  // One per I/O APIC
6954 #define MPIOINTR  0x03  // One per bus interrupt source
6955 #define MPLINTR   0x04  // One per system interrupt source
6956 
6957 // Blank page.
6958 
6959 
6960 
6961 
6962 
6963 
6964 
6965 
6966 
6967 
6968 
6969 
6970 
6971 
6972 
6973 
6974 
6975 
6976 
6977 
6978 
6979 
6980 
6981 
6982 
6983 
6984 
6985 
6986 
6987 
6988 
6989 
6990 
6991 
6992 
6993 
6994 
6995 
6996 
6997 
6998 
6999 
