/*  Group -13
	Vishwas Jain	13CS10053
	Rahul Sonanis	13CS10049
	Assignment		5 Part-1
	Consumer Code
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
#include <time.h>


#define BUFF_SIZE 1024


void release_queue_1();
void release_queue_2();
void for_queue_1();
void for_queue_2();
void checkfull_queue_1();
void checkfull_queue_2();
void grab(int);
void release(int);
void update_matrix(int, int, char);

typedef struct msgbuff {
    long mtype;               /* message mtype, must be > 0 */
    char mtext[BUFF_SIZE];    /* message data */
}msgbuff;

float prob;
float p;
struct sembuf sop;
msgbuff message;



char matrixFileName[] = "matrix.txt";
FILE* matrix;
int resource1, resource2, i, main_semaphore, consumer_id, process_pid;


/*
      argv[1] = consumer number
      argv[2] = case 1 or 2
      argv[3] = probability
*/

int main(int agc, char* argv[], char* env[]){
      srand(time(NULL));

      printf("\n\t Consumer No - %s Case - %s and probability = %s\n",argv[1], argv[2], argv[3]);


      consumer_id = atoi(argv[1]);

      resource1 = msgget(ftok("/tmp",100), IPC_CREAT  | 0666);
      resource2 = msgget(ftok("/tmp",200), IPC_CREAT  | 0666);
      process_pid = msgget(ftok("/tmp",400), IPC_CREAT | 0666);

      sprintf(message.mtext,"%d",getpid());
      message.mtype = 1;
      if(msgsnd(process_pid,&message,strlen(message.mtext)+1,0) == -1){
            perror("\nError in inserting to the queue 2 from producer");
      }

      main_semaphore =  semget(ftok("/tmp",300), 9 ,IPC_CREAT | 0666);

      p = atof(argv[3]);

      while(1){
            prob = (float)(rand()%101)/100;

            if(prob >= p){
                  int random_queue = rand()%2;

                  if(random_queue == 0){

                        update_matrix(consumer_id+5, 0, '1');
                        //Checking full for queue1
                        grab(1);
                        //Acquring the queue 1
                        grab(0);

                        update_matrix(consumer_id+5, 0, '2');


                        msgrcv(resource1, &message, BUFF_SIZE, 0, 0);
                        printf("\nValue %s Consumed from Resource 1\n",message.mtext);
                        //Increasing the number of reads
                        release(8);
                        //Increasing the number of empty slots
                        release(2);
                        //Releasing the quque 1
                        release(0);
                        update_matrix(consumer_id+5, 0, '0');

                  }
                  else{
                        update_matrix(consumer_id+5, 1, '1');
                        //Checking full for queue2
                        grab(4);
                        //Acquring the queue 2
                        grab(3);
                        update_matrix(consumer_id+5,1 , '2');
                        msgrcv(resource2, &message, BUFF_SIZE, 0, 0);
                        printf("\nValue %s Consumed from Resource 2\n",message.mtext);
                        //Increasing the number of reads
                        release(8);
                        //Increasing the number of empty slots
                        release(5);
                        //Releasing queue 2
                        release(3);
                        update_matrix(consumer_id+5, 1, '0');
                  }
            }
            else{
                  if(!strcmp(argv[2],"2")){
                        update_matrix(consumer_id+5, 0, '1');
                        //Checking full for queue1
                        grab(1);
                        update_matrix(consumer_id+5, 1, '1');
                        //Checking full for queue2
                        grab(4);
                        //Acquring the queue 1
                        grab(0);

                        update_matrix(consumer_id+5, 0, '2');


                        msgrcv(resource1, &message, BUFF_SIZE, 0, 0);
                        printf("\nValue %s Consumed from Resource 1\n",message.mtext);
                        //Increasing the number of reads
                        release(8);
                        //Increasing the number of empty slots
                        release(2);
                        //Acquring the queue 2
                        grab(3);
                        update_matrix(consumer_id+5,1 , '2');
                        msgrcv(resource2, &message, BUFF_SIZE, 0, 0);
                        printf("\nValue %s Consumed from Resource 2\n",message.mtext);
                        //Increasing the number of reads
                        release(8);
                        //Increasing the number of empty slots
                        release(5);
                        //Releasing the queue 1
                        release(0);
                        update_matrix(consumer_id+5, 0, '0');
                        //Releasing the queue 2
                        release(3);
                        update_matrix(consumer_id+5, 1, '0');
                  }
                  else if(!strcmp(argv[2],"1")){
                        int random_queue = rand()%2;
                        if(random_queue == 0){
                              update_matrix(consumer_id+5, 0, '1');
                              //Checking full for queue1
                              grab(1);
                              update_matrix(consumer_id+5, 1, '1');
                              //Checking full for queue2
                              grab(4);
                              //Acquring the queue 1
                              grab(0);

                              update_matrix(consumer_id+5, 0, '2');


                              msgrcv(resource1, &message, BUFF_SIZE, 0, 0);
                              printf("\nValue %s Consumed from Resource 1\n",message.mtext);
                              //Increasing the number of reads
                              release(8);
                              //Increasing the number of empty slots
                              release(2);
                              //Acquring the queue 2
                              grab(3);
                              update_matrix(consumer_id+5,1 , '2');
                              msgrcv(resource2, &message, BUFF_SIZE, 0, 0);
                              printf("\nValue %s Consumed from Resource 2\n",message.mtext);
                              //Increasing the number of reads
                              release(8);
                              //Increasing the number of empty slots
                              release(5);
                              release(0);
                              update_matrix(consumer_id+5, 0, '0');
                              release(3);
                              update_matrix(consumer_id+5, 1, '0');
                        }
                        else{
                              update_matrix(consumer_id+5, 1, '1');
                              //Checking full for queue2
                              grab(4);
                              update_matrix(consumer_id+5, 0, '1');
                              //Checking full for queue1
                              grab(1);
                              //Acquring the queue 2
                              grab(3);
                              update_matrix(consumer_id+5,1 , '2');
                              msgrcv(resource2, &message, BUFF_SIZE, 0, 0);
                              printf("\nValue %s Consumed from Resource 2\n",message.mtext);
                              //Increasing the number of reads
                              release(8);
                              //Increasing the number of empty slots
                              release(5);
                              //Acquring the queue 1
                              grab(0);

                              update_matrix(consumer_id+5, 0, '2');


                              msgrcv(resource1, &message, BUFF_SIZE, 0, 0);
                              printf("\nValue %s Consumed from Resource 1\n",message.mtext);
                              //Increasing the number of reads
                              release(8);
                              //Increasing the number of empty slots
                              release(2);
                              release(3);
                              update_matrix(consumer_id+5, 1, '0');
                              release(0);
                              update_matrix(consumer_id+5, 0, '0');
                        }
                  }

            }
      }
}

void update_matrix(int row, int column, char value){
      //Acquring matrix file
      grab(6);
      char line[BUFF_SIZE];
      char temp[BUFF_SIZE];
      matrix = fopen(matrixFileName,"r");
      i=1;
      temp[0] = '\0';
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(i == row){
                  line[column]=value;
            }
            strcat(temp,line);
            i++;
      }
      fclose(matrix);
      matrix = fopen(matrixFileName,"w");
      fprintf(matrix, "%s", temp);
      fclose(matrix);
      //Releasing matrix file
      release(6);
}

void grab(int sem){
      sop.sem_num=sem;
      sop.sem_op=-1;
      sop.sem_flg=0;
      semop(main_semaphore, &sop, 1);
}

void release(int sem){
      sop.sem_num=sem;
      sop.sem_op=1;
      sop.sem_flg=0;
      semop(main_semaphore,&sop,1);
}
