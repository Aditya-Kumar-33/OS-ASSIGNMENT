#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#define HISTORY_FILE "history.txt" // txt file where history will be stored
#define MAX_HISTORY_SIZE 256

struct Node // to store history
{
    char command[256];
    pid_t pid; // pid_t data type is a signed integer type , representing process ID
    time_t execution_time;
    double duration;
};

char *read_user_input();
// fn that takes user input
int launch(char *command);
// launches a command and passes it to appopriate fn handler
int create_process_and_run(char *command);
// executes a the command
int parse_input(char *command);
// responsible for  parsing and splitting the command
int count_token_pipes(char *command);
// counts token split about '|'
int implement_pipe(char *command1, char *command2);
// execute commands that involves single pipe
int execute_multi_pipes(int n, char *arr[]);
// execute commands that involves multi pipe
int global_flag = 0;
// a global flag used in piped command

void load_history(struct Node history[], int *history_count);
void save_history(const struct Node history[], int history_count);
void show_history(const struct Node history[], int history_count);
time_t start;
time_t end;
struct Node history[MAX_HISTORY_SIZE];
int history_count = 0;

// global flag is being created to unnecessary exxeccution of  piped indiidual commands

// declaring these functions before hand to avoid implicit declaration

void display()
{
    int status;
    do
    {
        sleep(1);
        printf("\nSimpleShelll_115:~$ ");
        char *command = read_user_input();
        // printf("printing %s",command);
        char* to_compare="history\n";
        if(strcmp(command,to_compare)==0){
            // printf("executed!\n");
            show_history(history, history_count);
            continue;

        }
        global_flag = 0;
        // simply takes the input command and stores in a var

        status = parse_input(command);
        // parse_input function includes parsing the input command
        // it automatically transfers the control to the function that handles pipe commands
        // if encountered a command that involves pipes
        // returns 0 if pipe command was executed

        if (status != 0)
        {
            // if status=0 then it means no pipes involved
            // it implies command will run as usual
            // if status =0 it means it was a piped command and it has been executed
            // thus,prompting to take input again

            continue;
        }
        if (command != NULL)
        {
            status = launch(command);
            free(command);
            // command corresponds to allocated memory in heap
            // needs to be freed
        }
    } while (status);
}

// fn to count no_of piped commands by splitting about '|'
int count_token_pipes(char *command)
{
    char delimiter[] = "|";
    char *token = strtok(command, delimiter);
    // returns a pointer to string separated by '|'
    int i = 0;
    while (token != NULL)
    {
        i++;
        // printf("%s",token);
        token = strtok(NULL, delimiter);
    }
    return i;
}

int parse_input(char *command)
{
    int status;
    char delimiter[] = "|";
    char *temp = (char *)malloc(strlen(command) + 1);
    strcpy(temp, command);

    char *token;

    int n = count_token_pipes(temp);

    char *args[n];
    token = strtok(command, delimiter);
    args[0] = token;
    for (int i = 1; i < n; i++)
    {
        token = strtok(NULL, delimiter);
        args[i] = token;
    }

    if (n == 2)
    {
        // implemented a seprate function that executes a command
        // that involves only two piped commands

        status = fork();
        if (status < 0)
        {
            perror("error!");
            exit(-1);
        }
        else if (status == 0)
        {
            // global_flag=1;
            implement_pipe(args[0], args[1]);
        }
        else
        {
            int ret_status;

            wait(&ret_status);
            if (ret_status == -1)
            {
                printf("error occured");
            }
            else if (WIFEXITED(ret_status))
            {
                // printf("this is in parsing!-----------------------------------\n");
                printf("\nchild process exited normally\nExit_Status =%d\n", WEXITSTATUS(ret_status));
            }
            // also needs to store return status and check if exited normaly
            // printf("“I am the parent Shell”");
            return ret_status;
        }
    }
    else if (n > 2)
    {
        // this involves a piped command that involves multiple pipes
        int n2 = fork();
        if (n2 < 0)
        {
            perror("fork failed in n>2 piped command");
            exit(-1);
        }
        if (n2 == 0)
        {
            global_flag = 1;
            // printf("it should print first---------------\n");
            execute_multi_pipes(n, args);
        }
        else if (n2 > 0)
        {
            // implemented in parent process
            int new_status;
            waitpid(n2, &new_status, 0);
            // wait(NULL);
            printf("completed piped commannds-------------\n");
            return 1;
        }

        // status=execute_multi_pipes(n,args);
        // printf("piped command executed successfully! and statud:%d\n",status);
        // return status;
        // if status!=0 our  shell will ask to give input again since it has already eexecuted a piped command
    }
    return 0;
    // since it means it is returned by parent process
    // hence as pere our code we dot want to run again pipe
}

void load_history(struct Node history[], int *history_count) // this function displays the history on termination
{
    FILE *file = fopen(HISTORY_FILE, "r");

    if (file == NULL)
    {
        return;
    }

    *history_count = 0; // reseting the value of history count
    char line[256];

    while (fgets(line, sizeof(line), file) != NULL)
    { //&& *history_count < MAX_HISTORY_SIZE

        char command[256];
        pid_t pid;
        time_t execution_time;
        double duration;
        sscanf(line, "%s %d %ld %lf", command, &pid, &execution_time, &duration); // reading from the file

        // unloading the data into Node
        strcpy(history[*history_count].command, command);
        history[*history_count].pid = pid;
        history[*history_count].execution_time = execution_time;
        history[*history_count].duration = duration;

        (*history_count)++; // updating history count until the file is read completely
    }

    fclose(file); // close file before exiting
}

void save_history(const struct Node history[], int history_count)
{
    FILE *file = fopen(HISTORY_FILE, "w"); // open history file in w mode
    if (file == NULL)
    {
        perror("Error in opening file"); // checking error in file opening
        return;
    }

    for (int i = 0; i < history_count; i++)
    {
        fprintf(file, "%s %d %ld %lf\n", history[i].command, history[i].pid, history[i].execution_time, history[i].duration); // writing onto the file
    }

    fclose(file); // closing file before exiting
}

char *read_user_input()
{
    char command[1000];
    char *command2 = NULL;
    // since we will lose command as it is a local variable

    fgets(command, 1000, stdin);
    command2 = (char *)malloc(strlen(command) + 1);
    // also need to free this memory

    if (command2 != NULL)
    {
        strcpy(command2, command);
    }
    else
    {
        perror("error in allocation of memory to character pointer");
    }
    return command2;
}

// launching a command
int launch(char *command)
{
    int status;
    status = create_process_and_run(command);
    return status;
}

int count_token(char *command)
{
    // counts no of strings separeted by whiteespace character
    char delimiter[] = " \n\0";
    char *token = strtok(command, delimiter);
    int i = 0;
    while (token != NULL)
    {
        i++;
        // printf("%s",token);
        token = strtok(NULL, delimiter);
    }
    return i;
}

int implement_pipe(char *command1, char *command2)
{
    // implements a single piped command
    int status;

    int arr[2];
    if (pipe(arr) < 0)
    {
        perror("piping failed!");
        exit(-1);
    }
    pid_t child = fork();
    if (child < 0)
    {
        perror("fork failed!");

        exit(-1);
    }
    else if (child == 0)
    {

        close(arr[0]);
        // closing read  since it is  supposed to write and need to free unused resources;
        dup2(arr[1], STDOUT_FILENO);

        close(arr[1]);
        // closing the original write end since we have its duplicate now;
        // printf("giving to input:%s\n",command1);
        if (execl("/bin/sh", "sh", "-c", command1, NULL) < 0)
        {
            perror("error occured;no instruction excuted!");
        }
    }
    else
    {
        // wait(NULL);
        pid_t child2 = fork();
        if (child2 < 0)
        {
            perror("fork for child2 failed!");
            exit(-1);
        }
        else if (child2 == 0)
        {
            close(arr[1]);
            // closing the write end;
            dup2(arr[0], STDIN_FILENO);
            close(arr[0]);
            // status=create_process_and_run(command2);
            // execlp("ls","ls","-l",NULL);
            if (execl("/bin/sh", "sh", "-c", command2, NULL) < 0)
            {
                perror("error occured;no instruction excuted!");
            }
        }
        else
        {
            close(arr[0]);
            close(arr[1]);

            wait(NULL);
        }
    }
    exit(1);
}

// cp
int execute_multi_pipes(int n, char *arr[])
{
    int pipes[n - 1][2];

    // Create n-1 pipes
    for (int i = 0; i < n - 1; i++)
    {
        if (pipe(pipes[i]) < 0)
        {
            perror("pipe creation failed");
            exit(-1);
        }
    }

    int n_comm = 0;
    for (int i = 0; i < n; i++)
    {
        pid_t child = fork();

        if (child < 0)
        {
            perror("fork failed");
            exit(-1);
        }
        else if (child == 0)
        {
            // Child process
            if (i != 0)
            {
                // Redirect input from the previous pipe
                dup2(pipes[i - 1][0], STDIN_FILENO);
                close(pipes[i - 1][0]);
            }

            if (i != n - 1)
            {
                // Redirect output to the next pipe
                dup2(pipes[i][1], STDOUT_FILENO);
                close(pipes[i][1]);
            }

            // Close all pipe ends in the child
            for (int j = 0; j < n - 1; j++)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            // Execute the command
            if (execl("/bin/sh", "sh", "-c", arr[n_comm], NULL) < 0)
            {
                perror("exec failed");
                exit(-1);
            }
        }
        else
        {
            // Parent process
            if (i != 0)
            {
                close(pipes[i - 1][0]);
                close(pipes[i - 1][1]);
            }

            n_comm++;

            if (i == n - 1)
            {
                // Wait for the last child
                wait(NULL);
                return 1;
            }
        }
    }
}

// executing multi pipes
// not used for execution
int implement_multi_pipes(int n, char *arr[])
{

    int pipes[n - 1][2];
    // since if there are n pipe commands
    // it involves n-1 pipes;
    for (int i = 0; i < n - 1;)
    {
        if (pipe(pipes[i]) < 0)
        {
            // here creating n-1 pipes for  n commands  involving pip
            perror("piping failed for  multi  pipes:");
            printf("%d\n", i);
            exit(-1);
        }
    }
    int n_comm = 0;
    for (int i = 0; i < n; i++)
    {
        // working on individual pipes
        // check if you  can close other ends of other pipes too
        pid_t child1 = fork();
        if (child1 < 0)
        {
            perror("fork failed for child1");
            exit(-1);
        }
        else if (child1 == 0)
        {
            // this is where the execution of child begins
            // close(pipes[i][0])
            // we cant close pipes end  here;this is a general implementation
            // and pipes in between will need both ends to be opened
            dup2(pipes[i][1], STDOUT_FILENO);
            close(pipes[i][1]);
            // we have its duplicate now
            if (execl("/bin/sh", "sh", "-c", arr[n_comm], NULL) < 0)
            {
                perror("error occured;no instruction excuted!");
            }
            else
            {

                pid_t child2 = fork();
                if (child2 < 0)
                {
                    perror("fork for child2 failed!");
                    exit(-1);
                }
                else if (child2 == 0)
                {
                    // close(arr[1]);
                    // cannnot close the write end since we might need to write to another;
                    dup2(pipes[i][0], STDIN_FILENO);
                    close(pipes[i][0]);
                    if (i != n - 1)
                    {
                        // if we want to continue piping;
                        dup2(pipes[i][1], STDOUT_FILENO);
                        close(pipes[i][1]);
                        if (execl("/bin/sh", "sh", "-c", arr[n_comm + 1], NULL) < 0)
                        {
                            perror("error occured;no instruction excuted!");
                        }
                    }

                    // below is the case when we want to all pipes have been executed and  final result is to be displayed on stdout

                    if (execl("/bin/sh", "sh", "-c", arr[n_comm + 1], NULL) < 0)
                    {
                        perror("error occured;no instruction excuted!");
                    }
                }
                else
                {
                    close(arr[i][0]);
                    close(arr[i][1]);

                    wait(NULL);
                }

                // error
            }
        }
        i += 1;
        n_comm += 2;
    }
}

int create_process_and_run(char *command)
{
    time(&start); // time when the process starts
    pid_t pid = getpid();

    // printf("executing command:%s\n",command);

    // printf("executing create_process globlflag:%d\n ",global_flag);

    if (global_flag == 1)
    {
        return 1;
    }
    char delimiter[] = " \n\0";
    int status = fork();
    char *temp = (char *)malloc(strlen(command) + 1);
    strcpy(temp, command);
    //  assigning a temp because strtok changes command
    char *token;

    int n = count_token(temp);
    //  printf("counted tokens:%d\n",n);
    char *args[n + 1];
    token = strtok(command, delimiter);
    args[0] = token;
    for (int i = 1; i < n; i++)
    {
        token = strtok(NULL, delimiter);
        args[i] = token;
        // printf("inside:%s\n",token);
    }
    args[n] = NULL;

    if (status < 0)
    {
        printf("Something bad happened\n");
        exit(-1);
    }
    else if (status == 0)
    {
        if (execvp(args[0], args) < 0)
        {
            perror("error occured;no instruction excuted!");
        }
        // this is a child process
        // hence we can write implementation for this
        //  printf(“I am the child process\n”);
    }
    else
    {
        int ret_status;

        waitpid(status, &ret_status, 0);
        if (ret_status == -1)
        {
            printf("error occured");
        }
        else if (WIFEXITED(ret_status))
        {
            // printf("this is in create process----------------\n");
            printf("\nchild process exited normally\nExit_Status =%d\n", WEXITSTATUS(ret_status));
        }
        // also needs to store return status and check if exited normaly
        // printf("“I am the parent Shell”");
    }

    time(&end); // time when the process ends
    double duration = difftime(end, start);

    // Store the command history entry
    strcpy(history[history_count].command, command);
    history[history_count].pid = pid;
    history[history_count].execution_time = start;
    history[history_count].duration = duration;
    history_count++;

    // Save the history to a file after each command
    save_history(history, history_count);
    return status;
}

static void my_handler(int signum)
{
    if (signum == SIGINT)
    {
        load_history(history, &history_count);

        for (int i = 0; i < history_count; i++)
        {
            time_t Time = history[i].execution_time;
            time(&Time);
            char *timeString = ctime(&Time);

            printf("[%d] %s , PID: %d , Execution time: %s , Duration: %lf\n", i + 1, history[i].command, history[i].pid, timeString, history[i].duration);
        }
        exit(0);
    }
}

void show_history(const struct Node history[], int history_count)
{
    printf(" \n");
    for (int i = 0; i < history_count; i++)
    {
        printf("[%d] %s\n", i + 1, history[i].command);
    }
}

int main()
{
    struct sigaction sig;
    memset(&sig, 0, sizeof(sig));
    sig.sa_handler = my_handler;

    if (sigaction(SIGINT, &sig, NULL) == -1) // checking if sigaction returns -1, handle any errors in setting the signal handler
    {
        perror("sigaction error");
        return 1;
    }
    load_history(history, &history_count); // history_count values will be reset before any exec starts

    display();

    return 0;
}