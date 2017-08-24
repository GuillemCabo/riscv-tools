#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "encoding.h"

/* Program meant to test swapping. Forks a process that eats a lot of 
 * memory and then parent process tries to access its memory
 */

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

#define NUM_PARENT_PAGES 1000

int main(int argc, char **argv) {
   char *buffer;
   int count;
   char *save[NUM_PARENT_PAGES];
   pid_t pid;
   
   if (argc <= 1) {
      printf("Enter number of pages to consume\n");
      return 0;
   }
   srand(1);
   count = atoi(argv[1]);

   printf("Allocating parent pages\n");
   // have the parent map pages to swapout
   for (int i = 0; i < NUM_PARENT_PAGES; i++) {
      save[i] = malloc(4096);
      if (save[i]  == NULL) {
         printf("Map failed on iteration %d\n", i);
         perror("");
         return 0;
      }

      memset(save[i], rand(), 4096);
      write_csr(utagctrl, TMASK_STORE_PROP | TMASK_LOAD_PROP);
      uint64_t write_val = 0xfeedfeed; 
      set_tagged_val((uint64_t *)(save[i]), write_val, 1);
      uint64_t read_val;
      uint8_t read_tag = get_tagged_val((uint64_t *)(save[i]), &read_val);
      if (read_tag != 1 || read_val != write_val) {
         printf("Parent writing tag error\n");
         return 0;
      }
      write_csr(utagctrl, 0);
   }

   // fork a child to consume a bunch of memory and force swapout of parent's pages
   fflush(stdout);
   pid = fork();
   if (pid == -1) {
      printf("Failed to fork\n");
   }
   // we're the child
   else if (pid == 0) {
      int size = (count/10000) + 1;
      char *sample[size];
      
      
      if (count == 0) {
         printf("No child pages to allocate\n");
         return 0;
      }

      printf("Child allocating %d pages\n", count);

      for (int i = 0; i < count; i++) {
         buffer = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
         if (buffer == MAP_FAILED) {
            printf("Map failed on iteration %d\n", i);
            perror("");
            return 0;
         }

         if (i%10000 == 0) {
            printf("On iteration %d\n", i);
            sample[i/10000] = buffer;
         }
         
         memset(buffer, rand(), 4096);

         write_csr(utagctrl, TMASK_STORE_PROP | TMASK_LOAD_PROP);
         uint64_t write_val = 0xdeadbeef; 
         set_tagged_val((uint64_t *)buffer, write_val, 4);
         uint64_t read_val;
         uint8_t read_tag = get_tagged_val((uint64_t *)buffer,  &read_val);
         if (read_tag != 4 || read_val != write_val) {
            printf("Child writing tag error\n");
            return 0;
         }
         write_csr(utagctrl, 0);

      }
      printf("Finished allocating\n");
      printf("Child accessing pages\n");
      for (int i = 0; i < size; i++) {
         write_csr(utagctrl, TMASK_STORE_PROP | TMASK_LOAD_PROP);
         uint64_t read_val;
         uint8_t read_tag = get_tagged_val((uint64_t *)sample[i], &read_val);
         if (read_tag != 4 || read_val != 0xdeadbeef) {
            printf("Child reading tag error. Got %x on %llu at %d\n", read_tag, read_val, i);
            return 0;
         }
         write_csr(utagctrl, 0);
         
      }
      printf("Child finished. Sleeping\n");
      sleep(1000);
      return 0;
   }
   // if we're the parent, wait for the child to force our pages out
   else {
      int status;
      waitpid(pid, &status, 0);
   }
   
   printf("Back in parent\n");


   while (1) {
      printf("Accessing mapped pages\n");
      // try and access some of our mapped pages so they're paged back in
      for (int i = 0; i < NUM_PARENT_PAGES; i++) {
         write_csr(utagctrl, TMASK_STORE_PROP | TMASK_LOAD_PROP);
         uint64_t read_val;
         uint8_t read_tag = get_tagged_val((uint64_t *)save[i], &read_val);
         if (read_tag != 1 || read_val != 0xfeedfeed) {
            printf("Parent reading tag error\n");
            return 0;
         }
         write_csr(utagctrl, 0);
      }

      pause();
   }

   return 0;
}
