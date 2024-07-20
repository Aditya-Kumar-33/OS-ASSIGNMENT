#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdbool.h>

struct Process
{
    pid_t pid;
    struct Process *next;
    char name[256];
    int execution_time;
    int waiting_time;
};







int NCPU;
struct Process* head = NULL; // Initialize head and tail to NULL
struct Process* tail = NULL;
int count = 0;

// int numCPUs()
// {
//     for(int i=0;i<5;i++){
//         Process* p=(process*) malloc()
//     }
//     FILE *fp = fopen("/proc/cpuinfo", "r");
//     if (fp == NULL)
//     {
//         perror("Error in getting number of CPUs\n");
//         return 1;
//     }

//     int numCPUs = 0;
//     char line[100];
//     while (fgets(line, sizeof(line), fp))
//     {
//         if (strstr(line, "processor") != NULL)
//         {
//             numCPUs++;
//         }
//     }

//     fclose(fp);
//     return numCPUs;
// }

// Allocating memory for a new process
struct Process *allocate_newprocess()
{
    struct Process *new = (struct Process *)malloc(sizeof(struct Process));
    if (new == NULL)
    {
        perror("allocation failed");
        exit(1);
    }

    return new;
}

// Add a new process to the back of the linked list
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

// Dequeue a process from the ready queue
struct Process *dequeue()
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

// Signal handler to stop a process when quantum is over
static void Time_over(int signum)
{
    struct Process *p = dequeue();
    if (p != NULL) // if the ready queue is not empty
    {
        kill(p->pid, SIGSTOP); // Kill sends a signal to the process to stop execution
        enqueue(p);            // Put the process back at the end of the queue
    }
}

int main()
{
    // NCPU = numCPUs();
    
    Process* p=allocate_newprocess();
    enqueue(p);

   
    

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
