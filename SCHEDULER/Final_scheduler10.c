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
#include<semaphore.h>


struct Process {
    pid_t pid;
    struct Process* next;
    char name[256];
    // stores name
    bool termination_status;
    // to know if process terminated
    int execution_time;

    
    int waiting_time;
    int process_cycles;
    //rounds taken by process to execute
    int start_counting_cycles;
    // cycles of scheduler when process entered

};

// global variables
int nprocs=0;
int tslice=0;
pid_t scheduler_pid;


struct SharedQueue {

    struct Process arr[1000];  // Store Process objects directly
    int front;
    int end;
    int count;
    sem_t mutex;
    // introducing semaphore to enable synchronization
    pid_t terminated_pids[1000];
    int t;
    int global_cycles;
    struct Process finalized_processes[1000];
    int ptr;
};

struct SharedQueue* ready_queue;

// initialzing queue including front pointer and end pointers
void initialize_queue(struct SharedQueue* queue) {
    queue->front = 0;
    queue->end = 0;
    queue->count = 0;
    queue->t=0;
    sem_init(&queue->mutex,1,1);
    // initializing semaphore;
    queue->global_cycles=0;
    queue->ptr=0;
    
    
}

void enqueue(struct SharedQueue* ready_queue, struct Process* new) {
    // Copy the Process object to the shared memory queue
    memcpy(&(ready_queue->arr[ready_queue->end]), new, sizeof(struct Process));
    ready_queue->end++;
    ready_queue->count++;
}

// fn to store the pid of terminated processes in an array
void register_terminated_pid(struct SharedQueue* ready_queue,pid_t pid){
    ready_queue->terminated_pids[ready_queue->t]=pid;
    ready_queue->t++;

}

struct Process* dequeue(struct SharedQueue* ready_queue) {
    if (ready_queue->count > 0) {
        struct Process* dequeued_element = &(ready_queue->arr[ready_queue->front]);
        ready_queue->count--;
        ready_queue->front++;
        return dequeued_element;
    }
    return NULL;
}


int execute_command(char command[],int priority);
int execute_command_simply(char command[]);


// initializing new process
struct Process* allocate_newprocess()
{
    struct Process *new = (struct Process *)malloc(sizeof(struct Process));
    if (new == NULL)
    {
        perror("allocation failed");
        exit(1);
    }
    new->process_cycles=0;
    new->termination_status=false;

    return new;
}


// simple shell welcome prompt
void display(){

    int status = 1;

    do
    {
        printf("\nSimpleShelll_115:~$ ");
        char command[1000];
        fgets(command, 1000, stdin);

        if (strncmp(command, "submit", 6) == 0) // cmp first 6 char to know if it's submit ./a.out
        {
            // handle the submit command

            char *token;
            char *command2;
            int priority = 1;
            int count = 0;

            token = strtok(command, " ");
            while (token != NULL)
            {
                count++;
                if(count==2){
                    command2 = token;
                }
                if (count == 3)
                {
                    priority = atoi(token);
                }
                token = strtok(NULL, " ");
            }

            execute_command(command2,priority);
        }
        else
        {
            execute_command_simply(command);
            // runs when there is no submit command
        }


    } while (status);
}

// fn for non executable commands
int execute_command_simply(char command[]){
    int status=fork();
    if(status < 0) {
    printf("Something bad happened\n");
    } 
    else if(status == 0) {
        if(execl("/bin/sh", "sh", "-c",command,NULL)<0){
            perror("error occured;no instruction excuted!");

            
        }
        
        
    } 
    
    
    return status;
}


// fn to check if pid is terminated or not
int if_pid_present(struct SharedQueue* ready_queue,pid_t pid){
    for(int i=0;i<ready_queue->t;i++){
        if(pid==ready_queue->terminated_pids[i]){
            return 1;
        }
    }
    return 0;
}

// to update the status of process to terminated
 struct Process* set_termination_status(struct SharedQueue* ready_queue,pid_t pid){
    
    for(int i=ready_queue->front;i<=ready_queue->end;i++){
        if(ready_queue->arr[i].pid==pid){
            ready_queue->arr[i].termination_status=true;
        }
        
    }
 }


//  main fn to execute the executable
int execute_command(char command[],int n){
    int status=fork();
    // creating a child
    if(status < 0) {
    printf("Something bad happened\n");
    } 
    else if(status == 0) {
        // setting default sinal 
        // because changed in the main fn
        struct sigaction default_sig;
        memset(&default_sig, 0, sizeof(default_sig));
        default_sig.sa_handler = SIG_DFL;
        sigaction(SIGINT, &default_sig, NULL);
        
        // creating a grandchild
        int grand_child=fork();
        if(grand_child==0){
            
            pid_t pid=getpid();
            struct Process* p=allocate_newprocess();
            p->pid=pid;
            // p->new_process=false;
            
            p->start_counting_cycles=ready_queue->global_cycles;

            
            strcpy(p->name,command);


            // using semaphore 
            sem_wait(&ready_queue->mutex);
            // calling sem_wait
            enqueue(ready_queue,p); 
            sem_post(&ready_queue->mutex);
            // repleninshing the value of the semaphore

            
            
            

            kill(pid, SIGSTOP);
            // sending signal to stop
           
            if(execl("/bin/sh", "sh", "-c",command,NULL)<0){
                perror("error occured;no instruction excuted!");

                
            }

        }
        else{
            // father waiting to update child status 
            
            int ret_status;
            waitpid(grand_child,&ret_status,0);
            
            register_terminated_pid(ready_queue,grand_child);
            
        }
        
        
        
    } 
    
    

    
    
    
    return status;
}

// main scheduler handler
void scheduler_inside(){
    
    int x=1;
    struct Process* p;
    

    while(x==1){
        
        if(ready_queue->count==0){
            
    // to give time to userr to to give inputs
            sleep(tslice);
            continue;
        }

        ready_queue->global_cycles++;
        // if scheduler arrived here it implies that atleast it ran for one tslice or cycle
        
       
    
    
        int temp=nprocs;
        
        if(ready_queue->count<nprocs){
            temp=ready_queue->count;
        }
        struct Process arr[temp];
        // creating an arrray of processes to schedule
        
        
        int m=0;;
        for(int j=0;j<temp;j++){
            m++;
            // callin sempahore
            sem_wait(&ready_queue->mutex);
            arr[j]=*(dequeue(ready_queue));
            sem_post(&ready_queue->mutex);
            // sem post to gie access to other processes
            arr[j].process_cycles++;
            
            

        }
       
        for(int i=0;i<temp;i++){
            struct Process p2=arr[i];
            // sending signal to continue 
            
            kill(p2.pid,SIGCONT);

        }
        // printf("putting scheduler to sleep\n");
        
        // scheduler sleeps for tslice
        // this gives time for processes to run for tslice
        // as soon as scheduler wakes,it gives signal  to  stop again
        // thus acting like a round robin
        sleep(tslice);
        



        for(int i=0;i<temp;i++){
            struct Process iter=arr[i];

            
            // it implies it has covered one cycle
            int child_status;
            // Check if the child process has terminated
            int result = waitpid(iter.pid, &child_status, WNOHANG);
            
        //    this is to ensure not to enqueue a terminated process
            if (if_pid_present(ready_queue,iter.pid)){
                iter.termination_status=true;
                // Child process has terminated
                iter.execution_time=arr[i].process_cycles*tslice;
                iter.waiting_time=(ready_queue->global_cycles-iter.start_counting_cycles-iter.process_cycles)*tslice;
                ready_queue->finalized_processes[ready_queue->ptr]=iter;
                ready_queue->ptr++;
               
                continue;
            }
            
        
            
            if(iter.termination_status==false){
                // sending signal to stop before enquing
                kill(iter.pid,SIGSTOP);
                sem_wait(&ready_queue->mutex);
                enqueue(ready_queue,&iter);
                sem_post(&ready_queue->mutex);
                // added back to ready_queue;
            }
            

        }
        

    }
}


void print_finalized_processes(struct SharedQueue* queue){
    for(int i=0;i<queue->ptr;i++){
        struct Process p=queue->finalized_processes[i];
        printf("command:%s|pid:%d|execution time:%d|waiting time:%d\n",p.name,p.pid,p.execution_time,p.waiting_time);
    }

}

const char* shm_name = "/my_shared_memory";
const int shm_size = sizeof(struct SharedQueue);
int shm_fd;
const char* shm_name;
const int shm_size; 

static void my_handler(int signum)
{
    if (signum == SIGINT)
    {
        printf("entered my_handler\n");
        print_finalized_processes(ready_queue);
        printf("reached the end\n");
        while(ready_queue->count!=0){
            // printf("running\n");
            int q=1;
            // waiting for scheduler to finish first
        }

        int result = kill(scheduler_pid, SIGTERM);

        if (result == 0) {
            printf("Signal sent to scheduler to terminate successfully.\n");
            printf("scheduler process terminated successfully before shell\n");
        } else {
            perror("kill");
        }
        

        sem_destroy(&ready_queue->mutex);
        // destroying the semaphore when there is no use of it
        munmap(ready_queue, shm_size);
        close(shm_fd);

        // Remove shared memory object
        shm_unlink(shm_name);
        printf("memory cleaned\n");
        

        exit(0);
    }
}










int main(int argc, char *argv[]) {


    // error handling
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


    
// crreating a shared memory file descriptor
    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
// allocating the size
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }
// assigning a pointer to shared memory after mappin it into physical adrress space of the process
    ready_queue = (struct SharedQueue*)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ready_queue == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize the shared queue structure in parent
    initialize_queue(ready_queue);

    // this creates a sseparate scheduler process which keeps running in the background
    int pid = fork();

    scheduler_pid=pid;

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
         // Child process

        // resetting default signal so that doesnt call my handler
         struct sigaction default_sig;
        memset(&default_sig, 0, sizeof(default_sig));
        default_sig.sa_handler = SIG_DFL;
        sigaction(SIGINT, &default_sig, NULL);

         sleep(8);
        //  printf("calling scheduler_inside function\n");
        scheduler_inside();






        
    } else { // Parent process

        struct sigaction sig;
        memset(&sig, 0, sizeof(sig));
        sig.sa_handler = my_handler;

        if (sigaction(SIGINT, &sig, NULL) == -1) // checking if sigaction returns -1, handle any errors in setting the signal handler
        {
            perror("sigaction error");
            return 1;
        }

        display();
        wait(NULL); // Wait for the child to finish

        

        // Unmap and close shared memory
        sem_destroy(&ready_queue->mutex);
        // destroying the semaphore when there is no use of it
        munmap(ready_queue, shm_size);
        close(shm_fd);

        // Remove shared memory object
        shm_unlink(shm_name);
    }

    return 0;
}
