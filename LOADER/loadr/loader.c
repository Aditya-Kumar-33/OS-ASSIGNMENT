#include "loader.h"


Elf32_Ehdr *ehdr;
Elf32_Phdr *phdr;
int fd;

struct node{
  unsigned int x;
  void* y;
  Elf32_Phdr *z;
  struct node *next;
};
struct node* head = NULL;

/*
 * release memory and other cleanups
 */

void loader_cleanup() {
  

    struct node* current = head;

    while (current != NULL) {
        struct node* temp = current;
        current = current->next;
        if (munmap(temp->y,temp->x) == -1) {
          perror("munmap failed");
          }
        free(temp->z);
        free(temp);
    }
    free(ehdr);

  close(fd);

}

/*
 * Load and run the ELF executable file
 */
void load_and_run_elf(char** exe) {
  fd = open(exe[1], O_RDONLY);
  if(fd<0){
    perror("error in opening file");
    assert(fd!=-1);
    return;
  }


  // also need to close the file
  // checck for error:when fd<0{it means there is error and need to be dealt}

  struct stat st;
  long file_size;
  if((exe[1],&st)==0){
    file_size=st.st_size;
  }
  // if above doesnt work
    // Get the file size
    // fseek(binary_file, 0, SEEK_END);
    // long file_size = ftell(binary_file);
    // fseek(binary_file, 0, SEEK_SET);
  ehdr=(Elf32_Ehdr*) malloc(sizeof(Elf32_Ehdr));

  assert(ehdr!=NULL);


  read(fd,ehdr,sizeof(Elf32_Ehdr));
  
  unsigned int entry=ehdr->e_entry;
  unsigned int phoff=ehdr->e_phoff;
  unsigned short ph_size=ehdr->e_phentsize;
  unsigned short ph_num=ehdr->e_phnum;
  // phdr=(Elf32_Phdr*) malloc(sizeof(Elf32_Phdr));
  // phdr=ehdr->e_phoff;
  phdr = (Elf32_Phdr *)((char *)ehdr + ehdr->e_phoff);

  Elf32_Phdr* temp_ph=(Elf32_Phdr*) malloc(sizeof(Elf32_Phdr));


  // printf("printing e_entry: %d\n",ehdr->e_entry);
  // printf("printing again: %d\n",entry);
  // printf("phoff %p\n",phdr);
  // printf("phnum:%d\n",ph_num);
  void* virtual_mem;

  typedef int (*startFunctionPtr)();
  startFunctionPtr new_start=NULL;
  // to later type cast address to function type

  for(int i=0;i<ph_num;i++){
    lseek(fd, phoff + i * ph_size, SEEK_SET);
    read(fd,temp_ph,sizeof(Elf32_Phdr));
    // printf("ashish\n");
    // printf("within loop: %d\n",temp_ph->p_offset);
    if(temp_ph->p_type==1){
      
      // printf("this is loadable type\n");
      unsigned int vaddr=temp_ph->p_vaddr;
      unsigned int memsz=temp_ph->p_memsz;
      if(entry>=vaddr &&entry<=(vaddr+memsz) ){
        // printf("p_off for pheader found: %d\n",temp_ph->p_offset);
        // printf("vadd:%d vad+memsz:%d\n",vaddr,vaddr+memsz);
        // printf("finally found the segment \n");
        void* virtual_mem=mmap(NULL,temp_ph->p_memsz, PROT_READ|PROT_WRITE|PROT_EXEC, MAP_ANONYMOUS|MAP_PRIVATE,fd,0);
        if (virtual_mem == MAP_FAILED) {
          perror("mmap failed");
          return;
        }

        struct node* newnode = (struct node*)malloc(sizeof(struct node));
        newnode->x = temp_ph->p_memsz;;
        newnode->y= virtual_mem;
        newnode->z=temp_ph;
        newnode->next = head; 
        head = newnode; 

        // read(fd, virtual_mem, temp_ph->p_filesz);

        // printf("loaded!!hurray\n");
        lseek(fd, temp_ph->p_offset, SEEK_SET);
        read(fd, virtual_mem, temp_ph->p_filesz);
        
        
        new_start=(startFunctionPtr)(entry-vaddr+virtual_mem);
        // new_start=(startFunctionPtr)(entry);

        break;
      }
    }
  
  }


  // char* file_ptr=(char*)malloc(file_size);
  // read(file_ptr,1,file_size,fd);
  // ehdr=file_ptr;
  // phdr=file_ptr+sizeof(ehdr)

  // again do error handling;
  
  // do erro handling here too;



  // need to perform error checking


  // 1. Load entire binary content into the memory from the ELF file.
  // 2. Iterate through the PHDR table and find the section of PT_LOAD 
  //    type that contains the address of the entrypoint method in fib.c
  // 3. Allocate memory of the size "p_memsz" using mmap function 
  //    and then copy the segment content
  // 4. Navigate to the entrypoint address into the segment loaded in the memory in above step
  // 5. Typecast the address to that of function pointer matching "_start" method in fib.c.
  // 6. Call the "_start" method and print the value returned from the "_start"
  
  int result=new_start();
  // int result = _start();
  printf("User _start return value = %d\n",result);
  // printf("yes printed...haha!\n");
  // free(ehdr);
  // free(phdr);
//   if (munmap(virtual_mem, temp_ph->p_memsz) == -1) {
//     perror("munmap failed");
    
// }
// free(temp_ph);
}

int main(int argc, char** argv) 
{
  if(argc != 2) {
    printf("Usage: %s <ELF Executable> \n",argv[0]);
    exit(1);
  }
  // 1. carry out necessary checks on the input ELF file
  // 2. passing it to the loader for carrying out the loading/execution
  load_and_run_elf(argv);
  // 3. invoke the cleanup routine inside the loader  
  loader_cleanup();
  return 0;
}
