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
#include <semaphore.h>

struct Process

{
    pid_t pid;
    char name[256];

    bool termination_status;
    volatile bool new_process;

    int execution_time;
    int waiting_time;

    int process_cycles;
    int start_counting_cycles;

    int priority;
};

int nprocs = 0;
int tslice = 0;
pid_t scheduler_pid;

struct SharedQueue
{

    struct Process arr[1000];
     // Store Process objects directly
    struct Process arr2[1000];
    int front2;
    int end2;

    struct Process arr3[1000];
    int front3,end3,front4,end4;


    struct Process arr4[1000];



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

struct SharedQueue *ready_queue;

void initialize_queue(struct SharedQueue *queue)
{
    queue->front = 0;
    queue->end = 0;
    queue->count = 0;
    queue->t = 0;
    queue->front2=0;
    queue->end2=0;
    queue->end3=0;
    queue->end4=0;
    queue->front3=0;
    queue->front4=0;
    
    sem_init(&queue->mutex, 1, 1);
    // initializing semaphore;
    queue->global_cycles = 0;
    queue->ptr = 0;
}

void enqueue(struct SharedQueue *ready_queue, struct Process *new)
{
    int  priority=new->priority;

    if(priority==2){
    memcpy(&(ready_queue->arr2[ready_queue->end2]), new, sizeof(struct Process));
    ready_queue->end2++;
    ready_queue->count++;
    printf("enqueud to arr2 with pid%d\n",ready_queue->arr2[0].pid);
    

    }
    else if(priority==3){
    memcpy(&(ready_queue->arr3[ready_queue->end3]), new, sizeof(struct Process));
    ready_queue->end3++;
    ready_queue->count++;
    }
    else if(priority==4){
        memcpy(&(ready_queue->arr4[ready_queue->end4]), new, sizeof(struct Process));
    ready_queue->end4++;
    ready_queue->count++;
    }
    // Copy the Process object to the shared memory queue
    else
    {memcpy(&(ready_queue->arr[ready_queue->end]), new, sizeof(struct Process));
    ready_queue->end++;
    ready_queue->count++;}
}

void register_terminated_pid(struct SharedQueue *ready_queue, pid_t pid)
{
    ready_queue->terminated_pids[ready_queue->t] = pid;
    ready_queue->t++;
}

struct Process *dequeue(struct SharedQueue *ready_queue,int  priority)
{
    // if (ready_queue->count > 0)
    printf("dequing called for %d-------------------\n",priority);
    
        if(priority==1){
        struct Process *dequeued_element = &(ready_queue->arr[ready_queue->front]);
        printf("dequing from 1___________\n");
        ready_queue->count--;
        ready_queue->front++;
        return dequeued_element;
        }
        else if(priority==2){
            struct Process *dequeued_element = &(ready_queue->arr[ready_queue->front2]);
            printf("dequing from 2-------------------\n");
            printf("inside dequeue pid:%d\n",dequeued_element->pid);
        ready_queue->count--;
        ready_queue->front2++;
        return dequeued_element;
        }
        else if(priority==3){
            struct Process *dequeued_element = &(ready_queue->arr[ready_queue->front3]);
        ready_queue->count--;
        ready_queue->front3++;
        return dequeued_element;
        }
        else if (priority==4){
            struct Process *dequeued_element = &(ready_queue->arr[ready_queue->front3]);
        ready_queue->count--;
        ready_queue->front4++;
        return dequeued_element;
        }
    
    return NULL;
}

int execute_command(char command[],int n);

struct Process *allocate_newprocess()
{
    struct Process *new = (struct Process *)malloc(sizeof(struct Process));
    if (new == NULL)
    {
        perror("allocation failed");
        exit(1);
    }
    new->process_cycles = 0;
    new->termination_status = false;

    return new;
}

void display()
{

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
            printf("priority %d\n",priority);

            execute_command(command2,priority);
        }
        else
        {
            execute_command(command,1);
        }


    } while (status);
}
int if_pid_present(struct SharedQueue *ready_queue, pid_t pid)
{
    for (int i = 0; i < ready_queue->t; i++)
    {
        if (pid == ready_queue->terminated_pids[i])
        {
            return 1;
        }
    }
    return 0;
}
struct Process *set_termination_status(struct SharedQueue *ready_queue, pid_t pid)
{

    for (int i = ready_queue->front; i <= ready_queue->end; i++)
    {
        if (ready_queue->arr[i].pid == pid)
        {
            ready_queue->arr[i].termination_status = true;
        }
    }
}
bool is_empty(struct SharedQueue* ready_queue,int priority){

    if(priority==1){
        if(ready_queue->front==ready_queue->end){
            return true;
        }
        else{
            return false;
        }
    }
    if(priority==2){
        if(ready_queue->front2==ready_queue->end2){
            return true;
        }
        else{
            return false;
        }
    }
    if(priority==3){
        if(ready_queue->front3==ready_queue->end3){
            return true;
        }
        else{
            return false;
        }
    }
    if(priority==4){
        if(ready_queue->front4==ready_queue->end4){
            return true;
        }
        else{
            return false;
        }
    }
    return false;
}

int execute_command(char command[],int n)
{
    int status = fork();
    if (status < 0)
    {
        printf("Something bad happened\n");
    }
    else if (status == 0)
    {
        struct sigaction default_sig;
        memset(&default_sig, 0, sizeof(default_sig));
        default_sig.sa_handler = SIG_DFL;
        sigaction(SIGINT, &default_sig, NULL);
        int grand_child = fork();
        if (grand_child == 0)
        {

            pid_t pid = getpid();
            struct Process *p = allocate_newprocess();
            strcpy(p->name,command);
            p->pid = pid;
            p->priority=n;
            p->new_process = false;
            p->start_counting_cycles = ready_queue->global_cycles;
            printf("process created with priority %d\n",n);

            strcpy(p->name, command);
            sem_wait(&ready_queue->mutex);
            // calling sem_wait
            enqueue(ready_queue, p);
            sem_post(&ready_queue->mutex);
            // repleninshing the value of the semaphore

            kill(pid, SIGSTOP);
            printf("it was stopped and  came back to continue\n");
            printf("came back to continue again\n");

            if (execl("/bin/sh", "sh", "-c", command, NULL) < 0)
            {
                perror("error occured;no instruction excuted!");
            }
        }
        else
        {
            int ret_status;
            waitpid(grand_child, &ret_status, 0);
            printf("process returned to its father\n");

            register_terminated_pid(ready_queue, grand_child);
        }
    }
    return status;
}

void scheduler_inside()
{

    int x = 1;
    struct Process *p;

    while (x == 1)
    {

        if (ready_queue->count == 0)
        {

            sleep(tslice);
            continue;
        }

        ready_queue->global_cycles++;
        // if scheduler arrived here it implies that atleast it ran for one tslice or cycle

        int temp = nprocs;

        if (ready_queue->count < nprocs)
        {
            temp = ready_queue->count;
        }
        struct Process arr[temp];

        int m = 0;
       
        for (int j = 0; j < temp; j++)
        {
            
            m++;
            sem_wait(&ready_queue->mutex);
            if(!is_empty(ready_queue,1)){
                arr[j] = *(dequeue(ready_queue,1));

            }
            else if(!is_empty(ready_queue,2)){
                printf("arr2 was not empty\n");
                arr[j] = *(dequeue(ready_queue,2));
                printf("pid for dequeud process is %d\n",arr[j].pid);

            }
            else if(!is_empty(ready_queue,3)){
                arr[j] = *(dequeue(ready_queue,3));

            }
            else if(!is_empty(ready_queue,4)){
                arr[j] = *(dequeue(ready_queue,4));

            }
            
            
            sem_post(&ready_queue->mutex);
            // arr[j].process_cycles++;
        }
        printf(" no of dequeued processes %d\n",m);
        // printf("\n------------------------\n");
        printf("\nable to dequeue into arrayy with pid:%d\n",arr[0].pid);
        for (int i = 0; i < temp; i++)
        {
            struct Process p2 = arr[i];
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
            kill(p2.pid, SIGCONT);
        }
        printf("putting scheduler to sleep\n");
        sleep(tslice);
        printf("scheduler has awakened\n");
        // process ran for tslice now
        // in below loop checking if they terminated
        // if thery terminated no need to enqueue again otherwise need to enqueue
        // after giving them signal to stop
        // printf("now moving on to passing stop signal to dequeued elements\n");

        for (int i = 0; i < temp; i++)
        {
            struct Process iter = arr[i];

            // it implies it has covered one cycle
            int child_status;
            // Check if the child process has terminated
            int result = waitpid(iter.pid, &child_status, WNOHANG);
            // printf("termination __status:%d\n",iter.termination_status);
            // if (result > 0)

            if (if_pid_present(ready_queue, iter.pid))
            {
                printf("debugged that process was terminated\n");
                iter.termination_status = true;
                // Child process has terminated
                iter.execution_time = arr[i].process_cycles * tslice;
                iter.waiting_time = (ready_queue->global_cycles - iter.start_counting_cycles - iter.process_cycles) * tslice;
                ready_queue->finalized_processes[ready_queue->ptr] = iter;
                ready_queue->ptr++;
                // iter.termination_status = true;
                // printf("detected that process got terminated\n");
                continue;
            }
            // may be this below blok is runnign for a proess which have not been forked yet

            if (iter.termination_status == false)
            {
                // printf("sending stop signal to process with pid:%d\n",iter.pid);
                kill(iter.pid, SIGSTOP);
                sem_wait(&ready_queue->mutex);
                enqueue(ready_queue, &iter);
                sem_post(&ready_queue->mutex);
                // added back to ready_queue;
            }
        }
    }
}

void print_finalized_processes(struct SharedQueue *queue)
{
    for (int i = 0; i < queue->ptr; i++)
    {
        struct Process p = queue->finalized_processes[i];
        printf("job:%s|pid:%d|execution time:%d|waiting time:%d\n",p.name, p.pid, p.execution_time, p.waiting_time);
    }
}

static void my_handler(int signum)
{
    if (signum == SIGINT)
    {
        printf("entered my_handler\n");
        print_finalized_processes(ready_queue);
        printf("reached the end\n");

        int result = kill(scheduler_pid, SIGTERM);

        if (result == 0)
        {
            printf("Signal sent to scheduler to terminate.\n");
            printf("scheduler process terminated successfully before shell\n");
        }
        else
        {
            perror("kill");
        }

        exit(0);
    }
}

const char *shm_name = "/my_shared_memory";
const int shm_size = sizeof(struct SharedQueue);

int main(int argc, char *argv[])
{

    // printf("main function pid:%d\n",getpid());
    if (argc != 3)
    {
        printf("Enter 2 arguments after command, the first argument is NCPUs and second argument is TSLICE in miliseconds");
        return 1;
    }

    nprocs = atoi(argv[1]); // atoi converts the NCPUs to int
    tslice = atoi(argv[2]); // atoi converts the Time slice to int

    if (nprocs <= 0 || tslice <= 0)
    {
        printf("Invalid NPROCS or TSLICE value\n");
        return 1;
    }

    printf("ncpu: %d tslice:%d\n", nprocs, tslice);

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1)
    {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shm_fd, shm_size) == -1)
    {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    ready_queue = (struct SharedQueue *)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (ready_queue == MAP_FAILED)
    {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    // Initialize the shared queue structure in parent
    initialize_queue(ready_queue);

    int pid = fork();
    scheduler_pid = pid;

    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {
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
    }
    else
    { // Parent process

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
