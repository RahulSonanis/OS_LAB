#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <time.h>

#define BUFF_SIZE 1024

typedef struct msgbuff
{
   long mtype;       		/* message type, must be > 0 */
   char mtext[BUFF_SIZE];    /* message data */
}msgbuff;

msgbuff message_mmu, message_ready;
int ready_queue, mmu_sched_queue;
int process_terminated = 0;
int total_number_of_processes;


int main(int argc, char* argv[], char* env[]){
      total_number_of_processes = atoi(argv[3]);

      ready_queue = msgget(ftok("/tmp",atoi(argv[1])), IPC_CREAT | 0666);
      mmu_sched_queue = msgget(ftok("/tmp",atoi(argv[2])), IPC_CREAT | 0666);
      if(ready_queue == -1 || mmu_sched_queue == -1){
            msgctl(ready_queue, IPC_RMID, NULL);
            msgctl(mmu_sched_queue, IPC_RMID, NULL);
            perror("Error in creating message queue: ");
            printf("Exiting...\n\n");
            exit(0);
      }

      // printf("SCHEDULER   ready_queue = %d   && mmu_sched_queue = %d\n",ready_queue,mmu_sched_queue);

      while(1){
      		printf("SCHEDULER 		selecting process to schedule\n");
            msgrcv(ready_queue, &message_ready, BUFF_SIZE, 0, 0);
            int curr_process_id = message_ready.mtype;
            kill(curr_process_id,SIGUSR1);

            printf("SCHEDULER 		PROCESS %ld for scheduling\n",message_ready.mtype);

            msgrcv(mmu_sched_queue, &message_mmu, BUFF_SIZE, 0, 0);
            printf("SCHEDULER 	   Received type %ld message from mmu\n",message_mmu.mtype);
            if(message_mmu.mtype == 1){
                  message_ready.mtype =curr_process_id;
                  msgsnd(ready_queue,&message_ready,strlen(message_ready.mtext)+1,0);
                  continue;
            }
            else{
                  process_terminated++;
                  if(total_number_of_processes == process_terminated){
                        printf("SCHEDULER 		terminating\n");
                        exit(1);
                  }
            }
      }
}
