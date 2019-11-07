#include "types.h"
#include "stat.h"
#include "user.h"
#include "proc_stat.h"

int
toint(char* str)
{
    int i;
    int ans = 0;
    int pos;
    for (i = 0;; i++) {
        if (str[i] == '\0') {
            pos = i;
            break;
        }
    }
    int ex = 1;
    for (i = pos-1; i >= 0; i--) {
        ans += (str[i] - '0') * ex;
        ex *= 10;
    }
    return ans;
}

int
main(int argc, char *argv[])
{
  /* int pid = toint(argv[1]); */
  /* printf(1, "**%d**\n", pid); */
  /* int wtime = 0, rtime = 0; */
  setpriority(1);
  int j;
  for (j = 0; j < 10; j++) {
    int pid = fork();
    if(pid < 0){
      printf(1, "fork failed lmao\n");
      continue;
    }
    if (pid == 0) {
        volatile int i;
        for(i = 0; i < 10000; i++){}
        setpriority(40-j);
        for(i = 0; i < 100000000; i++){
          if(i%1000 == 0){
            /* displayqueues(); */
          }
        }
        struct proc_stat ps;
        int t = getpinfo(&ps, getpid());
        if(t >= 0)printf(1, "pid %d, rtime %d, num_run %d, cur_queue %d\n", ps.pid, ps.runtime, ps.num_run, ps.current_queue);
        exit();
    } else {
      /* volatile int i; */
      /* for(i = 0; i < 10000; i++){ */
      /*   ; */
      /* } */
    }
    /* printf(1, "Rtime is: %d\n", rtime); */
    /* printf(1, "Wtime is: %d\n", wtime); */
  }
  for(j = 0; j < 10; j++) wait();
  exit();
}
