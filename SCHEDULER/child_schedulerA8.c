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


// changes 
// memory cleanup called in my handler 
// included the commands for nonexecutable coommands



// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/mman.h>
// #include <sys/stat.h>
// #include <fcntl.h>
// #include <stdbool.h>  // Include the bool type




struct Process {
    pid_t pid;
    struct Process* next;
    char name[256];
    bool termination_status;
    int execution_time;
    volatile bool new_process;
    int waiting_time;
    int process_cycles;
    int start_counting_cycles;
    int priority;

};
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
    if(new->pid==0){
        printf("did not enqued process with pid =0\n");
        return;
    }
    int priority=new->priority;
    
    int n = ready_queue->count; // Current number of elements in the array

    int indexToInsert = n; // Start by inserting at the end

    while (indexToInsert > 0 && ready_queue->arr[indexToInsert - 1].priority < priority) {
        // Move elements to the right to make space for the new process
        memcpy(&(ready_queue->arr[indexToInsert]), &(ready_queue->arr[indexToInsert - 1]), sizeof(struct Process));
        indexToInsert--;
    }

    // Copy the new Process object to the shared memory queue
    memcpy(&(ready_queue->arr[indexToInsert]), new, sizeof(struct Process));

    ready_queue->end++;
    ready_queue->count++;
}
void register_terminated_pid(struct SharedQueue* ready_queue,pid_t pid){
    ready_queue->terminated_pids[ready_queue->t]=pid;
    ready_queue->t++;

}


int execute_command_simply(char command[]);

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
        }


    } while (status);
}

int execute_command_simply(char command[]){
    int status=fork();
    if(status < 0) {
    printf("Something bad happened\n");
    } 
    else if(status == 0) {
        if(execl("/bin/sh", "sh", "-c",command,NULL)<0){
            perror("error occured;no instruction excuted!");

            
        }
        
        // this is a child process
        // hence we can write implementation for this
    //  printf(“I am the child process\n”);
    } 
    
    
    return status;
}



int if_pid_present(struct SharedQueue* ready_queue,pid_t pid){
    for(int i=0;i<ready_queue->t;i++){
        if(pid==ready_queue->terminated_pids[i]){
            return 1;
        }
    }
    return 0;
}
 struct Process* set_termination_status(struct SharedQueue* ready_queue,pid_t pid){
    
    for(int i=ready_queue->front;i<=ready_queue->end;i++){
        if(ready_queue->arr[i].pid==pid){
            ready_queue->arr[i].termination_status=true;
        }
        
    }
 }
int execute_command(char command[],int priority){
    int status=fork();
    if(status < 0) {
    printf("Something bad happened\n");
    } 
    else if(status == 0) {
        struct sigaction default_sig;
        memset(&default_sig, 0, sizeof(default_sig));
        default_sig.sa_handler = SIG_DFL;
        sigaction(SIGINT, &default_sig, NULL);
        int grand_child=fork();
        if(grand_child==0){
            
            pid_t pid=getpid();
            struct Process* p=allocate_newprocess();
            p->pid=pid;
            p->priority=priority;
            p->new_process=false;
            p->start_counting_cycles=ready_queue->global_cycles;

            
            strcpy(p->name,command);
            sem_wait(&ready_queue->mutex);
            // calling sem_wait
            enqueue(ready_queue,p); 
            sem_post(&ready_queue->mutex);
            printf("process with pid %d enqueued\n",getpid());
            // repleninshing the value of the semaphore

            
            
            printf("sending signal to %d\n",pid);

            kill(pid, SIGSTOP);
           
            if(execl("/bin/sh", "sh", "-c",command,NULL)<0){
                perror("error occured;no instruction excuted!");

                
            }

        }
        else{
            
            int ret_status;
            waitpid(grand_child,&ret_status,0);
            
            register_terminated_pid(ready_queue,grand_child);
            
        }
        
        
        
    } 
    
    

    
    
    
    return status;
}


void scheduler_inside(){
    
    int x=1;
    struct Process* p;
    

    while(x==1){
        
        if(ready_queue->count==0){
            
    
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
        
        
        int m=0;;
        for(int j=0;j<temp;j++){
            m++;
            sem_wait(&ready_queue->mutex);
            arr[j]=*(dequeue(ready_queue));

            sem_post(&ready_queue->mutex);
            arr[j].process_cycles++;
            
            

        }
        // printf(" no of dequeued processes %d\n",m);
        // printf("\n------------------------\n");
        // printf("\nable to dequeue into arrayy\n");
        for(int i=0;i<temp;i++){
            struct Process p2=arr[i];
            // printf("process  pid  is %d\n",p2.pid);
            // if(p2->new_process==true){
            //     p2->new_process=false;
            //     printf("passing to execute prcoesss function\n");
            //     execute_process(p2);
            // }
            // else{
            //     printf("sending signal to contiue\n");
            //     kill(arr[i]->pid,SIGCONT);
            // }
            // printf("sending signal to contiue to process with pid %d\n",arr[i].pid);
            printf("sending cotinue signal to %d\n",p2.pid);
            if(p2.pid==0){
                printf("--------------\n");
                continue;
            }
            kill(p2.pid,SIGCONT);

        }
        // printf("putting scheduler to sleep\n");
        sleep(tslice);
        // printf("scheduler has awakened\n");
        // process ran for tslice now
        // in below loop checking if they terminated
        // if thery terminated no need to enqueue again otherwise need to enqueue 
        // after giving them signal to stop
        // printf("now moving on to passing stop signal to dequeued elements\n");



        for(int i=0;i<temp;i++){
            struct Process iter=arr[i];

            
            // it implies it has covered one cycle
            int child_status;
            // Check if the child process has terminated
            int result = waitpid(iter.pid, &child_status, WNOHANG);
            // printf("termination __status:%d\n",iter.termination_status);
            // if (result > 0)
           
            if (if_pid_present(ready_queue,iter.pid)){
                iter.termination_status=true;
                // Child process has terminated
                iter.execution_time=arr[i].process_cycles*tslice;
                iter.waiting_time=(ready_queue->global_cycles-iter.start_counting_cycles-iter.process_cycles)*tslice;
                ready_queue->finalized_processes[ready_queue->ptr]=iter;
                ready_queue->ptr++;
                // iter.termination_status = true;
                // printf("detected that process got terminated\n");
                continue;
            }
            // may be this below blok is runnign for a proess which have not been forked yet
        
            
            if(iter.termination_status==false){
                printf("sending stop signal to process with pid:%d\n",iter.pid);
                if(iter.pid==0){
                    continue;
                }
                kill(iter.pid,SIGSTOP);
                sem_wait(&ready_queue->mutex);
                if(iter.pid!=0){
                    enqueue(ready_queue,&iter);
                    
                }
                
                sem_post(&ready_queue->mutex);
                // added back to ready_queue;
            }
            

        }
        

    }
}


void print_finalized_processes(struct SharedQueue* queue){
    for(int i=0;i<queue->ptr;i++){
        struct Process p=queue->finalized_processes[i];
        printf("pid:%d|execution time:%d|waiting time:%d\n",p.pid,p.execution_time,p.waiting_time);
    }

}
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







const char* shm_name = "/my_shared_memory";
const int shm_size = sizeof(struct SharedQueue);


int main(int argc, char *argv[]) {


    // printf("main function pid:%d\n",getpid());
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


    

    shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    ready_queue = (struct SharedQueue*)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ready_queue == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize the shared queue structure in parent
    initialize_queue(ready_queue);

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






        // // Access and modify the shared structure (child)
        // printf("Child Process:\n");
        // struct Process new_process;
        // new_process.execution_time = 45;
        // enqueue(ready_queue, &new_process);
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

        // Access and print the shared structure (parent)
        // printf("Parent Process:\n");
        // struct Process* dequeued_process = dequeue(ready_queue);
        // if (dequeued_process != NULL) {
        //     printf("Dequeued Process Execution Time: %d\n", dequeued_process->execution_time);
        // }

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
