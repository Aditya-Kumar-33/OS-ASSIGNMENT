#include<stdio.h>

int main(){
    printf("hello");
    
    sleep(10);
    printf("executing");


    return 0;
}





// cp





// Define the Process structure
// struct Process {
//     pid_t pid;
//     struct Process *next;
//     char name[256];
//     int execution_time;
//     int waiting_time;
// };

// // Define the Queue structure
// struct Queue {
//     struct Process *front;
//     struct Process *rear;
// };

// // Function to create an empty queue
// struct Queue *createQueue() {
//     struct Queue *queue = (struct Queue *)malloc(sizeof(struct Queue));
//     if (queue == NULL) {
//         perror("Queue creation failed");
//         exit(1);
//     }
//     queue->front = queue->rear = NULL;
//     return queue;
// }

// // Function to check if the queue is empty
// bool is_empty(struct Queue *queue) {
//     return queue->front == NULL;
// }

// // Function to add a process to the queue (enqueue)
// void enqueue(struct Queue *queue, pid_t pid, const char *name, int execution_time, int waiting_time) {
//     struct Process *newProcess = (struct Process *)malloc(sizeof(struct Process));
//     if (newProcess == NULL) {
//         perror("Memory allocation failed");
//         exit(1);
//     }

//     newProcess->pid = pid;
//     newProcess->next = NULL;
//     snprintf(newProcess->name, sizeof(newProcess->name), "%s", name);
//     newProcess->execution_time = execution_time;
//     newProcess->waiting_time = waiting_time;

//     if (is_empty(queue)) {
//         queue->front = queue->rear = newProcess;
//     } else {
//         queue->rear->next = newProcess;
//         queue->rear = newProcess;
//     }
// }

// // Function to remove a process from the queue (dequeue)
// struct Process *dequeue(struct Queue *queue) {
//     if (is_empty(queue)) {
//         return NULL;
//     }

//     struct Process *removedProcess = queue->front;
//     queue->front = queue->front->next;

//     if (queue->front == NULL) {
//         queue->rear = NULL;
//     }

//     return removedProcess;
// }

// // Function to peek at the front of the queue
// struct Process *peek(struct Queue *queue) {
//     return queue->front;
// }

// // Function to free the memory used by the queue
// void destroyQueue(struct Queue *queue) {
//     while (!is_empty(queue)) {
//         struct Process *removedProcess = dequeue(queue);
//         free(removedProcess);
//     }
//     free(queue);
// }

// int main() {
//     struct Queue *queue = createQueue();

//     // Example usage:
//     enqueue(queue, 1234, "Process A", 10, 5);
//     enqueue(queue, 5678, "Process B", 8, 3);

//     struct Process *frontProcess = peek(queue);
//     if (frontProcess != NULL) {
//         printf("Front of the queue: %s\n", frontProcess->name);
//     }

//     struct Process *removedProcess = dequeue(queue);
//     if (removedProcess != NULL) {
//         printf("Dequeued Process: %s\n", removedProcess->name);
//         free(removedProcess);
//     }

//     // Clean up and free memory
//     destroyQueue(queue);

//     return 0;
// }



// cp