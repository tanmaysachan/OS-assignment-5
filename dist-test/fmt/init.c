8200 // init: The initial user-level program
8201 
8202 #include "types.h"
8203 #include "stat.h"
8204 #include "user.h"
8205 #include "fcntl.h"
8206 
8207 char *argv[] = { "sh", 0 };
8208 
8209 int
8210 main(void)
8211 {
8212   int pid, wpid;
8213 
8214   if(open("console", O_RDWR) < 0){
8215     mknod("console", 1, 1);
8216     open("console", O_RDWR);
8217   }
8218   dup(0);  // stdout
8219   dup(0);  // stderr
8220 
8221   for(;;){
8222     printf(1, "init: starting sh\n");
8223     pid = fork();
8224     if(pid < 0){
8225       printf(1, "init: fork failed\n");
8226       exit();
8227     }
8228     if(pid == 0){
8229       exec("sh", argv);
8230       printf(1, "init: exec sh failed\n");
8231       exit();
8232     }
8233     while((wpid=wait()) >= 0 && wpid != pid)
8234       printf(1, "zombie!\n");
8235   }
8236 }
8237 
8238 
8239 
8240 
8241 
8242 
8243 
8244 
8245 
8246 
8247 
8248 
8249 
