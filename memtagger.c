#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include "encoding.h"

/* Basic program used to check that pages are zeroed before being allocated
 * to another process 
 */

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
   uint64_t *buffer;
   int count;
   pid_t pid;

   if (argc <= 1) {
      printf("Enter number of pages to tag\n");
      return 0;
   }
   count = atoi(argv[1]);
   
   fflush(stdout);
   pid = fork();

   if (pid == -1) {
      printf("Failed to fork\n");
   }
   else if (pid == 0) {
      printf("Child marking memory\n");
      write_csr(utagctrl, TMASK_STORE_PROP | TMASK_LOAD_PROP);
      for (int i = 0; i < count; i++) {
         if (i%10000 == 0) {
            printf("On iteration %d\n", i);
         }
         buffer = malloc(4096);
         for (int j = 0; j < RISCV_PGSIZE/sizeof(uint64_t); j++) {
            set_tagged_val(buffer + j, 0xfeedfeed, 4);
         }
      }

      printf("Child finished. Exiting...\n");
      return 0;
   }
   else {
      int status;
      waitpid(pid, &status, 0);
   }

   printf("Back in parent\n");
   // All of memory should be tagged now?
   write_csr(utagctrl, TMASK_STORE_CHECK | TMASK_LOAD_PROP);
   printf("About to malloc\n");
   buffer = malloc(4096);
   if (buffer == NULL) {
      printf("Failed to malloc\n");
   }
   printf("malloc'ed\n");
   uint64_t val;
   uint8_t tag = get_tagged_val(buffer, &val);
   printf("Got tag %x on %llx at %p\n", tag, val, buffer);
   *buffer = 0xdeaddead;
   
   printf("Finished. Exiting...\n");
   return 0;
}
