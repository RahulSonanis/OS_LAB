/*  Group -13
	Vishwas Jain	13CS10053
	Rahul Sonanis	13CS10049
	Assignment		5 Part-2
	Train Code
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



void grab_self(char*);
void grab(int);
void release_self(char*);
void release_right(char*);
void grab_right(char*);
void release(int);
void update_matrix(int,int ,char);
void train_started(char*);

#define BUFF_SIZE 1024

int process_pid, main_semaphore, train_number, i, pid;
typedef struct msgbuff {
    long mtype;               /* message mtype, must be > 0 */
    char mtext[BUFF_SIZE];    /* message data */
}msgbuff;
FILE* matrix;
msgbuff message;
struct sembuf sop;

/*
 *  argv[1] -> train number
 *  argv[2] -> train direction
 */
int main(int agc, char* argv[], char* env[]){

      train_number = atoi(argv[1]);
      process_pid = msgget(ftok("/tmp",100), IPC_CREAT | 0666);
      main_semaphore =  semget(ftok("/tmp",200), 6 ,IPC_CREAT | 0666);

      if(main_semaphore ==-1 || process_pid == -1){
            semctl(main_semaphore,0, IPC_RMID, NULL);
            msgctl(process_pid, IPC_RMID, NULL);
            perror("\nError in creating the message queues\n");
            printf("\nExiting...\n\n");
            exit(0);
      }
      pid = getpid();
      sprintf(message.mtext,"%d",pid);
      message.mtype = 100;
      if(msgsnd(process_pid,&message,strlen(message.mtext)+1,0) == -1){
            perror("\nError in sending\n");
      }

      train_started(argv[2]);
      grab_self(argv[2]);
      grab_right(argv[2]);
      printf("\nTrain<%d>: Requests Junction-Lock\n", pid);
      grab(0);
      printf("\nTrain<%d>: Acquires Junction-Lock; Passing Junction\n", pid);
      sleep(2);
      release(0);
      printf("\nTrain<%d>: Releases Junction-Lock\n", pid);
      release_self(argv[2]);
      release_right(argv[2]);

      sprintf(message.mtext,"Finished");
      message.mtype = 200;
      if(msgsnd(process_pid,&message,strlen(message.mtext)+1,0) == -1){
            perror("\nError in sending\n");
      }

      exit(0);
}

void train_started(char* Direction){
      if(!strcmp(Direction,"N")){
            printf("\nTrain<%d>: North train started.\n", pid);
      }
      else if(!strcmp(Direction, "E")){
            printf("\nTrain<%d>: East train started.\n", pid);
      }
      else if(!strcmp(Direction, "W")){
            printf("\nTrain<%d>: West train started.\n", pid);
      }
      else{
            printf("\nTrain<%d>: South train started.\n",pid);
      }
}

void grab_right(char* Direction){
      if(!strcmp(Direction,"N")){
            update_matrix(train_number, 4, '1');
            printf("\nTrain<%d>: Requests for West-Lock\n", pid);
            grab(4);
            printf("\nTrain<%d>: Acquires West-Lock\n", pid);
            update_matrix(train_number, 4, '2');
      }
      else if(!strcmp(Direction, "E")){
            update_matrix(train_number, 1, '1');
            printf("\nTrain<%d>: Requests for North-Lock\n", pid);
            grab(1);
            printf("\nTrain<%d>: Acquires North-Lock\n", pid);
            update_matrix(train_number, 1, '2');
      }
      else if(!strcmp(Direction, "W")){
            update_matrix(train_number, 3, '1');
            printf("\nTrain<%d>: Requests for South-Lock\n", pid);
            grab(3);
            printf("\nTrain<%d>: Acquires South-Lock\n", pid);
            update_matrix(train_number, 3, '2');
      }
      else{
            update_matrix(train_number, 2, '1');
            printf("\nTrain<%d>: Requests for East-Lock\n", pid);
            grab(2);
            printf("\nTrain<%d>: Acquires East-Lock\n", pid);
            update_matrix(train_number, 2, '2');
      }
}

void release_right(char* Direction){
      if(!strcmp(Direction,"N")){
            release(4);
            update_matrix(train_number, 4, '0');
            printf("\nTrain<%d>: Releases West-Lock\n", pid);
      }
      else if(!strcmp(Direction, "E")){
            release(1);
            update_matrix(train_number, 1, '0');
            printf("\nTrain<%d>: Releases North-Lock\n", pid);
      }
      else if(!strcmp(Direction, "W")){
            release(3);
            update_matrix(train_number, 3, '0');
            printf("\nTrain<%d>: Releases South-Lock\n", pid);
      }
      else{
            release(2);
            update_matrix(train_number, 2, '0');
            printf("\nTrain<%d>: Releases East-Lock\n", pid);
      }
}


void grab_self(char* Direction){
      if(!strcmp(Direction,"N")){
            update_matrix(train_number, 1, '1');
            printf("\nTrain<%d>: Requests for North-Lock\n", pid);
            grab(1);
            printf("\nTrain<%d>: Acquires North-Lock\n", pid);
            update_matrix(train_number, 1, '2');
      }
      else if(!strcmp(Direction, "E")){
            update_matrix(train_number, 2, '1');
            printf("\nTrain<%d>: Requests for East-Lock\n", pid);
            grab(2);
            printf("\nTrain<%d>: Acquires East-Lock\n", pid);
            update_matrix(train_number, 2, '2');
      }
      else if(!strcmp(Direction, "W")){
            update_matrix(train_number, 4, '1');
            printf("\nTrain<%d>: Requests for West-Lock\n", pid);
            grab(4);
            printf("\nTrain<%d>: Acquires West-Lock\n", pid);
            update_matrix(train_number, 4, '2');
      }
      else{
            update_matrix(train_number, 3, '1');
            printf("\nTrain<%d>: Requests for South-Lock\n", pid);
            grab(3);
            printf("\nTrain<%d>: Acquires South-Lock\n", pid);
            update_matrix(train_number, 3, '2');
      }
}


void release_self(char* Direction){
      if(!strcmp(Direction,"N")){
            release(1);
            update_matrix(train_number, 1, '0');
            printf("\nTrain<%d>: Releases North-Lock\n", pid);
      }
      else if(!strcmp(Direction, "E")){
            release(2);
            update_matrix(train_number, 2, '0');
            printf("\nTrain<%d>: Releases East-Lock\n", pid);
      }
      else if(!strcmp(Direction, "W")){
            release(4);
            update_matrix(train_number, 4, '0');
            printf("\nTrain<%d>: Releases West-Lock\n", pid);
      }
      else{
            release(3);
            update_matrix(train_number, 3, '0');
            printf("\nTrain<%d>: Releases South-Lock\n", pid);
      }
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

void update_matrix(int row, int column, char value){
      grab(5);
      char line[BUFF_SIZE];
      char temp[BUFF_SIZE];
      matrix = fopen("matrix.txt","r");
      i=1;
      temp[0] = '\0';
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(i == row){
                  line[column - 1]=value;
            }
            strcat(temp,line);
            i++;
      }
      fclose(matrix);

      matrix = fopen("matrix.txt","w");
      fprintf(matrix, "%s", temp);
      fclose(matrix);
      release(5);
}
