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

int nprocs = 0;
int tslice = 0;

struct Process {
    pid_t pid;
    struct Process* next;
    char name[256];
    bool termination_status;
    int execution_time;
};

struct SharedQueue {
    struct Process* head;
    struct Process* tail;
    int count;
};

struct SharedQueue* ready_queue;

struct Process* allocate_newprocess() {
    struct Process* new = (struct Process*)malloc(sizeof(struct Process));
    if (new == NULL) {
        perror("Allocation failed");
        exit(1);
    }
    new->termination_status = false;
    new->next = NULL;

    return new;
}

void enqueue(struct Process* new) {
    if (ready_queue->head == NULL) {
        ready_queue->head = new;
        ready_queue->tail = new;
    } else {
        ready_queue->tail->next = new;
        ready_queue->tail = new;
    }
    ready_queue->count++;
}

struct Process* dequeue() {
    if (ready_queue->count > 0) {
        struct Process* dequeued_element = ready_queue->head;
        ready_queue->head = dequeued_element->next;
        ready_queue->count--;
        return dequeued_element;
    }
    return NULL;
}

void display() {
    int status;

    do {
        printf("\nSimpleShell_115:~$ ");
        char command[1000];
        fgets(command, 1000, stdin);
        status = execute_command(command);
    } while (status);
}

int execute_command(char command[]) {
    int status = fork();

    if (status < 0) {
        printf("Error forking\n");
        exit(1);
    } else if (status == 0) {
        pid_t pid = getpid();
        struct Process* p = allocate_newprocess();
        p->pid = pid;
        strcpy(p->name, command);
        p->execution_time = 5; // Replace with actual execution time
        enqueue(p);

        kill(pid, SIGSTOP);

        if (execl("/bin/sh", "sh", "-c", p->name, NULL) < 0) {
            perror("Error executing command");
            exit(1);
        }
    }

    sleep(1);
    return status;
}

void scheduler_inside() {
    while (1) {
        int temp = nprocs;
        if (ready_queue->count < nprocs) {
            temp = ready_queue->count;
        }

        struct Process* arr[temp];

        for (int j = 0; j < temp; j++) {
            arr[j] = dequeue();
        }

        for (int i = 0; i < temp; i++) {
            struct Process* p2 = arr[i];
            if (p2->execution_time > 0) {
                kill(p2->pid, SIGCONT);
            }
        }

        sleep(tslice);

        for (int i = 0; i < temp; i++) {
            struct Process* iter = arr[i];
            if (iter->execution_time <= 0) {
                iter->termination_status = true;
            }
            if (!iter->termination_status) {
                kill(iter->pid, SIGSTOP);
                enqueue(iter);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Enter 2 arguments after the command: the first argument is NCPUs and the second argument is TSLICE in milliseconds\n");
        return 1;
    }

    nprocs = atoi(argv[1]);
    tslice = atoi(argv[2]);

    if (nprocs <= 0 || tslice <= 0) {
        printf("Invalid NCPUs or TSLICE value\n");
        return 1;
    }

    printf("ncpu: %d tslice: %d\n", nprocs, tslice);

    const char* shm_name = "/my_shared_memory";

    int shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, sizeof(struct SharedQueue));
    ready_queue = (struct SharedQueue*)mmap(NULL, sizeof(struct SharedQueue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    ready_queue->head = NULL;
    ready_queue->tail = NULL;
    ready_queue->count = 0;

    pid_t child_pid = fork();

        if (child_pid < 0) {
        perror("Error in creating child");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) {
        char shm_fd_str[10];
        sprintf(shm_fd_str, "%d", shm_fd);

        ready_queue = (struct SharedQueue*)mmap(NULL, sizeof(struct SharedQueue), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

        if (ready_queue == MAP_FAILED) {
            perror("mmap in child_scheduler");
            exit(EXIT_FAILURE);
        }

        scheduler_inside();
        // This function runs the scheduler logic in an infinite loop.

        // exec some code or function, or simply exit to end the child process.
        // Use exec* functions if you need to run another program here.
    }

    display();

    // Wait for the child to complete
    wait(NULL);

    munmap(ready_queue, sizeof(struct SharedQueue));
    close(shm_fd);
    shm_unlink(shm_name);

    exit(EXIT_SUCCESS);
}

