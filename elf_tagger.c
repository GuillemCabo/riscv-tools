#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <elf.h>

/* Basic program used to produce the binary file to be used as the 
 * .tags section of an ELF binary 
 */

unsigned long get_next_target(FILE *target_file, unsigned long start_load, int *done) {
   char *target = NULL;
   size_t size_line = 0;
   size_t num_read;
   unsigned long target_addr, tag_word;

   num_read = getline(&target, &size_line, target_file);
   //printf("num read: %lu\n", num_read);
   if (num_read != -1) {
      //printf("target: %s\n", target);
      target_addr = strtol(target, NULL, 16);
      //printf("converted target: %lx\n", target_addr);
      tag_word = (target_addr)/sizeof(unsigned long);
   }
   else {
      *done = 1;
   }

   free(target);
   return tag_word;
}

int main(int argc, char **argv) {
   FILE *elf_file;
   Elf64_Ehdr elf_header;
   Elf64_Phdr *pheaders;
   Elf64_Phdr *pheader;
   int i, done = 0;
   unsigned long size = 0;
   unsigned long num_words;
   unsigned long tag_word;
   unsigned long start_load;
   unsigned long num_tags = 0;
   FILE *target_file;
   FILE *tag_file;   

   if (argc != 4) {
      printf("Provide the name of the ELF binary the output file and the branch target file\n");
      return 0;
   }

   printf("Writing tags to %s\n", argv[1]);
   elf_file = fopen(argv[1], "r");
   fread(&elf_header, 1, sizeof(Elf64_Ehdr), elf_file);
   
   fseek(elf_file, (long)(elf_header.e_phoff), SEEK_SET);
   pheaders = malloc(sizeof(Elf64_Phdr) * elf_header.e_phnum);
   fread(pheaders, sizeof(Elf64_Phdr), elf_header.e_phnum, elf_file);
   
   // get start load address
   start_load = pheaders->p_vaddr;

   // get last program header and calculate its max address
   pheader = pheaders + ((elf_header.e_phnum) - 1);
   size = pheader->p_vaddr + pheader->p_memsz;
   printf("Size: %lx, vaddr: %lx, memsz: %lx\n", size, pheader->p_vaddr, pheader->p_memsz);

   printf("Size: %lu\n", size);
   num_words = size/sizeof(unsigned long);
   tag_file = fopen(argv[2], "w");

   target_file = fopen(argv[3], "r");
   
   tag_word = get_next_target(target_file, start_load, &done);
   
   for (i = 0; i < num_words; i++) {
      if (!done && i == tag_word) {
         //printf("Writing tag to word %lx\n", i);
         num_tags++;
         fputc(0x3, tag_file);
         tag_word = get_next_target(target_file, start_load, &done);
         //printf("Next tag word: %lx\n", tag_word);
      }
      else {
         fputc(0, tag_file);
      }
   }
   printf("Num tags written: %lu\n", num_tags);
   
   return 0;
}
