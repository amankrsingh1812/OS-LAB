#include "types.h"
#include "user.h"
#include "processInfo.h"

int main(int argc, char *argv[]){
    struct processInfo pi;
    for(int pid=1;pid<=64;pid++){
        if(getProcInfo(pid, &pi) != -1){
            printf(1,"PID: %d - \n %d %d %d\n\n", pid, pi.ppid, pi.psize, pi.numberContextSwitches);
        }
    }
    exit();
}