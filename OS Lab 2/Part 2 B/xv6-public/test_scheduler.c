#include "types.h"
#include "user.h"

int arrivalTime[3];

// Loop "loopfac" number of times
void looper(int bt, int loopfac, int idx) {
  set_burst_time(bt);  
  int tick_start = uptime();  

  if (loopfac>5) 
    loopfac = 5;

  for (volatile int l=0; l<loopfac; l++)
    for (volatile int i=0; i<10000000; i++)
      ;

  // getProcInfo();
  int tick_end = uptime();
  printf(1, "Exiting PID: %d\tArrivalUptime: %d\tCompletionUptime: %d\tTurnaroundTime: %d\tResponseUptime: %d\n"
  , getpid()
  , arrivalTime[idx]
  , tick_end
  , tick_end - arrivalTime[idx]
  , tick_start - arrivalTime[idx]
  );
  exit();
}

// User IO
void userIO(int bt)
{
  set_burst_time(bt);

  char buf[256];
  printf(1, "Enter for user IO:\n");
  int amt = read(0, buf, 256);
  buf[amt] = 0;
  printf(1, "So you entered: %s", buf);

  // getProcInfo(); 
  exit();
}

// File IO
void fileIO(int bt)
{
  set_burst_time(bt);

  int fd = open("README", 0);
  char buf[1500];
  read(fd, buf, 1500);
  buf[1499] = 0;
  // printf(1, "1500 Words of README: \n%s\n---------\n", buf);

  // getProcInfo(); 
  exit();
}


int main(int argc, char *argv[])
{

  int pid[3];
  int rpid[3];

  char exitInfoGatherer[3][70];

  arrivalTime[0] = uptime();
  if ( !(pid[0] = fork()) )  looper(8,2,0);
  arrivalTime[1] = uptime();
  if ( !(pid[1] = fork()) )  looper(16,4,1);
  arrivalTime[2] = uptime();
  if ( !(pid[2] = fork()) )  looper(4,1,2); 


  for (int i=0; i<3; i++)
    rpid[i] = wait();

  strcpy(exitInfoGatherer[0], "BurstTime: 8  - Empty loop running 2e8 times\n");
  strcpy(exitInfoGatherer[1], "BurstTime: 16 - Empty loop running 4e8 times\n");
  strcpy(exitInfoGatherer[2], "BurstTime: 4  - Empty loop running 1e8 times\n");

  printf(1, "\n******** CHILDREN EXIT ORDER SUMMARY ********\n");
  for (int i=0; i<3; i++) {
    for (int j=0; j<3; j++) {
      if (rpid[i] == pid[j])
        printf(1, exitInfoGatherer[j]);
    }
  }
  printf(1, "\n****** Summary ends, completing parent *******\n\n");

  // int pid[5];
  // int rpid[5];
  // char exitInfoGatherer[5][70];

  // if ( !(pid[0] = fork()) )  looper(6,2);
  // if ( !(pid[1] = fork()) )  userIO(1);
  // if ( !(pid[2] = fork()) )  looper(10,4);
  // if ( !(pid[3] = fork()) )  fileIO(3);
  // if ( !(pid[4] = fork()) )  looper(8,1);  

  // for (int i=0; i<5; i++)
  //   rpid[i] = wait();

  // strcpy(exitInfoGatherer[0], "BurstTime: 8  - Empty loop running 2e8 times\n");
  // strcpy(exitInfoGatherer[1], "BurstTime: 1  - Taking user IO and printing it\n");
  // strcpy(exitInfoGatherer[2], "BurstTime: 10 - Empty loop running 4e8 times\n");
  // strcpy(exitInfoGatherer[3], "BurstTime: 3  - Reading from file and printing it\n");
  // strcpy(exitInfoGatherer[4], "BurstTime: 6  - Empty loop running 1e8 times\n");

  // printf(1, "\n******** CHILDREN EXIT ORDER SUMMARY ********\n");
  // for (int i=0; i<5; i++) {
  //   for (int j=0; j<5; j++) {
  //     if (rpid[i] == pid[j])
  //       printf(1, exitInfoGatherer[j]);
  //   }
  // }
  // printf(1, "\n****** Summary ends, completing parent *******\n\n");

  // getProcInfo();
	exit();
}

/*

base_process = 3, 6

[4, 5, 6]
[5, 6, 4]
[6, 4, 5]
[4, 5, 6]
[6, 4, 5]

Expected order : 4, 8, 6, 7, 5

Priority order : 5, 7, 4, 8, 6
Is it blocking : *, *, -, -, -

Observer order : 4, 7, 8, 6, 5

*/


