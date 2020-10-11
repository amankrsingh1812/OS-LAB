#include "types.h"
#include "user.h"
#include "processInfo.h"

void disp(int n) {
  char ans[15];
	int i=0;
	while (n) {
		ans[i++] = '0'+(n%10); 
    n/=10;
  }
  ans[i++] = '\n';
	ans[i++] = '\0';
	printf(1, "Number: ");
  printf(1, ans);	
}

int main(int argc, char *argv[])
{
//  printf(1, "Wolfie Wolf - user level application\n\n");
//  char wolf[100];
//  wolfie(wolf, 100);
  
//	int cnt = getNumProc();
//	int cnt = getMaxPid();

/*
  struct processInfo pInfo;
  
  for (int i=1; i<4; i++) {
    pInfo.ppid = 9;
    int cnt = getProcInfo(i, &pInfo);

    if (cnt == -1)
      printf(1, "PID DNE\n"); 

    disp(pInfo.ppid);
    disp(pInfo.psize);
    disp(pInfo.numberContextSwitches);
  }
*/

  set_burst_time(5);
  disp(get_burst_time());

  set_burst_time(55);
  disp(get_burst_time());

	exit();
}
