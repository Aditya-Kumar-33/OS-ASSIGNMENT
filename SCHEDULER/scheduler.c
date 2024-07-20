#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>













int nprocs=0;
int tslice=0;
// this is for hard coding need to be removed later


struct Process
{
    pid_t pid;
    struct Process *next;
    char name[256];
    int execution_time;
    int waiting_time;
};


// creating shared segment 
struct SharedQueue {
    struct Process* head;
    struct Process* tail;
    int count;
};







struct Process* head = NULL; // Initialize head and tail to NULL
struct Process* tail = NULL;
int count = 0;


struct Process* allocate_newprocess()
{
    struct Process *new = (struct Process *)malloc(sizeof(struct Process));
    if (new == NULL)
    {
        perror("allocation failed");
        exit(1);
    }

    return new;
}

void enqueue(struct Process* new)
{
    if (head == NULL)
    {
        head = new;
        tail = new;
        new->next = NULL;
    }
    else
    {
        tail->next = new;
        new->next = NULL;
        tail = new;
    }
    count++;
}


struct Process* dequeue()
{
    if (count > 0)
    {
        struct Process *dequeued_element = head; // Dequeue the head element
        count--;
        head = head->next;
        return dequeued_element; // Return the dequeued process
    }

    // If the ready queue is empty
    return NULL;
}


void schedule(){

}



// shell 
// shell
void display(){

    int status;
    
    do{
        printf("\nSimpleShelll_115:~$ ");
        char command[1000];
        fgets(command,1000,stdin);
        status=execute_command(command);
        // printf("\n");
    
    }while(status);
}


int execute_command(char command[]){
    int status=fork();
    if(status < 0) {
    printf("Something bad happened\n");
    } 
    else if(status == 0) {
        pid_t pid=getpid();
        struct Process* p=allocate_newprocess();
        p->pid=pid;
        // p->name=command;
        strcpy(p->name,command);
        enqueue(p); 
        kill(pid, SIGSTOP);
        if(execl("/bin/sh", "sh", "-c",command,NULL)<0){
            perror("error occured;no instruction excuted!");

            
        }
        
        // this is a child process
        // hence we can write implementation for this
    //  printf(“I am the child process\n”);
    } 
    else{
        wait(NULL);
        // waiting for child to finish executing
        // prevents it from becoming a zombie
        printf("i am a parent;i have waited for child to finished executing");
    }
    // else {
    //     printf("“I am the parent Shell\n”");
    // }
    // if we dont want to mention whther a paret process was executed;
    return status;
}







int main()

{
    // printf("input:");
    // scanf("%d",&nprocs);
    // scanf("%*c");
    // scanf("%d",&tslice);
    // scanf("%*c");
     // Define a shared memory object name
    const char* shm_name = "/my_shared_memory";

    // Create a shared memory object
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    // Set the size of the shared memory region for the shared queue
    size_t shm_size = sizeof(struct SharedQueue);
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    // Map the shared memory into the parent process's address space
    struct SharedQueue* shared_queue = (struct SharedQueue*)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shared_queue == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize the shared queue
    shared_queue->head = NULL;
    shared_queue->tail = NULL;
    shared_queue->count = 0;








    pid_t child_pid=fork();
    if(child_pid<0){
        perror("error in creating child");
        exit(EXIT_FAILURE);

    }
    if(child_pid==0){
        char command[]="./child_scheduler";
        printf("child scheduler invoked");
        if(execl("/bin/sh", "sh", "-c",command,NULL)<0){
            perror("error occured;no instruction excuted!");

            
        }

    }
    munmap(shared_queue, shm_size);
    close(shm_fd);

    // Unlink (delete) the shared memory object
    shm_unlink(shm_name);

    exit(EXIT_SUCCESS);
    display();
    // NCPU = numCPUs();
    
    struct Process* p=allocate_newprocess();
    strcpy(p->name,"exec");

    enqueue(p);
    struct Process* p2=allocate_newprocess();
    strcpy(p2->name,"ls");
    enqueue(p2);
    p=dequeue();
    printf("%s\n",p->name);

    printf("process added successfully\n");

   


    // for (int i = 0; i < NCPU; i++) 
    // //NCPUs number of processes will dequeue from the front, gets executed for the quantumn and enqueued back in linked list
    // {
    //     struct sigaction sig;
    //     memset(&sig, 0, sizeof(sig));
    //     sig.sa_handler = Time_over;
    //     if (sigaction(SIGSTOP, &sig, NULL) == -1) // Check if sigaction returns -1, handle any errors in setting the signal handler
    //     {
    //         perror("sigaction error");
    //         return 1;
    //     }
    // }

    return 0;
}