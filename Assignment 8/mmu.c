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
int PGH(int);

typedef struct free_frame{
      int process_pid;
      int allocated;
}free_frame;

typedef struct page_table{
      int frame_number;
      int valid;
      int timestamp;
}page_table;

typedef struct msgbuff
{
   long mtype;       		/* message type, must be > 0 */
   char mtext[BUFF_SIZE];    /* message data */
}msgbuff;

msgbuff message;

// FILE *hi;

int total_number_of_processes, physical_address_space, virtual_address_space;
int mmu_sched_queue, mmu_process_queue, page_table_id, free_frame_id, process_to_pageno;
int global_timestamp = 1;

int main(int argc, char* argv[], char* env[])
{
	// hi = fopen("mmu.txt","w");
	// int fd = fileno(hi);
	// dup2(fd,1);

      total_number_of_processes = atoi(argv[6]);
      physical_address_space = atoi(argv[7]);
      virtual_address_space = atoi(argv[8]);

      mmu_sched_queue = msgget(ftok("/tmp",atoi(argv[1])), IPC_CREAT | 0666);
      mmu_process_queue = msgget(ftok("/tmp",atoi(argv[2])), IPC_CREAT | 0666);

      // printf("MMU   mmu_sched_queue = %d   && mmu_process_queue = %d\n",mmu_sched_queue,mmu_process_queue);

      page_table_id = shmget(ftok("/tmp",atoi(argv[3])), 12*virtual_address_space*total_number_of_processes, IPC_CREAT | 0666);
      free_frame_id = shmget(ftok("/tmp",atoi(argv[4])), 8*physical_address_space, IPC_CREAT | 0666);
      process_to_pageno = shmget(ftok("/tmp",atoi(argv[5])), 4*total_number_of_processes, IPC_CREAT | 0666);

      // printf("MMU 		page_table_id = %d   && free_frame_id = %d  &&  process_to_pageno = %d\n",page_table_id,free_frame_id,process_to_pageno);

      while(1){
      		// printf("Waiting on mmu_process_queue\n");
            int err = msgrcv(mmu_process_queue, &message, BUFF_SIZE, 0, 0);
            // printf("Finished on mmu_process_queue\n");
            int process_id = message.mtype-1;
            int page_number = atoi(message.mtext);
            printf("MMU 		Requested page no %d from process_id %d\n",page_number,process_id);

            if(page_number == -9){
                  message.mtype = 2;
                  msgsnd(mmu_sched_queue,&message,strlen(message.mtext)+1,0);
                  continue;
            }

            page_table* page_tables = (page_table*)shmat(page_table_id,NULL,0) + virtual_address_space*(process_id);
            if(page_tables[page_number].valid == 1)
            {
                  page_tables[page_number].timestamp = global_timestamp;
                  global_timestamp++;
                  sprintf(message.mtext,"%d",page_tables[page_number].frame_number);
                  message.mtype = 101;
                  msgsnd(mmu_process_queue,&message,strlen(message.mtext)+1,0);

                  printf("MMU 		Global Ordering - (%d, %d, %d)\n",global_timestamp, process_id, page_number );
            }
            else
            {

                  int* process_to_pageno_array = (int *)shmat(process_to_pageno,NULL,0);

                  if(process_to_pageno_array[process_id] > page_number)
                  {
                        printf("MMU 		Page Fault Sequence - (%d, %d)\n",page_number,process_id );
                        sprintf(message.mtext,"-1");
                        message.mtype = 101;
                        msgsnd(mmu_process_queue,&message,strlen(message.mtext)+1,0);

                        int frame_received = PGH(process_id);

                        if(frame_received == -1){
                              int i;
                              long long time_stamp = 1000000000;
                              int entry;
                              for (i = 0; i < process_to_pageno_array[process_id]; i++) 
                              {
                                    if(page_tables[i].valid == 1 && page_tables[i].timestamp < time_stamp){
                                          entry = i;
                                          time_stamp = page_tables[i].timestamp;
                                    }
                              }
                              page_tables[entry].valid = 0;
                              frame_received = page_tables[entry].frame_number;
                        }

                        page_tables[page_number].valid = 1;
                        page_tables[page_number].frame_number = frame_received;
                        page_tables[page_number].timestamp = global_timestamp;
                        global_timestamp++;
                        message.mtype = 1;
                        int err = msgsnd(mmu_sched_queue,&message,strlen(message.mtext)+1,0);
                        printf("MMU 		Global Ordering 032- (%d, %d, %d)\n",global_timestamp, process_id, page_number );
                  }
                  else{
                        printf("MMU 		Invalid page reference - (%d, %d)\n",page_number,process_id );
                        sprintf(message.mtext,"-2");
                        message.mtype = 101;
                        msgsnd(mmu_process_queue,&message,strlen(message.mtext)+1,0);
                  }
            }
      }

}



int PGH(int process_id)
{
      free_frame* free_frames = (free_frame *)shmat(free_frame_id,NULL,0);
      int i;
      for ( i = 0; i < physical_address_space; i++) {
            if(free_frames[i].process_pid == process_id && free_frames[i].allocated == -1){
            	shmdt((void *)free_frames);
                return i;
            }
      }
      shmdt((void *)free_frames);
      return -1;
}
