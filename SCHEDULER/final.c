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
#include <time.h>

struct Process
{
    pid_t pid;      // stores the pid of the process
    char name[256]; // command name

    bool termination_status;   // if the process has terminated, True
    volatile bool new_process; // if the process has not yet started execution, True

    double execution_time; // stores execution time
    double waiting_time;   // stores waiting time

    time_t start_time; // stores current time during allocation
    time_t end_time;   // stores time during termination
    int priority;      // priority order
};

int nprocs = 0;                    // stores NCPUs
double tslice = 0;                 // stores time Quantum
pid_t scheduler_pid;               // stores pid of Scheduler
time_t temp;                       // temp variable to store time
#define HISTORY_FILE "history.txt" // txt file where history will be stored

struct SharedQueue
{
    struct Process arr[1000]; // Store Process objects directly
    int front;                // head of ready queue
    int end;                  // last process in ready queue
    int count;                // total number of processes

    sem_t mutex;
    // introducing semaphore to enable synchronization
    pid_t terminated_pids[1000];
    int t;
    struct Process History[1000]; // stores history after termination
    int history_count;            // total number of terminated processes
};

struct SharedQueue *ready_queue;

void show_history(struct SharedQueue *queue);
void save_history(struct Process p);

void initialize_queue(struct SharedQueue *queue)
{ // ready queue setter
    queue->front = 0;
    queue->end = 0;
    queue->count = 0;
    queue->t = 0;
    sem_init(&queue->mutex, 1, 1); // initializing semaphore;
    queue->history_count = 0;
}

void enqueue(struct SharedQueue *ready_queue, struct Process *new) // after quantum is over, process is enqueued back to ready queue
{
    // Copy the Process object to the shared memory queue
    memcpy(&(ready_queue->arr[ready_queue->end]), new, sizeof(struct Process));
    ready_queue->end++;
    ready_queue->count++;
}

void register_terminated_pid(struct SharedQueue *ready_queue, pid_t pid) // adds terminated processes to terminated pid array
{
    ready_queue->terminated_pids[ready_queue->t] = pid;
    ready_queue->t++;
}

struct Process *dequeue(struct SharedQueue *ready_queue) // process dequeued from ready queue for execution
{
    if (ready_queue->count > 0)
    {
        struct Process *dequeued_element = &(ready_queue->arr[ready_queue->front]);
        ready_queue->count--;
        ready_queue->front++;
        return dequeued_element;
    }
    return NULL;
}

int execute_command(char command[]); // function calling execl function to start execution

struct Process *allocate_newprocess() // creates new process for new command
{
    struct Process *new = (struct Process *)malloc(sizeof(struct Process));
    if (new == NULL)
    {
        perror("allocation failed");
        exit(1);
    }

    time(&new->start_time);
    new->termination_status = false;
    new->execution_time = 0;
    new->waiting_time = 0;

    return new;
}

void display() // displays shell prompt and takes user input
{

    int status = 1;

    do
    {
        printf("\nSimpleShelll_115:~$ ");
        char command[1000];
        fgets(command, 1000, stdin);

        if (strncmp(command, "submit", 6) == 0) // cmp first 6 char to know if it's submit ./a.out
        {
            // handle the submit command with priority

            char *token;
            char *command2;
            int priority = 1; // default priority
            int count = 0;

            token = strtok(command, " ");
            while (token != NULL)
            {
                count++;
                if (count == 2)
                {
                    command2 = token; // stores executable file string
                }
                if (count == 3)
                {
                    priority = atoi(token); // stores the priority of the process
                }
                token = strtok(NULL, " ");
            }

            execute_command(command2);
        }

        else
        {
            execute_command(command);
        }

    } while (status);
}

int if_pid_present(struct SharedQueue *ready_queue, pid_t pid) // checks if process exists in ready queue
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

struct Process *set_termination_status(struct SharedQueue *ready_queue, pid_t pid) // after termination, changes terminations status to true
{

    for (int i = ready_queue->front; i <= ready_queue->end; i++)
    {
        if (ready_queue->arr[i].pid == pid)
        {
            ready_queue->arr[i].termination_status = true;
        }
    }
}

int execute_command(char command[])
{
    int status = fork();
    if (status < 0) // Error hanlding for the fork()
    {
        printf("Something bad happened\n");
    }
    else if (status == 0) // child process
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
            p->pid = pid;
            p->new_process = false;

            strcpy(p->name, command);
            sem_wait(&ready_queue->mutex);
            // calling sem_wait
            enqueue(ready_queue, p);
            sem_post(&ready_queue->mutex);
            // repleninshing the value of the semaphore

            kill(pid, SIGSTOP);

            if (execl("/bin/sh", "sh", "-c", command, NULL) < 0)
            {
                perror("error occured;no instruction excuted!");
            }
        }
        else
        {
            int ret_status;
            waitpid(grand_child, &ret_status, 0);

            register_terminated_pid(ready_queue, grand_child);
        }
    }
    return status;
}

void scheduler_inside() // implementation of scheduler
{

    int x = 1;
    struct Process *p;

    while (x == 1)
    {

        if (ready_queue->count == 0) // when no process is in queue, loop will continue
        {
            continue;
        }

        int temp = nprocs;

        if (ready_queue->count < nprocs)
        {
            temp = ready_queue->count; // if number of process are less than number of CPUs, all processes will be executed
        }

        struct Process arr[temp]; // this will store the process to be execute
        int m = 0;

        for (int j = 0; j < temp; j++)
        {
            m++;
            sem_wait(&ready_queue->mutex);
            arr[j] = *(dequeue(ready_queue)); // loading process from ready queue to arr
            sem_post(&ready_queue->mutex);
        }

        for (int i = 0; i < temp; i++)
        {
            struct Process p2 = arr[i];
            kill(p2.pid, SIGCONT); // sending signal to resume execution to the process
        }

        sleep(tslice);

        for (int i = 0; i < temp; i++)
        {
            struct Process iter = arr[i];
            iter.execution_time += tslice; // adding tslice to it's execution time

            int child_status;
            int result = waitpid(iter.pid, &child_status, WNOHANG); // Check if the child process has terminated

            if (if_pid_present(ready_queue, iter.pid))
            {
                iter.termination_status = true; // Child process has terminated
                time(&iter.end_time);
                iter.waiting_time = (double)(iter.end_time - iter.start_time) - iter.execution_time;
                ready_queue->History[ready_queue->history_count] = iter;
                ready_queue->history_count++;
                save_history(iter);
                continue;
            }

            // may be this below blok is runnign for a proess which have not been forked yet

            if (iter.termination_status == false)
            {

                kill(iter.pid, SIGSTOP);
                sem_wait(&ready_queue->mutex);
                enqueue(ready_queue, &iter);
                sem_post(&ready_queue->mutex);
                // added back to ready_queue;
            }
        }
    }
}

void save_history(struct Process p)
{
    FILE *file = fopen(HISTORY_FILE, "a"); // open history file in append mode
    if (file == NULL)
    {
        perror("Error in opening file"); // checking error in file opening
        return;
    }

    int l = strlen(p.name) - 1;
    char newString[l];
    strncpy(newString, p.name, l);
    newString[l] = '\0';
    fprintf(file, "Command: %s, pid: %d, Execution time: %0.3lf, Waiting time: %0.3lf\n", newString, p.pid, p.execution_time, p.waiting_time); // writing onto the file

    fclose(file); // closing file before exiting
}

void show_history(struct SharedQueue *queue) // shows the history in the end of termination
{

    for (int i = 0; i < queue->history_count; i++)
    {

        struct Process p = queue->History[i];

        int l = strlen(p.name) - 1;
        char newString[l];
        strncpy(newString, p.name, l);
        newString[l] = '\0';

        printf("[%d] Command: %s, pid: %d, Execution time: %0.3lf, Waiting time: %0.3lf\n", i + 1, newString, p.pid, p.execution_time, p.waiting_time);
    }
}

static void my_handler(int signum) // to show history on termination and also close the scheduler
{
    if (signum == SIGINT)
    {
        printf("\n");
        show_history(ready_queue);

        int result = kill(scheduler_pid, SIGTERM);

        if (result == 0)
        {
            printf("\nScheduler process terminated successfully with shell\n");
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

    nprocs = atoi(argv[1]);                // atoi converts the NCPUs to int
    tslice = (double)atoi(argv[2]) / 1000; // atoi converts the Time slice to int

    if (nprocs <= 0 || tslice <= 0) // Error handling
    {
        printf("Invalid NPROCS or TSLICE value\n");
        return 1;
    }

    printf("ncpu: %d tslice:%0.3lf\n", nprocs, tslice);

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

        // Calling scheduler_inside function
        scheduler_inside();
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
