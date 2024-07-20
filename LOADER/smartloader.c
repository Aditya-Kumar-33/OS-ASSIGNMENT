#include <stdio.h>
#include <elf.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <signal.h>
#include <stdint.h>
#include <math.h>

Elf32_Ehdr *ehdr; // Elf32_Ehdr structure represents the ELF header and ehdr is pointer to it
Elf32_Phdr *phdr;
int fd;
int n_segmentation_faults = 0;
int n_page_allocations = 0;
int total_internal_fragmentation = 0;
// to  keep a  track of  no of segmentation faults

typedef int (*startFunctionPtr)();
startFunctionPtr new_start;
int termination_flag = 0;
// to keep a track when the process has  finished executing;
struct node
{
  unsigned int x;
  void *y;
  Elf32_Phdr *z;
  struct node *next;
  unsigned int mem_allocated;
};
// initializing a linked list
// this linked list will be later used in cleanup function

struct node *head = NULL;

void loader_cleanup()
{

  struct node *current = head;

  while (current != NULL)
  {
    struct node *temp = current;
    current = current->next;
    if (munmap(temp->y, temp->mem_allocated) == -1)
    {
      perror("munmap failed");
    }
    free(temp->z);
    free(temp);
  }

  free(ehdr);
  close(fd);
}

void load_and_run_elf(char **exe)
{
  fd = open(exe[1], O_RDONLY); // Open the ELF file specified in the first element of the 'exe' array
  // error handling for executable

  if (fd < 0) // file handling error
  {
    perror("error in opening file");
    assert(fd != -1);
    return;
  }

  ehdr = (Elf32_Ehdr *)malloc(sizeof(Elf32_Ehdr)); // ELF header
  // error handling for  malloc
  if (ehdr == NULL)
  {
    perror("malloc failed for ehdr");
    exit(1);
  }

  assert(ehdr != NULL); // error handling for malloc

  read(fd, ehdr, sizeof(Elf32_Ehdr)); // to read ELF header data from a fd into pointer ehdr

  new_start = ehdr->e_entry;
  // initialized our typecasted address to e_entry
  // likely to generate error since no segment loaded
  // this segmentation fault will caught by our handler

  int result = new_start();
  // calling the start function
  // this will generate page fault

  printf("User _start return value = %d\n", result);
  printf("--------------------------------------\n");
  printf("Number of Page_faults:%d\n", n_segmentation_faults);
  printf("N_page_Allocations:%d\n", n_page_allocations);
  printf("Total Internal_Fragmentaion(KB):%.3f KB\n", total_internal_fragmentation / 1024.0);
  printf("--------------------------------------\n");
}

void handleSegmentationFault(void *segmentation_address)
{
  unsigned int entry = ehdr->e_entry;
  // entry asigned to e_entry from elf header pointer ehdr
  if (segmentation_address != NULL)
  {
    entry = segmentation_address;
    // this sets entry to segmentation address which caused the page fault
    // entry is just a variable
  }

  unsigned int phoff = ehdr->e_phoff;         // segment offset
  unsigned short ph_size = ehdr->e_phentsize; // segment size
  unsigned short ph_num = ehdr->e_phnum;      // total number of segments

  phdr = (Elf32_Phdr *)((char *)ehdr + ehdr->e_phoff);

  Elf32_Phdr *temp_ph = (Elf32_Phdr *)malloc(sizeof(Elf32_Phdr));
  // assigning a temp variable  to  iterate among phdrs
  if (temp_ph == NULL)
  {
    perror("malloc allocation failed");
    exit(1);
  }

  // void virtual_mem; // pointer void is special type that can hold address of any data type, (generic pointer type)
  // iterate as many times as number of segment
  for (int i = 0; i < ph_num; i++)
  {
    lseek(fd, phoff + i * ph_size, SEEK_SET);
    // this takes the filepointer to desired address
    read(fd, temp_ph, sizeof(Elf32_Phdr));
    // reads certain bytes from the file

    unsigned int vaddr = temp_ph->p_vaddr;
    unsigned int memsz = temp_ph->p_memsz;

    if (temp_ph->p_type == 1)
    {
      if (entry >= vaddr && entry <= (vaddr + memsz))
      {
        // checking whether that adddress lies in the segment
        // if it lies then that segment is loaded into the memory

        n_page_allocations += ceil(memsz / 4096.0);
        // to  ensure that no of page allocation are multiples of 4kb
        unsigned int mem_to_allocate = ceil(memsz / 4096.0) * 4096;
        // converting the memory i.e multiple of 4kb to bytes
        total_internal_fragmentation += (mem_to_allocate - memsz);
        // calculating internal fragmentation

        // void *virtual_mem = mmap(vaddr, temp_ph->p_memsz, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, fd, 0);
        void *virtual_mem = mmap(vaddr, mem_to_allocate, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, fd, 0);

        if (virtual_mem == MAP_FAILED)
        {
          perror("mmap failed");
          return;
        }
        struct node *newnode = (struct node *)malloc(sizeof(struct node));
        if(newnode==NULL){
          perror("malloc failed for node in linkedlist");
          exit(1);
        }
        newnode->x = temp_ph->p_memsz;
        
        newnode->y = virtual_mem;
        newnode->z = temp_ph;
        newnode->mem_allocated=mem_to_allocate;
        newnode->next = head;
        head = newnode;
        // adding in linked list to later  munmap while iterating through the linkedlist
        // this cleanup happens in cleanuup function

        lseek(fd, temp_ph->p_offset, SEEK_SET);
        read(fd, virtual_mem, temp_ph->p_filesz);

        

        break;
      }
    }
  }
}

static void my_handler(int signum, siginfo_t *info, void *context)
{
  n_segmentation_faults++;
  //  to keep a count of no of pagefaults

  if (signum == SIGSEGV)
  {

    void *segmentation_address = (void *)info->si_addr;
    // this passes the address which caused the segmentation error
    handleSegmentationFault(segmentation_address);

    
  }
}

int main(int argc, char **argv)
{

  // do error handling for argv  etc

  struct sigaction sig;
  memset(&sig, 0, sizeof(sig));
  sig.sa_handler = my_handler;
  sig.sa_flags = SA_SIGINFO;

  if (sigaction(SIGSEGV, &sig, NULL) == -1) // checking if sigaction returns -1, handle any errors in setting the signal handler
  {
    perror("sigaction error");
    return 1;
  }

  // 1. carry out necessary checks on the input ELF file
  if (argc != 2)
  {
    printf("Usage: %s <ELF Executable> \n", argv[0]);
    exit(1);
  }

  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);

  // 3. invoke the cleanup routine inside the loader
  loader_cleanup();

  return 0;
}