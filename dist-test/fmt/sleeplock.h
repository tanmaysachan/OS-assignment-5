4000 // Long-term locks for processes
4001 struct sleeplock {
4002   uint locked;       // Is the lock held?
4003   struct spinlock lk; // spinlock protecting this sleep lock
4004 
4005   // For debugging:
4006   char *name;        // Name of lock.
4007   int pid;           // Process holding lock
4008 };
4009 
4010 
4011 
4012 
4013 
4014 
4015 
4016 
4017 
4018 
4019 
4020 
4021 
4022 
4023 
4024 
4025 
4026 
4027 
4028 
4029 
4030 
4031 
4032 
4033 
4034 
4035 
4036 
4037 
4038 
4039 
4040 
4041 
4042 
4043 
4044 
4045 
4046 
4047 
4048 
4049 
