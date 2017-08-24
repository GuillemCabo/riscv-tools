#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include "encoding.h"

int main(int argc, char **argv) {
   pid_t pid;
   unsigned long tagctrl = read_csr(utagctrl);
   
   printf("Parent tagctrl at beginning: %lx\n", tagctrl);
   write_csr(utagctrl, TMASK_ALU_PROP);
   printf("Parent tagctrl set to: %lx\n", read_csr(utagctrl));
   fflush(stdout);
   pid = fork();

   if (pid == -1) {
      printf("Failed to fork\n");
   }
   else if (pid == 0) {
      printf("Child tagctrl at beginning %lx\n", read_csr(utagctrl));
      write_csr(utagctrl, TMASK_STORE_PROP);
      printf("Child tagctrl set to %lx\n", read_csr(utagctrl));
      return 0;
   }
   else {
      int status;
      waitpid(pid, &status, 0);

      printf("Parent tagctrl after child: %lx\n", read_csr(utagctrl));
   }
   return 0;
}
