#include "types.h"
#include "user.h"
#include "processInfo.h"

// Loop "loopfac" number of times
void looper(int bt, int loopfac) {
  set_burst_time(bt);        
  struct processInfo pInfo;

  if (loopfac>5) 
    loopfac = 5;

  for (volatile int l=0; l<loopfac; l++)
    for (volatile int i=0; i<100000000; i++)
      ;

  getProcInfo(&pInfo);    
  exit();
}

// User IO
void userIO(int bt)
{
  set_burst_time(bt);
  struct processInfo pInfo;

  char buf[256];
  printf(1, "Enter for user IO: ");
  int amt = read(0, buf, 256);
  buf[amt] = 0;
  printf(1, "So you entered: %s", buf);

  getProcInfo(&pInfo); 
  exit();
}

// File IO
void fileIO(int bt)
{
  set_burst_time(bt);
  struct processInfo pInfo;

  int fd = open("README", 0);
  char buf[1500];
  read(fd, buf, 1500);
  buf[1499] = 0;
  printf(1, "1500 Words of README: \n%s\n---------\n", buf);

  getProcInfo(&pInfo); 
  exit();
}


int main(int argc, char *argv[])
{

  struct processInfo pInfo_p;
  
  if ( !fork() )  looper(6,2);
  if ( !fork() )  userIO(1);
  if ( !fork() )  looper(10,4);
  if ( !fork() )  fileIO(3);
  if ( !fork() )  looper(8,1);  

  for (int i=0; i<5; i++)
    wait();

  getProcInfo(&pInfo_p);
	exit();
}

/*

Expected order : 4, 8, 6, 7, 5

Priority order : 5, 7, 4, 8, 6
Is it blocking : *, *, -, -, -

Observer order : 4, 7, 8, 6, 5

*/


