#include "types.h"
#include "user.h"
#include "stat.h"

int main()
{
  volatile int i = 0, a = 0;
  for(i = 0; i < 1000000000; i++){
    a += 3;
  }
  printf(1, "%d\n", a);
  exit();
}
