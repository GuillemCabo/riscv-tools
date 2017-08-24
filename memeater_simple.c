#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "encoding.h"

/* Simple program to consume a lot of memory */

uint8_t get_tagged_val(uint64_t *src, uint64_t *out) {
  uint8_t tag;
  uint64_t val;
  asm volatile( "ld %0, 0(%2); tagr %1, %0; tagw %0, zero;" : "=r" (val), "=r" (tag) : "r" (src));
  *out = val;
  return tag;
}

void set_tagged_val(uint64_t *dst, uint64_t val, uint8_t tag) {
  asm volatile ( "tagw %0, %1; sd %0, 0(%2); tagw %0, zero;" : : "r" (val), "r" (tag), "r" (dst));
}

int main(int argc, char **argv) {
   char *buffer;
   int count;
   
   if (argc <= 1) {
      printf("Enter number of pages to consume\n");
      return 0;
   }
   srand(1);
   count = atoi(argv[1]);

   printf("Allocating %d pages\n", count);
   for (int i = 0; i < count; i++) {
      if (i%10000 == 0) {
         printf("On iteration %d\n", i);
      }
      buffer = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
      if (buffer == MAP_FAILED) {
         printf("Map failed on iteration %d\n", i);
         perror("");
         return 0;
      }
      
      memset(buffer, rand(), 4096);
      write_csr(utagctrl, TMASK_STORE_PROP | TMASK_LOAD_PROP);
      uint64_t write_val = 0xfeedfeed; 
      set_tagged_val((uint64_t *)buffer, write_val, 3);
      uint64_t read_val;
      uint8_t read_tag = get_tagged_val((uint64_t *)buffer, &read_val);
      if (read_tag != 3 || read_val != write_val) {
         printf("Tag error\n");
         return 0;
      }
      write_csr(utagctrl, 0);
   }
   printf("Finished allocating. Sleeping...\n");
   sleep(10000);
      
   return 0;
}
