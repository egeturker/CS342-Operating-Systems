

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "sbmem.h"

int main()
{
    int i, ret; 
    char *p;  	

    ret = sbmem_open(); 
    if (ret == -1)
	exit (1); 

    p = sbmem_alloc (256); // allocate space to hold 1024 characters
   // for (i = 0; i < 256; ++i)
	//    p[i] = 'b'; // init all chars to ‘a’
    //for (i = 0; i < 256; ++i)
	//    printf("%d,%c\n",i,p[i]);

 //   sbmem_free (p);

    sbmem_close(); 
    
    return (0); 
}