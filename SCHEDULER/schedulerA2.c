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
// this code uses queue using linked list




int nprocs=0;
int tslice=0;
// this is for hard coding need to be removed later


struct Process
{
    pid_t pid;
    struct Process *next;
    char name[256];
    bool termination_status;
    int execution_time;
    volatile bool new_process;
    int waiting_time;
};


// creating shared segment 
struct SharedQueue {
    struct Process* head;
    struct Process* tail;
    int count;
};
// struct SharedQueue2 {
//     struct Process* arr[1000];
//     int front;
//     int end;

//     int count;
// };

struct SharedQueue* ready_queue;

struct Process* head = NULL; // Initialize head and tail to NULL
struct Process* tail = NULL;
int count = 0;

int execute_command(char command[]);

struct Process* allocate_newprocess()
{
    struct Process *new = (struct Process *)malloc(sizeof(struct Process));
    if (new == NULL)
    {
        perror("allocation failed");
        exit(1);
    }
    // new->new_process=true;
    printf("setting the status of this process to true--------------------------------signal\n");
    new->termination_status=false;

    return new;
}

void enqueue(struct Process* new)
{
    if (ready_queue->head == NULL)
    {
        ready_queue->head = new;
        ready_queue->tail = new;
        new->next = NULL;
    }
    else
    {
        ready_queue->tail->next = new;
        new->next = NULL;
        ready_queue->tail = new;
    }
    ready_queue->count++;
}


struct Process* dequeue()
{
    if (ready_queue->count > 0)
    {
        struct Process *dequeued_element = ready_queue->head; // Dequeue the head element
        ready_queue->count--;
        ready_queue->head = dequeued_element->next;
        return dequeued_element; // Return the dequeued process
    }

    // If the ready queue is empty
    return NULL;
}

// shell 
// shell
int add_to_ready_queue(char command[]);
void display(){

    int status=1;
    
    do{
        printf("\nSimpleShelll_115:~$ ");
        char command[1000];
        fgets(command,1000,stdin);
        // add_to_ready_queue(command);
        execute_command(command);
        
        // printf("\n");
    
    }while(status);
}


int add_to_ready_queue(char command[]){
    struct Process* p=allocate_newprocess();
    // p->pid=pid;
    // p->name=command;
    strcpy(p->name,command);
    // p->new_process=true;
    enqueue(p); 
    printf("added to ready_queue\n");
    

}


int execute_process(struct Process* p){
    printf("we are here at execute_process\n");
    p->new_process=false;
    int status=fork();
    if(status < 0) {
    printf("Something bad happened\n");
    } 
    else if(status == 0) {
        pid_t pid=getpid();
        p->pid=pid;
        printf("goingg to execute execl\n");
        
       
        if(execl("/bin/sh", "sh", "-c",p->name,NULL)<0){
            perror("error occured;no instruction excuted!");

            
        }
        exit(0);
        
        // this is a child process
        // hence we can write implementation for this
    //  printf(“I am the child process\n”);
    } 
    // this block will be executed  in parent process;
  
    // wait(NULL);
    // we cant call wait otherwise scheduler will keep waiting for the process to finish executing
    // p->termination_status=true;
    return status;
}


// struct Process* ret_process_from_queue(pid_t pid){
//     struct Process* temp=ready_queue->head;
//     while(head)
// }


int execute_command(char command[]){
    int status=fork();
    if(status < 0) {
    printf("Something bad happened\n");
    } 
    else if(status == 0) {
        pid_t pid=getpid();
        struct Process* p=allocate_newprocess();
        p->pid=pid;
        p->new_process=false;

        // p->name=command;
        strcpy(p->name,command);
        enqueue(p); 
        printf("enqueued!\n");
        printf("stop signal sent to process with pid:%d\n",getpid());

        kill(pid, SIGSTOP);
        printf("stop signal sent to process\n");
        if(execl("/bin/sh", "sh", "-c",command,NULL)<0){
            perror("error occured;no instruction excuted!");

            
        }
        
        // this is a child process
        // hence we can write implementation for this
    //  printf(“I am the child process\n”);
    } 
    
    // else {
    //     printf("“I am the parent Shell\n”");
    // }
    // if we dont want to mention whther a paret process was executed;
    // sleep(1);
    wait(NULL);
    // if(hang not working)
    // use here retrieve process with parentss child pid and set terminatons status  to true;

    
    return status;
}

void scheduler_inside(){
    printf("scheduler pid:%d\n",getpid());
    int x=1;
    struct Process* p;
    

    while(x==1){
        printf("pid inside while loop(scheduler's pid):%d\n",getpid());
        if(ready_queue->count==0){
            
            printf("ready_queue is empty\n");
            sleep(tslice);
            continue;
        }
        
       
        printf("scheduler is implemented!\n");
        printf("count:%d\n",ready_queue->count);
        int temp=nprocs;
        
        if(ready_queue->count<nprocs){
            temp=ready_queue->count;
        }
        struct Process* arr[temp];
        printf("executed till creation of arr\n");
        printf("------------------------\n");
        for(int j=0;j<temp;j++){
            arr[j]=dequeue();
            printf("dequed processs's pid:%d ",arr[j]->pid);
            // kill(p->pid, SIG_CONT);

        }
        printf("\n------------------------\n");
        printf("\nable to dequeue into arrayy\n");
        for(int i=0;i<temp;i++){
            struct Process* p2=arr[i];
            printf("process  pid  is %d\n",p2->pid);
            // if(p2->new_process==true){
            //     p2->new_process=false;
            //     printf("passing to execute prcoesss function\n");
            //     execute_process(p2);
            // }
            // else{
            //     printf("sending signal to contiue\n");
            //     kill(arr[i]->pid,SIGCONT);
            // }
            printf("sending signal to contiue to process with pid %d\n",arr[i]->pid);
            kill(p2->pid,SIGCONT);

        }
        printf("putting scheduler to sleep\n");
        sleep(tslice);
        printf("scheduler has awakened\n");
        // process ran for tslice now
        // in below loop checking if they terminated
        // if thery terminated no need to enqueue again otherwise need to enqueue 
        // after giving them signal to stop
        printf("now moving on to passing stop signal to dequeued elements\n");



        for(int i=0;i<temp;i++){
            struct Process* iter=arr[i];
            int child_status;
            // Check if the child process has terminated
            int result = waitpid(iter->pid, &child_status, WNOHANG);
            if (result > 0) {
                // Child process has terminated
                iter->termination_status = true;
                printf("detected that process got terminated\n");
                continue;
            }
            // may be this below blok is runnign for a proess which have not been forked yet
        
            
            if(iter->termination_status==false){
                printf("sending stop signal to process with pid:%d\n",iter->pid);
                kill(iter->pid,SIGSTOP);
                enqueue(iter);
                // added back to ready_queue;
            }
            

        }
        

    }
}


int main(int argc, char *argv[])

{
    printf("main function pid:%d\n",getpid());
    if (argc != 3)
    {
        printf("Enter 2 arguments after command, the first argument is NCPUs and second argument is TSLICE in miliseconds");
        return 1;
    }

    nprocs = atoi(argv[1]);   // atoi converts the NCPUs to int
    tslice = atoi(argv[2]); // atoi converts the Time slice to int

    if (nprocs <= 0 || tslice <= 0)
    {
        printf("Invalid NPROCS or TSLICE value\n");
        return 1;
    }

    printf("ncpu: %d tslice:%d\n",nprocs,tslice);
 
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
    ready_queue = (struct SharedQueue*)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ready_queue == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize the shared queue
    ready_queue->head = NULL;
    ready_queue->tail = NULL;
    ready_queue->count = 0;
  
    pid_t child_pid=fork();
    // creating the process for scheduler
    // it runs as a daemon process
    if(child_pid<0){
        perror("error in creating child");
        exit(EXIT_FAILURE);

    }
    if(child_pid==0){
        printf("initializing sheduler as a process\n");
        
       
        char shm_fd_str[10];
        // using it to pass the file descriptor for the shared memory as a command line arguement
        
        // snprintf(shm_fd_str, sizeof(shm_fd_str), "%d", shm_fd);
        sprintf(shm_fd_str,"%d",shm_fd);
        // converts the shm_fd which is an int to string to pass  as the arguement
        


        ready_queue = (struct SharedQueue*)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        // mapping the shared memory into the address space of the child i.e scheduler
        if (ready_queue == MAP_FAILED) {
            perror("mmap in child_scheduler");
            exit(EXIT_FAILURE);
        }

        // instead of calling execv we can use function for scheduler which implements the schduler 
        // since it is an altogether different process created by fork 
        // hence the purpose of using daemon process is served


        scheduler_inside();
        // it is function with an infinite loop which only returns when the whole program terminates
        // thus serving the purpose of daemon process





        // execl("./child_scheduler", "./child_scheduler", shm_fd_str, NULL);

        // execl will only return if an error occurs
        // perror("execl");
        // exit(EXIT_FAILURE);

    }
    
    display();


    // Wait for the child to complete
    // To perform
    // needs to send a signal for scheduler to terminate so that it can return to wait and shell can terminate
    wait(NULL);
    // sleep(5);
    munmap(ready_queue, shm_size);
    close(shm_fd);

    // Unlink (delete) the shared memory object
    shm_unlink(shm_name);

    exit(EXIT_SUCCESS);
    // 
    // // NCPU = numCPUs();

    // printf("process added successfully\n");

    return 0;
}