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

int ready_queue,mmu_process_queue;
char reference_string[100000];
#define BUFF_SIZE 1024

typedef struct msgbuff
{
   long mtype;       		/* message type, must be > 0 */
   char mtext[BUFF_SIZE];    /* message data */
}msgbuff;

msgbuff message;
int process_id;

void handler(int signum)
{

}

int main(int argc, char* argv[], char* env[]){

      signal(SIGUSR1,handler);

      ready_queue = msgget(ftok("/tmp",atoi(argv[1])), IPC_CREAT | 0666);
      mmu_process_queue = msgget(ftok("/tmp",atoi(argv[2])), IPC_CREAT | 0666);
      process_id = atoi(argv[3]);
      strcpy(reference_string,argv[4]);
      printf("PROCESS 		reference_string received for %d process_id %d = %s\n",getpid(),process_id,reference_string );

      message.mtype = getpid();
      msgsnd(ready_queue,&message,strlen(message.mtext)+1,0);
      pause();

      char *token;
      token = strtok(reference_string,",");

      while(token != NULL)
      {
            message.mtype = process_id+1;
            sprintf(message.mtext,"%d",atoi(token));
            printf("PROCESS 		%d requesting page %d\n",process_id,atoi(token));
            msgsnd(mmu_process_queue,&message,strlen(message.mtext)+1,0);
            msgrcv(mmu_process_queue, &message, BUFF_SIZE, 101, 0);


            int received_frame = atoi(message.mtext);
            printf("PROCESS 		%d got frame %d for page %d\n",process_id,received_frame,atoi(token));

            if(received_frame == -2)
            {
                  break;
            }
            else if(received_frame == -1)
            {
                  pause();
                  continue;
            }

            token = strtok(NULL,",");
      }

      message.mtype = process_id+1;
      strcpy(message.mtext,"-9");
      msgsnd(mmu_process_queue,&message,strlen(message.mtext)+1,0);
      printf("PROCESS 		%d terminating\n",getpid());

}
