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

#define HISTORY_FILE "history.txt" // txt file where history will be stored
#define MAX_HISTORY_SIZE 256

struct Process
{
    pid_t pid;
    char name[256];

    bool termination_status;
    volatile bool new_process;

    double execution_time;
    double waiting_time;
    int number_cycle;
};

struct Process history[MAX_HISTORY_SIZE];
int history_count = 0;
void load_history(struct Process history[], int *history_count);
void save_history(const struct Process history[], int history_count);
void show_history(const struct Process history[], int history_count);

int nprocs = 0;
int tslice = 0;

struct SharedQueue
{
    struct Process arr[1000]; // Store Process objects directly
    int front;
    int end;
    int count;
    pid_t terminated_pids[1000];
    int t;
    int cycle;
};

struct SharedQueue *ready_queue;

void initialize_queue(struct SharedQueue *queue)
{
    queue->front = 0;
    queue->end = 0;
    queue->count = 0;
    queue->t = 0;
    queue->cycle = 0;
}

void enqueue(struct SharedQueue *ready_queue, struct Process *new)
{
    // Copy the Process object to the shared memory queue
    memcpy(&(ready_queue->arr[ready_queue->end]), new, sizeof(struct Process));
    ready_queue->end++;
    ready_queue->count++;
}
void register_terminated_pid(struct SharedQueue *ready_queue, pid_t pid)
{
    ready_queue->terminated_pids[ready_queue->t] = pid;
    ready_queue->t++;
}

struct Process *dequeue(struct SharedQueue *ready_queue)
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

int execute_command(char command[]);

struct Process *allocate_newprocess(struct SharedQueue *r)
{
    struct Process *new = (struct Process *)malloc(sizeof(struct Process));
    if (new == NULL)
    {
        perror("allocation failed");
        exit(1);
    }

    new->termination_status = false;
    new->number_cycle = r->cycle;
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
        // add_to_ready_queue(command);
        execute_command(command);

        // printf("\n");

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

int execute_command(char command[])
{
    int status = fork();
    if (status < 0)
    {
        printf("Something bad happened\n");
    }
    else if (status == 0)
    {
        int grand_child = fork();
        if (grand_child == 0)
        {
            // printf("this is grandchild\n");
            pid_t pid = getpid();
            struct Process *p = allocate_newprocess(ready_queue);
            p->pid = pid;
            p->new_process = false;

            // p->name=command;
            strcpy(p->name, command);
            enqueue(ready_queue, p);
            // printf("enqueued!\n");
            // printf("stop signal sent to process with pid:%d\n",getpid());
            // printf("name of the executable%s and pid:%d\n",ready_queue->arr[0].name,ready_queue->arr[0].pid);

            kill(pid, SIGSTOP);
            // printf("stop signal sent to process\n");
            if (execl("/bin/sh", "sh", "-c", command, NULL) < 0)
            {
                perror("error occured;no instruction excuted!");
            }
        }
        else
        {
            // this is child of parent;
            int ret_status;
            waitpid(grand_child, &ret_status, 0);
            // printf("this child waited for grandchild to finish\n");
            // set_termination_status(ready_queue,grand_child);
            register_terminated_pid(ready_queue, grand_child);
            // grandchild is pid of the grandchild;
            // printf("set termination successfull\n");
        }
    }

    int aise_hi;

    return status;
}

void scheduler_inside()
{
    // printf("scheduler pid:%d\n",getpid());
    int x = 1;
    struct Process *p;

    while (x == 1)
    {
        // printf("pid inside while loop(scheduler's pid):%d\n",getpid());
        if (ready_queue->count == 0)
        {

            // printf("ready_queue is empty\n");
            sleep(tslice);
            continue;
        }

        int temp = nprocs;

        if (ready_queue->count < nprocs)
        {
            temp = ready_queue->count;
        }
        struct Process arr[temp];
        int m = 0;
        ;
        for (int j = 0; j < temp; j++)
        {
            m++;
            arr[j] = *(dequeue(ready_queue));
        }
        for (int i = 0; i < temp; i++)
        {
            struct Process p2 = arr[i];
            p->waiting_time += (ready_queue->cycle - p->number_cycle) * tslice;
            p->execution_time += tslice;
            kill(p2.pid, SIGCONT);
        }

        sleep(tslice);
        ready_queue->cycle++;

        for (int i = 0; i < temp; i++)
        {
            struct Process iter = arr[i];
            int child_status;

            int result = waitpid(iter.pid, &child_status, WNOHANG);

            if (if_pid_present(ready_queue, iter.pid))
            {
                strcpy(history[history_count].name, iter.name);
                history[history_count].pid = iter.pid;
                history[history_count].execution_time = iter.execution_time;
                history[history_count].waiting_time = iter.waiting_time;
                history_count++;
                
                // Save the history to a file after termination
                save_history(history, history_count);

                iter.termination_status = true;
                continue;
            }

            if (iter.termination_status == false)
            {
                kill(iter.pid, SIGSTOP);
                enqueue(ready_queue, &iter);
            }
        }
    }
}

void load_history(struct Process history[], int *history_count) // this function displays the history on termination
{
    FILE *file = fopen(HISTORY_FILE, "r");

    if (file == NULL)
    {
        perror("Error in opening file");
        return;
    }

    *history_count = 0; // reseting the value of history count
    char line[256];

    while (fgets(line, sizeof(line), file) != NULL)
    {
        char command[256];
        pid_t pid;
        double execution_time;
        double waiting_time;
        sscanf(line, "%s %d %lf %lf", command, &pid, &execution_time, &waiting_time); // reading from the file

        // unloading the data into Process
        strcpy(history[*history_count].name, command);
        history[*history_count].pid = pid;
        history[*history_count].execution_time = execution_time;
        history[*history_count].waiting_time = waiting_time;

        (*history_count)++; // updating history count until the file is read completely
    }

    fclose(file); // close file before exiting
}

void save_history(const struct Process history[], int history_count)
{
    FILE *file = fopen(HISTORY_FILE, "w"); // open history file in w mode
    if (file == NULL)
    {
        perror("Error in opening file"); // checking error in file opening
        return;
    }

    for (int i = 0; i < history_count; i++)
    {
        fprintf(file, "%s %d %lf %lf\n", history[i].name, history[i].pid, history[i].execution_time, history[i].waiting_time); // writing onto the file
    }

    fclose(file); // closing file before exiting
}

void show_history(const struct Process history[], int history_count)
{
    printf(" \n");
    for (int i = 0; i < history_count; i++)
    {
        printf("[%d] Command: %s, pid: %d, Execution time: %lf, Waiting time: %lf\n", i + 1,  history[i].name, history[i].pid, history[i].execution_time, history[i].waiting_time);
    }
}


const char *shm_name = "/my_shared_memory";
const int shm_size = sizeof(struct SharedQueue);

static void my_handler(int signum)
{
    if (signum == SIGINT)
    {
        load_history(history, &history_count);

        show_history(history, history_count);

        exit(0);
    }
}

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

    printf("ncpu: %d tslice in seconds:%d\n", nprocs, tslice);

    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = my_handler;

    if (sigaction(SIGINT, &sig, NULL) == -1) // checking if sigaction returns -1, handle any errors in setting the signal handler
    {
        perror("sigaction error");
        return 1;
    }
    load_history(history, &history_count); // history_count values will be reset before any exec starts

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

    if (pid == -1)
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0)
    {

        sleep(8);

        scheduler_inside();
    }
    else
    { // Parent process

        display();
        wait(NULL); // Wait for the child to finish

        munmap(ready_queue, shm_size);
        close(shm_fd);

        // Remove shared memory object
        shm_unlink(shm_name);
    }

    return 0;
}
