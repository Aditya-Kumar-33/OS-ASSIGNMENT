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
};
int nprocs=0;
int tslice=0;



struct SharedQueue {
    struct Process arr[1000];  // Store Process objects directly
    int front;
    int end;
    int count;
};

struct SharedQueue* ready_queue;

void initialize_queue(struct SharedQueue* queue) {
    queue->front = 0;
    queue->end = 0;
    queue->count = 0;
}

void enqueue(struct SharedQueue* ready_queue, struct Process* new) {
    // Copy the Process object to the shared memory queue
    memcpy(&(ready_queue->arr[ready_queue->end]), new, sizeof(struct Process));
    ready_queue->end++;
    ready_queue->count++;
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
    // printf("setting the status of this process to true--------------------------------signal\n");
    new->termination_status=false;

    return new;
}



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
 struct Process* find_process(struct SharedQueue* ready_queue,pid_t pid){
    
    for(int i=ready_queue->front;i<=ready_queue->end;i++){
        if(ready_queue->arr[i].pid==pid){
            ready_queue->arr[i].termination_status=true;
        }
        
    }
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
        p->new_process=false;

        // p->name=command;
        strcpy(p->name,command);
        enqueue(ready_queue,p); 
        printf("enqueued!\n");
        // printf("stop signal sent to process with pid:%d\n",getpid());
        // printf("name of the executable%s and pid:%d\n",ready_queue->arr[0].name,ready_queue->arr[0].pid);

        kill(pid, SIGSTOP);
        // printf("stop signal sent to process\n");
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
    int aise_hi;
    
    // waitpid(status,&aise_hi,0);
    printf("parent process for child for pid%d\n",status);
    // find_process(ready_queue,status);

    // if(hang not working)
    // use here retrieve process with parentss child pid and set terminatons status  to true;

    
    return status;
}


void scheduler_inside(){
    // printf("scheduler pid:%d\n",getpid());
    int x=1;
    struct Process* p;
    

    while(x==1){
        // printf("pid inside while loop(scheduler's pid):%d\n",getpid());
        if(ready_queue->count==0){
            
            // printf("ready_queue is empty\n");
            sleep(tslice);
            continue;
        }
        
       
        // printf("scheduler is implemented!\n");
        // printf("count:%d\n",ready_queue->count);
        int temp=nprocs;
        
        if(ready_queue->count<nprocs){
            temp=ready_queue->count;
        }
        struct Process arr[temp];
        // printf("executed till creation of arr\n");
        // printf("------------------------\n");
        int m=0;;
        for(int j=0;j<temp;j++){
            m++;
            arr[j]=*(dequeue(ready_queue));
            // printf("name:%s",arr[j].name);
            // printf("dequed processs's pid:%d ",arr[j].pid);
            // kill(p->pid, SIG_CONT);
            

        }
        printf(" no of dequeued processes %d\n",m);
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
            printf("sending signal to contiue to process with pid %d\n",arr[i].pid);
            kill(p2.pid,SIGCONT);

        }
        printf("putting scheduler to sleep\n");
        sleep(tslice);
        printf("scheduler has awakened\n");
        // process ran for tslice now
        // in below loop checking if they terminated
        // if thery terminated no need to enqueue again otherwise need to enqueue 
        // after giving them signal to stop
        // printf("now moving on to passing stop signal to dequeued elements\n");



        for(int i=0;i<temp;i++){
            struct Process iter=arr[i];
            int child_status;
            // Check if the child process has terminated
            int result = waitpid(iter.pid, &child_status, WNOHANG);
            printf("result of waitpid:%d\n",result);
            // if (result > 0)
            if (WIFEXITED(child_status)) {
        // Process exited normally
        int exit_status = WEXITSTATUS(child_status);
        printf("Child process %d exited with status %d\n", result, exit_status);
    }
            if (result>0){
                iter.termination_status=true;
                // Child process has terminated
                // iter.termination_status = true;
                printf("detected that process got terminated\n");
                continue;
            }
            // may be this below blok is runnign for a proess which have not been forked yet
        
            
            if(iter.termination_status==false){
                // printf("sending stop signal to process with pid:%d\n",iter.pid);
                kill(iter.pid,SIGSTOP);
                enqueue(ready_queue,&iter);
                // added back to ready_queue;
            }
            

        }
        

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

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
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

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
         // Child process

         sleep(8);
         printf("calling scheduler_inside function\n");
        scheduler_inside();






        // // Access and modify the shared structure (child)
        // printf("Child Process:\n");
        // struct Process new_process;
        // new_process.execution_time = 45;
        // enqueue(ready_queue, &new_process);
    } else { // Parent process

        display();
        wait(NULL); // Wait for the child to finish

        // Access and print the shared structure (parent)
        // printf("Parent Process:\n");
        // struct Process* dequeued_process = dequeue(ready_queue);
        // if (dequeued_process != NULL) {
        //     printf("Dequeued Process Execution Time: %d\n", dequeued_process->execution_time);
        // }

        // Unmap and close shared memory
        munmap(ready_queue, shm_size);
        close(shm_fd);

        // Remove shared memory object
        shm_unlink(shm_name);
    }

    return 0;
}
