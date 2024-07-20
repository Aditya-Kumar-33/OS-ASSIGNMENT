submit ./a
submit ./b
submit ./fib
submit ./c


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

void enqueue(struct SharedQueue* ready_queue, struct Process* new)