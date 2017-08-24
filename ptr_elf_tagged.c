#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "encoding.h"


// Taken from 
// https://github.com/nekt/csaw_esc_2016/blob/master/tools/exploits/stack.c 
// (MIT licensed)

/* Basic program showing how tagging branch targets can protect against 
 * malicious function pointers
 */

#define disable_tag_rules() write_csr(utagctrl, 0)
#define enable_tag_rules() write_csr(utagctrl, (unsigned long)(TMASK_CFLOW_INDIR_TGT | TMASK_CFLOW_DIR_TGT | TMASK_STORE_PROP | TMASK_LOAD_PROP))

void shell(){  //never called
    printf("\n==Attack==\nLaunching date, with malice\n");
    system("/bin/date");
    exit(0);
}

void foo(char * s){
    printf("Hello from foo()\n");
    printf("Received: %s\n", s);
}

char vuln(char * s, int count){
    void (* volatile fptr) (char *); //function pointer
    char buff[16];
    
    fptr = &foo; //fptr pointer updated

    printf("vuln() has received %d bytes\n", count);
    memcpy(buff, s, count); //no bounds check

    printf("Calling foo() at %p\n", fptr);
    fptr(s);
    
    return buff[15];
}

void exploit(){
    FILE * f;
    char overflow[32];
    unsigned int offset = 16; //offset for fptr in vuln()
    long int redirect = (long int) shell; //set shell() as a redirection target
      
    memset(&overflow, 0x41, 32); //fill with character 'A'
    overflow[offset - 1] = 0;
    memcpy(overflow + offset, &redirect, 8);
    
    f = fopen("ptr.txt", "w");
    fwrite(overflow, 32, 1, f);
    fclose(f);
}

int main(int argc, char **argv){
    char count;
    FILE * f;
    char overflow[32];

    enable_tag_rules();
    
    if (argc < 2) {
        printf("Missing first argument (integer).\nUsing number 20 as the first argument seems interesting.\n Exiting...\n");
        return 0;
    }
    
    count = (char) atoi(argv[1]);
    
    if (!(count > 0)) {
        printf("Incorrect first argument (%d). Exiting...\n", count);
        return 0;    
    }
    
    if ((count > 20)) {
        printf("First argument (%d) is too large. Exiting...\n", count);
        return 0;    
    }
    
    exploit(); //helper function to prepare a malicious input file

    printf("Reading %d bytes from file\n", count);
  
    f = fopen("ptr.txt", "r");
    if (f != NULL) {
        fread(overflow, sizeof(char), count, f);
        fclose(f);
        vuln(overflow, count);
    }
    else {
        printf("Input file does not exist. Exiting...\n");
        return 0;
    }
  
  printf("Control has returned to main()\nNormal operation.\n");
  disable_tag_rules();
  return 1;
}
  
