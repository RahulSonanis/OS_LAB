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

void initialise_ipc();
void create_scheduler();
void create_mmu();
void create_processes();
void exiting();

typedef struct free_frames{
      int process_pid;
      int allocated;
}free_frames;

typedef struct page_table{
      int frame_number;
      int valid;
      int timestamp;
}page_table;

int page_table_id, free_frame_id, ready_queue, mmu_process_queue, mmu_sched_queue;
int total_number_of_processes, virtual_address_space, physical_address_space,process_to_pageno;
pid_t sched_pid, mmu_pid;
int total_pages_required;
int *process_to_pageno_array;
int sched_pid,mmu_pid;



int main(int argc, char* argv[], char* env[]){

      signal(SIGINT,exiting);
      srand(time(NULL));

      if(argc != 4){
            printf("Usage: ./master total_number_of_processes virtual_address_space physical_address_space\n");
            printf("Exiting...\n\n");
            exit(0);
      }

      total_number_of_processes = atoi(argv[1]);
      virtual_address_space = atoi(argv[2]);
      physical_address_space = atoi(argv[3]);

      initialise_ipc();

      int i,number_of_pages;

      total_pages_required = 0;
      process_to_pageno_array = (int *)shmat(process_to_pageno,NULL,0);
      for(i = 0; i < total_number_of_processes; i++) {
            number_of_pages = (rand()%virtual_address_space) +1;
            process_to_pageno_array[i] = number_of_pages;
            printf("MASTER 		allocated %d pages to process %d\n",process_to_pageno_array[i],i);
            total_pages_required += number_of_pages;
      }

      printf("MASTER 	Total pages required = %d\n",total_pages_required);

      // printf("MASTER   mmu_sched_queue = %d   &&  mmu_process_queue = %d  &&  ready_queue = %d\n",mmu_sched_queue,mmu_process_queue,ready_queue);      
      // printf("MASTER  page_table_id = %d   &&  free_frame_id = %d  &&  process_to_pageno = %d\n",page_table_id,free_frame_id,process_to_pageno);


      create_scheduler();
      create_mmu();
      create_processes();

      shmdt((int *)process_to_pageno_array);
      waitpid(sched_pid,NULL,0);
      kill(mmu_pid,SIGKILL);
      exiting();
}


void create_scheduler(){
       sched_pid = fork();
       char total_processes[100];
       sprintf(total_processes,"%d",total_number_of_processes);
      if(sched_pid < 0){
            exiting();
      }
      else if(sched_pid == 0){//child
            if(execlp("./scheduler","./scheduler","90","110",total_processes,(const char*)NULL) == -1){
                  exit(0);
            }

      }
}

void create_mmu(){
      mmu_pid = fork();
      char total_process[100];
      char physical_space[100];
      char virtual_space[100];
      sprintf(total_process,"%d",total_number_of_processes);
      sprintf(physical_space,"%d",physical_address_space);
      sprintf(virtual_space,"%d",virtual_address_space);

      if(mmu_pid < 0){
            exiting();
      }
      else if(mmu_pid == 0){//child
            // char argument[5000];
            // sprintf(argument,"./mmu 110 100 90 100 110 %d %d %d",total_number_of_processes,physical_address_space,virtual_address_space);
            // if(execlp("gnome-terminal","gnome-terminal","-e",argument,(const char*)NULL) == -1)
            //     exit(0);
            if(execlp("./mmu","./mmu","110","100","90","100","110",total_process,physical_space,virtual_space,(const char*)NULL) == -1)
                  exit(0);
      }
}

void create_processes(){
      int i,j;

      char reference_string[100000];
      for ( i = 0; i < total_number_of_processes; i++) {
            int length_of_string = (rand()%(8*process_to_pageno_array[i]+1)) + 2*process_to_pageno_array[i];
            reference_string[0] = '\0';
            for ( j = 0; j < length_of_string; j++) {
                  int max = (3*process_to_pageno_array[i])/2;
                  // int rand_page = (rand()%(max)) + 1;
                  int rand_page = (rand()%(max));
                  char temp[100];
                  sprintf(temp,"%d,",rand_page);
                  strcat(reference_string,temp);
            }

            
            int frames = (process_to_pageno_array[i]*physical_address_space)/total_pages_required;
            int count = 0;

            free_frames* free_frame = (free_frames *)shmat(free_frame_id,NULL,0);
            for ( j = 0; j < physical_address_space; j++) {
                  if(free_frame[j].process_pid == -1){
                        free_frame[j].process_pid = i;
                        count++;
                        if(count == frames)     break;
                  }
            }

            shmdt((free_frames*)free_frame);

            int process_pid1 = fork();

            if(process_pid1 < 0){
                  exiting();
            }
            else if(process_pid1 == 0){//child
                  char temp[100];
                  sprintf(temp,"%d",i);
                  if(execlp("./process","./process","90","100",temp,reference_string,(const char*)NULL) == -1)
                        exit(0);
            }
            usleep(250000);
      }
}


void initialise_ipc(){
      int i;
      page_table_id = shmget(ftok("/tmp",90), 12*virtual_address_space*total_number_of_processes, IPC_CREAT | IPC_EXCL | 0666);
      if(page_table_id == -1){
            perror("Error in creating the shared memory: ");
            printf("Exiting...\n\n");
            exit(0);
      }
      page_table* page_tables = (page_table *)shmat(page_table_id,NULL,0);
      for(i = 0; i < virtual_address_space*total_number_of_processes; i++) {
            page_tables[i].valid = 0;
      }
      shmdt((void *)page_tables);


      free_frame_id = shmget(ftok("/tmp",100), 8*physical_address_space, IPC_CREAT | IPC_EXCL | 0666);
      if(free_frame_id == -1){
            shmctl(page_table_id, IPC_RMID, 0);
            perror("Error in creating the shared memory: ");
            printf("Exiting...\n\n");
            exit(0);
      }
      free_frames* free_frame = (free_frames *)shmat(free_frame_id,NULL,0);
      for ( i = 0; i < physical_address_space; i++) {
            free_frame[i].allocated = -1;
            free_frame[i].process_pid = -1;
      }
      shmdt((void *)free_frame);


      process_to_pageno = shmget(ftok("/tmp",110), 4*total_number_of_processes, IPC_CREAT | IPC_EXCL | 0666);
      if(process_to_pageno == -1){
            shmctl(page_table_id, IPC_RMID, 0);
            shmctl(free_frame_id, IPC_RMID, 0);
            perror("Error in creating the shared memory: ");
            printf("Exiting...\n\n");
            exit(0);
      }

      ready_queue = msgget(ftok("/tmp",90), IPC_CREAT | IPC_EXCL | 0666);
      mmu_process_queue = msgget(ftok("/tmp",100), IPC_CREAT | IPC_EXCL | 0666);
      mmu_sched_queue = msgget(ftok("/tmp",110), IPC_CREAT | IPC_EXCL | 0666);
      if(ready_queue == -1 ||  mmu_sched_queue == -1 || mmu_process_queue == -1 ){
            shmctl(page_table_id, IPC_RMID, 0);
            shmctl(free_frame_id, IPC_RMID, 0);
            shmctl(process_to_pageno, IPC_RMID, 0);
            msgctl(ready_queue, IPC_RMID, NULL);
            msgctl(mmu_process_queue, IPC_RMID, NULL);
            msgctl(mmu_sched_queue, IPC_RMID, NULL);
            perror("Error in creating message queue: ");
            printf("Exiting...\n\n");
            exit(0);
      }
}


void exiting()
{
      shmctl(page_table_id, IPC_RMID, 0);
      shmctl(free_frame_id, IPC_RMID, 0);
      shmctl(process_to_pageno, IPC_RMID, 0);
      msgctl(ready_queue, IPC_RMID, NULL);
      msgctl(mmu_process_queue, IPC_RMID, NULL);
      msgctl(mmu_sched_queue, IPC_RMID, NULL);
      printf("Exiting...\n\n");
      exit(0);
}
