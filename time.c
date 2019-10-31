#include "types.h"
#include "stat.h"
#include "user.h"

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
  setpriority(1);
  int pid = toint(argv[1]);
  printf(1, "**%d**\n", pid);
  /* int wtime = 0, rtime = 0; */
  int j;
  for (j = 0; j < 10; j++) {
    int pid = fork();
    if (pid == 0) {
        int new_priority = j+10;
        setpriority(new_priority);
        volatile int i;
        /* printf(1, "process with priority %d chosen\n", new_priority); */
        for(i = 0; i < 100000000; i++){
            ;
        }
        exit();
    } else {
        volatile int i;
        for(i = 0; i < 1000000; i++){
            ;
        }
    }
    /* printf(1, "Rtime is: %d\n", rtime); */
    /* printf(1, "Wtime is: %d\n", wtime); */
  }
  exit();
}
