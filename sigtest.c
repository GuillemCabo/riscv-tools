#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include "encoding.h"

/* User program experimenting with handling tag exceptions */

#define ILL_TAGEXC (9)

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

void tag_handler(int signum, siginfo_t *info, void *ptr) {
   if (((info->si_code)&ILL_TAGEXC) == ILL_TAGEXC) {
      int *fault_address = info->si_addr;
      int fault_instruction = *fault_address;
      unsigned long utagctrl = read_csr(utagctrl);
      unsigned int opcode = fault_instruction & 0b1111111;
      printf("Current utagctrl: %lx\n", read_csr(utagctrl));

      if ((opcode == 0x3) && ((utagctrl & TMASK_LOAD_CHECK) == TMASK_LOAD_CHECK)) {
         printf("load tag exception\n");
      }

      write_csr(utagctrl, 0);
      printf("Faulting address: %p\n", fault_address);
      printf("Faulting instruction: %x\n", fault_instruction);

   }
   else {
      // go thru the signal again, but use the default signal handler
      struct sigaction act;
      act.sa_handler = SIG_DFL;
      act.sa_flags = 0;
      sigaction(SIGILL, &act, NULL);
   }
}

int main(void) {
  uint64_t utagctrl_val = read_csr(utagctrl);
  struct sigaction act;
  printf("utagctrl at entry: 0x%lx\n", utagctrl_val);
  utagctrl_val = TMASK_STORE_PROP | TMASK_LOAD_PROP | TMASK_LOAD_CHECK;
  write_csr(utagctrl, utagctrl_val);
  utagctrl_val = read_csr(utagctrl);
  printf("utagctrl: 0x%lx\n", utagctrl_val);

  printf("Performing a write syscall\n");
  // ensure the syscall happens by flushing stdout, shouldn't be necessary if 
  // stdout it connected to a terminal
  fflush(stdout);


  // setup signal handler
  act.sa_sigaction = tag_handler;
  act.sa_flags = SA_SIGINFO;

  //sigaction(SIGILL, &act, NULL);

  // Set a tag on a memory location
  uint64_t stack_slot = -1;
  set_tagged_val(&stack_slot, 1337, 3);
  uint64_t read_val;
  printf("About to get tagged val\n");
  //sleep(1);
  //uint8_t read_tag = get_tagged_val(&stack_slot, &read_val);
  //printf("Read value 0x%lx from location 0x%p with tag 0x%x\n", read_val, &stack_slot, read_tag);

  utagctrl_val = TMASK_STORE_PROP | TMASK_LOAD_PROP | TMASK_ALU_CHECK;
  write_csr(utagctrl, utagctrl_val);

  uint64_t test = stack_slot + 1;
  printf("Got value %d\n", test);

  utagctrl_val = read_csr(utagctrl);
  printf("utagctrl at exit: 0x%lx\n", utagctrl_val);
  return 0;
}
