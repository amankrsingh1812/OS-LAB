#include "types.h"
#include "user.h"

#define PGSIZE 4096

void child_process(int i) 
{
    // int pid= getpid();
    for (int j=1; j<=20; j++) 
    {
        // printf(1,"malloc called: %d\n",pid);
        char *ptr = (char *)malloc(PGSIZE);
        for (int k=0; k<PGSIZE; k++)
        {
            ptr[k] = (i + j*k)%128;
            // printf(1, "Value@ %d %d %d %d\n", i,j,k,ptr[k]);
        
        }
        for (int k=0; k<PGSIZE; k++)
            if (ptr[k] != (i + j*k)%128)
                printf(1, "Error@ %d %d %d %d\n", i,j,k,ptr[k]);
    }
    printf(1, "CHILD: %d\n", i);
    exit();
}

int main(int argc, char *argv[])
{    
    for (int i=1; i<=20; i++) {
        if (!fork()) {
            child_process(i);
        }
    } 
    
    for (int i=1; i<=20; i++)
        wait();
    
    openFilecount();
    exit();
}