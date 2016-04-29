/*  Group -13
	Vishwas Jain	13CS10053
	Rahul Sonanis	13CS10049
	Assignment		5 Part-2
	Producer Code
*/

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
#include <errno.h>
#include <time.h>
#define BUFF_SIZE 1024

void release_queue_1();
void release_queue_2();
void for_queue_1();
void for_queue_2();


typedef struct msgbuff {
    long mtype;               /* message mtype, must be > 0 */
    char mtext[BUFF_SIZE];    /* message data */
}msgbuff;

msgbuff message;
struct sembuf sop;
char matrixFileName[] = "matrix.txt";
FILE* matrix;
int resource1, resource2, i, main_semaphore, producer_id, process_pid;

/*
      argv[1] = producer number
*/
int main(int agc, char* argv[], char* env[]){


      srand(time(NULL));
      printf("\n\tProducer No - %s\n",argv[1]);



      producer_id = atoi(argv[1]);
      resource1 = msgget(ftok("/tmp",100), IPC_CREAT  | 0666);
      resource2 = msgget(ftok("/tmp",200), IPC_CREAT  | 0666);
      process_pid = msgget(ftok("/tmp",400), IPC_CREAT | 0666);
      sprintf(message.mtext,"%d",getpid());
      message.mtype = 1;
      if(msgsnd(process_pid,&message,strlen(message.mtext)+1,0) == -1){
            perror("\nError in inserting to the queue 2 from producer");
      }

      main_semaphore =  semget(ftok("/tmp",300), 9 ,IPC_CREAT | 0666);


      if(resource1 == -1 || resource2== -1 || main_semaphore==-1){
            msgctl(resource1, IPC_RMID, NULL);
            msgctl(resource2, IPC_RMID, NULL);
            semctl(main_semaphore,0, IPC_RMID, NULL);
            perror("\nError in creating the message queues");
            printf("Exiting...\n\n");
            exit(0);
      }

      while(1){
            int rand_sleep = (rand()%5000) + 5000;
            usleep(rand_sleep);
            int random_queue = rand()%2;
            if(random_queue == 0){  //try to insert in queue 1

                  // printf("\nChecking empty for queue1\n" );
                  //Checking empty for queue1
                  sop.sem_num=2;
                  sop.sem_op=-1;
                  sop.sem_flg=IPC_NOWAIT;
                  int rst = semop(main_semaphore, &sop, 1);

                  if(rst == -1 && errno == EAGAIN){
                        // printf("\nNo empty slots to insert in queue 1\n");
                        continue;
                  }

                  printf("\nProducer %d Trying to insert in queue 1\n",producer_id);
                  for_queue_1();
                  release_queue_1();
                  printf("\nProducer %d successfully inserted in queue 1\n", producer_id );

            }
            else{// try to insert in queue 2
                  // printf("\nChecking empty for queue2\n" );
                  //Checking empty for queue2
                  sop.sem_num=5;
                  sop.sem_op=-1;
                  sop.sem_flg=IPC_NOWAIT;
                  int rst = semop(main_semaphore, &sop, 1);
                  if(rst == -1 && errno == EAGAIN){
                        // printf("\nNo empty slots to insert in queue 2\n");
                        continue;
                  }
                  printf("\nProducer %d Trying to insert in queue 2\n",producer_id);
                  for_queue_2();
                  release_queue_2();
                  printf("\nProducer %d successfully inserted in queue 2\n", producer_id );
            }
      }
}

void for_queue_1(){

      //Acquring matrix file
      sop.sem_num=6;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);


      char line[BUFF_SIZE];
      char temp[BUFF_SIZE];
      matrix = fopen(matrixFileName,"r");
      i=0;
      temp[0] = '\0';
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(i == producer_id-1){
                  line[0]='1';
            }
            strcat(temp,line);
            i++;
      }
      fclose(matrix);

      matrix = fopen(matrixFileName,"w");
      fprintf(matrix, "%s", temp);
      fclose(matrix);

      //Releasing matrix file
      sop.sem_num=6;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);



      //Acquring the queue 1
      sop.sem_num=0;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);


      //Acquring matrix file
      sop.sem_num=6;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);
      //

      matrix = fopen(matrixFileName,"r");
      i=0;
      temp[0] = '\0';
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(i == producer_id-1){
                  line[0]='2';
            }
            strcat(temp,line);
            i++;
      }
      fclose(matrix);

      matrix = fopen(matrixFileName,"w");
      fprintf(matrix, "%s", temp);
      fclose(matrix);


      //Releasing matrix file
      sop.sem_num=6;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);



      int number_to_insert = rand()%50 + 1;
      message.mtype = 1;
      sprintf(message.mtext,"%d",number_to_insert);
      // printf("\n message being sent = %s \n", message.mtext);
      if(msgsnd(resource1,&message,strlen(message.mtext)+1,0) == -1){
            perror("\nError in inserting to the queue 1 from producer");
      }

      //Increasing the number of inserts
      sop.sem_num=7;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);

      //Increasing the number of full slots
      sop.sem_num=1;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);
}

void for_queue_2(){

      //Acquring matrix file
      sop.sem_num=6;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);


      char line[BUFF_SIZE];
      char temp[BUFF_SIZE];
      matrix = fopen(matrixFileName,"r");
      i=0;
      temp[0] = '\0';
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(i == producer_id-1){
                  line[1]='1';
            }
            strcat(temp,line);
            i++;
      }
      fclose(matrix);

      matrix = fopen(matrixFileName,"w");
      fprintf(matrix, "%s", temp);
      fclose(matrix);

      //Releasing matrix file
      sop.sem_num=6;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);



      //Acquring the queue 2
      sop.sem_num=3;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);

      //Acquring matrix file
      sop.sem_num=6;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);



      matrix = fopen(matrixFileName,"r");
      i=0;
      temp[0] = '\0';
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(i == producer_id-1){
                  line[1]='2';
            }
            strcat(temp,line);
            i++;
      }
      fclose(matrix);

      matrix = fopen(matrixFileName,"w");
      fprintf(matrix, "%s", temp);
      fclose(matrix);

      //Releasing matrix file
      sop.sem_num=6;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);

      int number_to_insert = rand()%50 + 1;

      sprintf(message.mtext,"%d",number_to_insert);
      message.mtype = 1;
      if(msgsnd(resource2,&message,strlen(message.mtext)+1,0) == -1){
            perror("\nError in inserting to the queue 2 from producer");
      }

      //Increasing the number of inserts
      sop.sem_num=7;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);

      //Increasing the number of full slots
      sop.sem_num=4;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);

}

void release_queue_1(){


      //Releasing queue 1
      sop.sem_num=0;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);


      //Acquring matrix file
      sop.sem_num=6;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);


      char line[BUFF_SIZE];
      char temp[BUFF_SIZE];
      matrix = fopen(matrixFileName,"r");
      i=0;
      temp[0] = '\0';
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(i == producer_id-1){
                  line[0]='0';
            }
            strcat(temp,line);
            i++;
      }
      fclose(matrix);

      matrix = fopen(matrixFileName,"w");
      fprintf(matrix, "%s", temp);
      fclose(matrix);

      //Releasing matrix file
      sop.sem_num=6;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);

}

void release_queue_2(){
      //Releasing queue 2
      sop.sem_num=3;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);

      //Acquring matrix file
      sop.sem_num=6;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);


      char line[BUFF_SIZE];
      char temp[BUFF_SIZE];
      matrix = fopen(matrixFileName,"r");
      i=0;
      temp[0] = '\0';
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(i == producer_id-1){
                  line[1]='0';
            }
            strcat(temp,line);
            i++;
      }
      fclose(matrix);

      matrix = fopen(matrixFileName,"w");
      fprintf(matrix, "%s", temp);
      fclose(matrix);

      //Releasing matrix file
      sop.sem_num=6;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);
}
