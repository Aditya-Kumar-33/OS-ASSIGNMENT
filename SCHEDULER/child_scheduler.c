#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>


struct Process
{
    pid_t pid;
    struct Process *next;
    char name[256];
    int execution_time;
    int waiting_time;
};


// creating shared segment 
struct SharedQueue {
    struct Process* head;
    struct Process* tail;
    int count;
};

// Your child_scheduler logic goes here

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <shm_fd>\n", argv[0]);
        return 1;
    }

    // Get the shared memory file descriptor from the command-line arguments
    int shm_fd = atoi(argv[1]);

    // Open the shared memory segment using the file descriptor
    struct SharedQueue* ready_queue = (struct SharedQueue*)mmap(
        NULL,
        sizeof(struct SharedQueue),
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        shm_fd,
        0
    );

    if (ready_queue == MAP_FAILED) {
        perror("mmap in child_scheduler");
        return 1;
    }
    printf("we are inside the child_scheduler\n");

    // Access and manipulate the ready_queue as needed

    // Don't forget to unmap the shared memory when you're done
    munmap(ready_queue, sizeof(struct SharedQueue));

    return 0;
}




























// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <signal.h>
// #include <sys/types.h>
// #include <sys/wait.h>
// #include <string.h>
// #include <stdbool.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>
// #include <sys/mman.h>
// #include <fcntl.h>
// struct Process
// {
//     pid_t pid;
//     struct Process *next;
//     char name[256];
//     int execution_time;
//     int waiting_time;
// };


// // creating shared segment 
// struct SharedQueue {
//     struct Process* head;
//     struct Process* tail;
//     int count;
// };


// int main(int argc, char *argv[]) {
//     printf("entered  the  scheduler with pid:%d\n",getpid());
//     printf("before %s\n",argv[1]);
    
//     if (argc != 2) {
//         // printf("when argv is not equal to 2");
//         fprintf(stderr, "Usage: %s <shm_fd>\n", argv[0]);
//         exit(EXIT_FAILURE);
//     }

//     int shm_fd2 = atoi(argv[1]);
//     printf("within scheduler fd:  %d",shm_fd2);

//     // Map the shared memory into the child process's address space
//     // size_t shm_size = sizeof(struct SharedQueue);
//     // struct SharedQueue* ready_queue = (struct SharedQueue*)mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd2, 0);
//     // if (ready_queue == MAP_FAILED) {
//     //     perror("mmap");
//     //     exit(EXIT_FAILURE);
//     // }

//     // Access and use the shared memory
//     // printf("Child Process: Value read from shared memory: %d\n", ready_queue->head->execution_time);

//     // Unmap shared memory
//     // munmap(ready_queue, shm_size);

//     exit(EXIT_SUCCESS);
// }
