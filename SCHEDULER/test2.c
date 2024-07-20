#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/wait.h>

struct Process {
    pid_t pid;
    struct Process* next;
    char name[256];
    bool termination_status;
    int execution_time;
    volatile bool new_process;
    int waiting_time;
};

struct SharedQueue {
    struct Process arr[1000];
    int front;
    int end;
    int count;
    

};

struct SharedQueue* ready_queue;

void initialize_queue(struct SharedQueue* queue) {
    queue->front = 0;
    queue->end = 0;
    queue->count = 0;
    queue->t=0;
}

void enqueue(struct SharedQueue* ready_queue, struct Process* new) {
    if (ready_queue->count < 1000) {
        memcpy(&(ready_queue->arr[ready_queue->end]), new, sizeof(struct Process));
        ready_queue->end++;
        ready_queue->count++;
    }
}

struct Process* dequeue(struct SharedQueue* ready_queue) {
    if (ready_queue->count > 0) {
        struct Process* dequeued_element = &(ready_queue->arr[ready_queue->front]);
        ready_queue->front++;
        ready_queue->count--;
        return dequeued_element;
    }
    return NULL;
}

const char* shm_name = "/my_shared_memory";
const int shm_size = sizeof(struct SharedQueue);

int main() {
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

    // Initialize the shared queue structure in the parent
    initialize_queue(ready_queue);

    int pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { // Child process
        // Access and modify the shared structure (child)
        printf("Child Process:\n");
        struct Process new_process;
        new_process.execution_time = 42;
        enqueue(ready_queue, &new_process);
        exit(0); // Exit the child process
    } else { // Parent process
        wait(NULL); // Wait for the child to finish

        // Access and print the shared structure (parent)
        printf("Parent Process:\n");
        struct Process* dequeued_process = dequeue(ready_queue);
        if (dequeued_process != NULL) {
            printf("Dequeued Process Execution Time: %d\n", dequeued_process->execution_time);
        }

        // Unmap and close shared memory
        munmap(ready_queue, shm_size);
        close(shm_fd);

        // Remove shared memory object
        shm_unlink(shm_name);
    }

    return 0;
}
