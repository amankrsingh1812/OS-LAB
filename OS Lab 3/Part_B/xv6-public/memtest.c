#include "types.h"
#include "user.h"

#define PGSIZE 4096*2

void child_process(int i) 
{
    for (int j=1; j<=10; j++) 
    {
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
    
    exit();
}