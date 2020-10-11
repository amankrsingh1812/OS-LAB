#include "types.h"
#include "user.h"
#include "processInfo.h"

void make_process(int burstTime) {
  struct processInfo pInfo;

  set_burst_time(burstTime);        
  for (volatile int i=0; i<100000000; i++);

  getProcInfo(&pInfo);    
  exit();
}

int main(int argc, char *argv[])
{

  struct processInfo pInfo_p;
  int x[5] = {4,70,2,8,23};

  for (int i=0; i<5; i++)
    if (!fork()) 
      make_process(x[i]);
  
  for (int i=0; i<5; i++)
    wait();

  getProcInfo(&pInfo_p);
	exit();
}
