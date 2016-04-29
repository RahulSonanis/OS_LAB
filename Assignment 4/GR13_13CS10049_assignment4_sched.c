/*  Group -13
	Vishwas Jain	13CS10053
	Rahul Sonanis	13CS10049
	Assignment		4
	Scheduler Code
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>


#define   BUFF_SIZE   1024
#define   NOTIFY      SIGQUIT
#define   SUSPEND     SIGINT
#define   IOREQUEST   SIGUSR1
#define   TERMINATE   SIGUSR2
#define   RR          1
#define   PR          2


int sched_algo;
int timeQuanta;
int msgqid;
int numberOfProcessCompleted;
bool isIORequest;
bool isTerminate;

void regularRR();
void priorityRR();
void newProcess();
void ioSignalHand(int signum);
void terSignalHand(int signum);
void exiting(int signum);
void PrintReadyQueue();
void newProirProcess();



// A structure to represent a process
typedef struct process {
    int pid;
    int Priority;
    clock_t start, end, response, last_wait;
    bool scheduled;
    bool waiting;
    double waiting_time;
    double response_time;
    double turnaround_time;
}process;

typedef struct msgbuff {
    long mtype;               /* message mtype, must be > 0 */
    char mtext[BUFF_SIZE];    /* message data */
}msgbuff;


// A structure to represent a queue
typedef struct Queue
{
    int front, rear, size;
    int capacity;
    process** array;
}Queue;



// function to create a queue of given capacity. It initializes size of
// queue as 0
Queue* createQueue(int capacity)
{
    Queue* queue = (Queue*) malloc(sizeof(Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
    queue->rear = capacity - 1;  // This is important, see the enqueue
    queue->array = (process**) malloc(queue->capacity * sizeof(process*));
    return queue;
}

// Queue is full when size becomes equal to the capacity
bool isFull(Queue* queue)
{  return (queue->size == queue->capacity);  }


// Queue is empty when size is 0
bool isEmpty(Queue* queue)
{  return (queue->size == 0); }


// Function to add an item to the queue.  It changes rear and size
void enqueue(Queue* queue, process* item)
{
    if (isFull(queue))
    return;
    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    //  printf("%d enqueued to queue\n", item);
}

// Function to remove an item from queue.  It changes front and size
process* dequeue(Queue* queue)
{
    process* item = queue->array[queue->front];
    queue->front = (queue->front + 1)%queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

// Function to get front of queue
process* front(Queue* queue)
{
    return queue->array[queue->front];
}

// Function to get rear of queue
process* rear(Queue* queue)
{
    return queue->array[queue->rear];
}
// Function to add an item to the queue.  It changes rear and size

void enqueueWithPriority(Queue* queue, process* item)
{
    if (isFull(queue))
    return;

    queue->rear = (queue->rear + 1)%queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;

    int j = queue->rear;
    while(j != queue->front){
        int k = (j-1)%queue->capacity;
        if(queue->array[k]->Priority < queue->array[j]->Priority){
            process* temp = queue->array[k];
            queue->array[k] = queue->array[j];
            queue->array[j] = temp;
            j=k;
        }
        else break;
    }
    //  printf("%d enqueued to queue\n", item);
}


Queue* ReadyQueue;
process* Total_processes[50];
int total_number_process = 0;

int main(int argc, char* argv[]){

    remove("result.txt");                                                  // removing the file "history.txt" if it already exists
    //Checking if the input format is correct
    //scheduling-algorithm-mtype gives the mtype of scheduling algorithm to run
    if(argc != 3 || (strcmp(argv[1],"RR") && strcmp(argv[1],"PR")) || atoi(argv[2]) <= 0 ){
        printf("Usage: ./sched scheduling-algorithm-mtype time-quanta\n\n");
        printf("scheduling-algorithm-mtype = 'RR' : Regular round robin\nscheduling-algorithm-mtype = 'PR' : Priority based round robin\n");
        printf("time-quanta > 0\n");
        printf("Exiting...\n\n");
        exit(0);
    }

    signal(IOREQUEST, ioSignalHand);
    signal(TERMINATE, terSignalHand);
    signal(SIGINT, exiting);

    timeQuanta = atoi(argv[2]);
    numberOfProcessCompleted = 0;

    msgqid = msgget(ftok("/tmp",100), IPC_CREAT | IPC_EXCL | 0666);
    if(msgqid == -1){
        perror("\nError in creating the message queue");
        printf("Exiting...\n\n");
        exit(0);
    }

    //Regular round robin scheduling algorithm
    if(!strcmp(argv[1],"RR")){
        printf("\n\nType of scheduling algorithm = Regular Round Robin with time quanta = %d\n\n",timeQuanta);
        sched_algo = RR;
    }

    //Priority based round robin scheduling algorithm
    if(!strcmp(argv[1],"PR")){
        printf("\n\nType of scheduling algorithm = Priority Based Round Robin with time quanta = %d\n\n",timeQuanta);
        sched_algo = PR;
    }
    regularRR();
    if(msgctl(msgqid, IPC_RMID, NULL) == -1){
        perror("\nError in removing the message queue\n");
    }



    printf("\nFinish\n");

    return 0;
}

void regularRR(){
    ReadyQueue = createQueue(50);
    int i;
    while(1){
        if(!isEmpty(ReadyQueue)){
            isIORequest = false;
            isTerminate = false;

            process* current = front(ReadyQueue);
            dequeue(ReadyQueue);
            //PrintReadyQueue();
            kill(current->pid, NOTIFY);
            printf("Process <%d> is running.\n", current->pid);
            if(!current->scheduled){
                current->response = clock();
                current->scheduled = true;
            }
            if(current->waiting){
                current->waiting = false;
                clock_t t = clock();
                current->waiting_time += (double)(t - current->last_wait)/CLOCKS_PER_SEC;
            }
            for(i = 0; i < timeQuanta; i++){
                if(isIORequest || isTerminate)
                    break;
                usleep(50);
            }
            if(isIORequest){
                printf("Process <%d> requests I/O.\n", current->pid);
            }
            else if(isTerminate){
                numberOfProcessCompleted = numberOfProcessCompleted + 1;
                printf("Process <%d> terminates.\n", current->pid);
                current->end = clock();
                FILE* result = fopen("result.txt","a+");
                double Turn_around = (double)(current->end - current->start ) / CLOCKS_PER_SEC;
                double Response_time = (double)(current->response - current->start ) / CLOCKS_PER_SEC;
                current->turnaround_time = Turn_around;
                current->response_time = Response_time;
                fprintf(result, "\nProcess pid = %d\tWaiting time=%lf\tTurn-around time = %lf\tResponse-time=%lf\n",current->pid, current->waiting_time,Turn_around, Response_time);
                fclose(result);
            }
            else{
                printf("Process <%d> is suspended.\n", current->pid);
                kill(current->pid, SUSPEND);
                current->waiting = true;
                current->last_wait = clock();
                enqueue(ReadyQueue, current);
                //PrintReadyQueue();
            }
        }

        //Waiting for the processes to arrive in the ready queue (if any)
        newProcess();

        if(numberOfProcessCompleted >= 4){
            FILE* result = fopen("result.txt","a+");
            double average_turnaround = 0,average_response_time=0,average_waiting_time=0;
            for (i = 0; i < 4; i++) {
                average_turnaround += Total_processes[i]->turnaround_time;
                average_waiting_time += Total_processes[i]->waiting_time;
                average_response_time += Total_processes[i]->response_time;
            }
            average_turnaround /= 4;
            average_waiting_time /= 4;
            average_response_time /= 4;
            fprintf(result, "\nAverage Waiting time=%lf\tAverage Turn-around time = %lf\tAverage Response-time=%lf\n",average_waiting_time,average_turnaround, average_response_time);
            fclose(result);
            return;
        }
    }
}


void newProcess(){
    msgbuff message;
    process *p;
    while(1){
        if(msgrcv(msgqid, &message, BUFF_SIZE, 0, IPC_NOWAIT) <= 0){
            return;
        }


        if(message.mtype == 101){
            int pid = atoi(strtok(message.mtext, " "));
            printf("New process <%d> arrived in the ready queue.\n", pid);
            Total_processes[total_number_process++]                 = (process *)malloc(sizeof(process));
            Total_processes[total_number_process-1]->pid            = pid;
            Total_processes[total_number_process-1]->Priority       = atoi(strtok(NULL," "));
            Total_processes[total_number_process-1]->scheduled      = false;
            Total_processes[total_number_process-1]->start          = clock();
            Total_processes[total_number_process-1]->waiting        = true;
            Total_processes[total_number_process-1]->last_wait      = Total_processes[total_number_process-1]->start;
            Total_processes[total_number_process-1]->waiting_time   = 0;
            p = Total_processes[total_number_process-1];
        }
        else if(message.mtype == 102){
            int pid = atoi(strtok(message.mtext, " "));
            printf("Process <%d> completes I/O.\n", pid);
            int i;
            for(i=0; i<total_number_process; i++){
                if(Total_processes[i]->pid == pid){
                    Total_processes[i]->waiting = true;
                    Total_processes[i]->last_wait = clock();
                    p = Total_processes[i];
                    break;
                }
            }
        }

        if(sched_algo == RR){
            enqueue(ReadyQueue,p);
        }
        else{
            enqueueWithPriority(ReadyQueue,p);
        }
    }
}


void ioSignalHand(int signum){
    isIORequest = true;
}
void terSignalHand(int signum){
    isTerminate = true;
}
void exiting(int signum){
    if(msgctl(msgqid, IPC_RMID, NULL) == -1){
        perror("\nError in removing the message queue\n");
    }

    printf("\nFinish\n");
    exit(0);
}
