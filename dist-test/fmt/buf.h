3950 struct buf {
3951   int flags;
3952   uint dev;
3953   uint blockno;
3954   struct sleeplock lock;
3955   uint refcnt;
3956   struct buf *prev; // LRU cache list
3957   struct buf *next;
3958   struct buf *qnext; // disk queue
3959   uchar data[BSIZE];
3960 };
3961 #define B_VALID 0x2  // buffer has been read from disk
3962 #define B_DIRTY 0x4  // buffer needs to be written to disk
3963 
3964 
3965 
3966 
3967 
3968 
3969 
3970 
3971 
3972 
3973 
3974 
3975 
3976 
3977 
3978 
3979 
3980 
3981 
3982 
3983 
3984 
3985 
3986 
3987 
3988 
3989 
3990 
3991 
3992 
3993 
3994 
3995 
3996 
3997 
3998 
3999 
