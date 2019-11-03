3800 #include "types.h"
3801 #include "x86.h"
3802 #include "defs.h"
3803 #include "date.h"
3804 #include "param.h"
3805 #include "memlayout.h"
3806 #include "mmu.h"
3807 #include "proc.h"
3808 
3809 int
3810 sys_fork(void)
3811 {
3812   return fork();
3813 }
3814 
3815 int
3816 sys_exit(void)
3817 {
3818   exit();
3819   return 0;  // not reached
3820 }
3821 
3822 int
3823 sys_wait(void)
3824 {
3825   return wait();
3826 }
3827 
3828 int
3829 sys_setpriority(void)
3830 {
3831   int p;
3832   if(argint(0, &p) < 0)
3833     return -1;
3834   return setpriority(p);
3835 }
3836 
3837 int
3838 sys_waitx(void)
3839 {
3840   int* wtime;
3841   int* rtime;
3842   if(argptr(0, (void*)&wtime, sizeof(int)) || argptr(1, (void*)&rtime, sizeof(int)))
3843     return -1;
3844   return waitx((int*)wtime, (int*)rtime);
3845 }
3846 
3847 
3848 
3849 
3850 int
3851 sys_kill(void)
3852 {
3853   int pid;
3854 
3855   if(argint(0, &pid) < 0)
3856     return -1;
3857   return kill(pid);
3858 }
3859 
3860 int
3861 sys_getpid(void)
3862 {
3863   return myproc()->pid;
3864 }
3865 
3866 int
3867 sys_sbrk(void)
3868 {
3869   int addr;
3870   int n;
3871 
3872   if(argint(0, &n) < 0)
3873     return -1;
3874   addr = myproc()->sz;
3875   if(growproc(n) < 0)
3876     return -1;
3877   return addr;
3878 }
3879 
3880 int
3881 sys_sleep(void)
3882 {
3883   int n;
3884   uint ticks0;
3885 
3886   if(argint(0, &n) < 0)
3887     return -1;
3888   acquire(&tickslock);
3889   ticks0 = ticks;
3890   while(ticks - ticks0 < n){
3891     if(myproc()->killed){
3892       release(&tickslock);
3893       return -1;
3894     }
3895     sleep(&ticks, &tickslock);
3896   }
3897   release(&tickslock);
3898   return 0;
3899 }
3900 // return how many clock tick interrupts have occurred
3901 // since start.
3902 int
3903 sys_uptime(void)
3904 {
3905   uint xticks;
3906 
3907   acquire(&tickslock);
3908   xticks = ticks;
3909   release(&tickslock);
3910   return xticks;
3911 }
3912 
3913 
3914 
3915 
3916 
3917 
3918 
3919 
3920 
3921 
3922 
3923 
3924 
3925 
3926 
3927 
3928 
3929 
3930 
3931 
3932 
3933 
3934 
3935 
3936 
3937 
3938 
3939 
3940 
3941 
3942 
3943 
3944 
3945 
3946 
3947 
3948 
3949 
