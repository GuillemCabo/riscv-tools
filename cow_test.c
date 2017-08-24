#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include "encoding.h"

/* Basic test to check that tags are copied on COW pages */

void set_tagged_val(uint64_t *dst, uint64_t val, uint8_t tag) {
  asm volatile ( "tagw %0, %1; sd %0, 0(%2); tagw %0, zero;" : : "r" (val), "r" (tag), "r" (dst));
}

uint8_t get_tagged_val(uint64_t *src, uint64_t *out) {
  uint8_t tag;
  uint64_t val;
  asm volatile( "ld %0, 0(%2); tagr %1, %0; tagw %0, zero;" : "=r" (val), "=r" (tag) : "r" (src));
  *out = val;
  return tag;
}

int main(int argc, char **argv) {
   unsigned long value = 0xfeedfeed;
   pid_t pid;
   
   write_csr(utagctrl, TMASK_LOAD_PROP | TMASK_STORE_PROP);
   set_tagged_val(&value, 0xfeedfeed, 4);
   printf("About to fork\n");
   fflush(stdout);
   pid = fork();
   if (pid == -1) {
      printf("Failed to fork\n");
      return 0;
   }
   else if (pid == 0) {
      unsigned long read_val;
      uint8_t tag = get_tagged_val(&value, &read_val);
      printf("Child got value: %lx with tag %x\n", value, tag);
      printf("Child writing to value\n");
      set_tagged_val(&value, 0xdeadbeef, 1);
      tag = get_tagged_val(&value, &read_val);
      printf("Child value: %lx, tag: %x\n", value, tag);
      return 0;
   }   
   else {
      int status;
      waitpid(pid, &status, 0);
   }
   
   unsigned long out_val;
   uint8_t parent_tag = get_tagged_val(&value, &out_val);
   
   printf("Parent value: %lx, tag: %x\n", value, parent_tag);
  
   return 0;
}

