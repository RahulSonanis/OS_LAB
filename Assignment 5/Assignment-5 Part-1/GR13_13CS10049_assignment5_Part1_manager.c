/*  Group -13
	Vishwas Jain	13CS10053
	Rahul Sonanis	13CS10049
	Assignment		5 Part-2
	Manager Code
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

void exiting(int signum);
bool checkDeadlock();
bool isCyclic();
bool isCyclicUtil(int, int);
void grab(int);
void release(int);
struct sembuf sop;
typedef struct msgbuff {
    long mtype;               /* message mtype, must be > 0 */
    char mtext[BUFF_SIZE];    /* message data */
}msgbuff;


msgbuff message;
char matrixFileName[] = "matrix.txt";
FILE* matrix;
int resource1, resource2, main_semaphore, process_pid;
int process_pid_array[10];



//First 5 are producers, next 5 are consumers and last 2 are resources
int adj[12][12];
int color[12];
int starting, ending;
int par[12];


/*
0 -> process i has not requested for queue j or released queue j
1 -> process i has requested for queue j
2 -> process i has acquired lock for queue j
*/

/*
      argv[1] -> case 1  or 2
      argv[2] -> prob
*/
int main(int agc, char* argv[], char* env[]){
      int i;
      if(agc != 3){
            printf("\nUsage - ./manager CASE Probability\nCASE -> 1/2\n\n");
            exit(0);
      }

      if(strcmp(argv[1],"1") && strcmp(argv[1],"2")){
            printf("\nUsage - ./manager CASE Probability\nCASE -> 1/2\n\n");
            exit(0);
      }

      srand(time(NULL));

      printf("\n\tCase - %s and probability = %s\n",argv[1], argv[2]);

      signal(SIGINT, exiting);


      matrix = fopen(matrixFileName,"w");

      for ( i = 0; i < 10; i++) {
            fprintf(matrix,"00\n");
      }
      fclose(matrix);

      resource1 = msgget(ftok("/tmp",100), IPC_CREAT | IPC_EXCL | 0666);
      resource2 = msgget(ftok("/tmp",200), IPC_CREAT | IPC_EXCL | 0666);
      process_pid = msgget(ftok("/tmp",400), IPC_CREAT | IPC_EXCL | 0666);
      main_semaphore =  semget(ftok("/tmp",300), 9 ,IPC_CREAT | IPC_EXCL | 0666);


      if(resource1 == -1 || resource2== -1 || main_semaphore==-1 || process_pid == -1){
            msgctl(resource1, IPC_RMID, NULL);
            msgctl(resource2, IPC_RMID, NULL);
            semctl(main_semaphore,0, IPC_RMID, NULL);
            msgctl(process_pid, IPC_RMID, NULL);
            perror("\nError in creating the message queue or semaphore\n");
            printf("Exiting...\n\n");
            exit(0);
      }


      //For Resource 1 mutex
      semctl(main_semaphore, 0, SETVAL, 1);
      // printf("value of semaphore 0 - %d\n", semctl(main_semaphore, 0, GETVAL, 0));
      //For Resource 1 full
      semctl(main_semaphore, 1, SETVAL, 0);
      // printf("value of semaphore 1 - %d\n", semctl(main_semaphore, 1, GETVAL, 0));
      //For Resource 1 Empty
      semctl(main_semaphore, 2, SETVAL, 10);
      // printf("value of semaphore 2 - %d\n", semctl(main_semaphore, 2, GETVAL, 0));
      //For Resource 2 mutex
      semctl(main_semaphore, 3, SETVAL, 1);
      // printf("value of semaphore 3 - %d\n", semctl(main_semaphore, 3, GETVAL, 0));
      //For Resource 2 full
      semctl(main_semaphore, 4, SETVAL, 0);
      // printf("value of semaphore 4 - %d\n", semctl(main_semaphore, 4, GETVAL, 0));
      //For resource 2 empty
      semctl(main_semaphore, 5, SETVAL, 10);
      // printf("value of semaphore 5 - %d\n", semctl(main_semaphore, 5, GETVAL, 0));
      //For matrix file mutex
      semctl(main_semaphore, 6, SETVAL, 1);
      // printf("value of semaphore 6 - %d\n", semctl(main_semaphore, 6, GETVAL, 0));
      semctl(main_semaphore, 7, SETVAL, 0);
      // printf("value of semaphore 7 - %d\n", semctl(main_semaphore, 7, GETVAL, 0));
      semctl(main_semaphore, 8, SETVAL, 0);
      // printf("value of semaphore 8 - %d\n", semctl(main_semaphore, 8, GETVAL, 0));





      for ( i = 0; i < 5; i++) {
            char argument[BUFF_SIZE];
            sprintf(argument,"./producer %d",i+1);
            if(!fork()){
                  if(execlp("xterm","xterm","-hold","-e",argument,(const char*)NULL) == -1)
                        exit(0);
            }
      }
      for ( i = 0; i < 5; i++) {
            char argument[BUFF_SIZE];
            sprintf(argument,"./consumer %d %s %s",i+1, argv[1], argv[2]);
            if(!fork()){
                  if(execlp("xterm","xterm","-hold","-e",argument,(const char*)NULL) == -1)
                        exit(0);
            }
      }

      for ( i = 0; i < 10; i++) {
            msgrcv(process_pid, &message, BUFF_SIZE, 0, 0);
            process_pid_array[i] = atoi(message.mtext);
      }


      while(1){
            sleep(2);

            if(checkDeadlock()){

                  for ( i = 0; i < 12; i++) {
                        kill(process_pid_array[i], SIGKILL);
                  }

                  FILE* result = fopen("result.txt","a+");
                  fprintf(result,"probability p = %f\tNo of Inserts = %d\tNo of Deletes = %d\n",atof(argv[2]),semctl(main_semaphore, 7, GETVAL, 0),semctl(main_semaphore, 8, GETVAL, 0));
                  fclose(result);
                  msgctl(resource1, IPC_RMID, NULL);
                  msgctl(resource2, IPC_RMID, NULL);
                  msgctl(process_pid, IPC_RMID, NULL);
                  semctl(main_semaphore,0, IPC_RMID, NULL);
                  break;
            }
      }
      exit(0);
}

bool checkDeadlock(){
      int i;
      for (i = 0; i < 12; i++) {
            int j;
            for ( j = 0; j < 12; j++) {
                  adj[i][j] = 0;
            }
      }


      //Acquring matrix file
      grab(6);
      char line[BUFF_SIZE];
      matrix = fopen(matrixFileName,"r");
      i=0;
      while(fgets(line, sizeof(line), matrix) != NULL){
            if(line[0] == '1'){
                  adj[i][10] = 1;
            }
            else if(line[0] == '2'){
                  adj[10][i] = 1;
            }

            if(line[1] == '1'){
                  adj[i][11] = 1;
            }
            else if(line[1] == '2'){
                  adj[11][i] = 1;
            }
            i++;
      }
      fclose(matrix);
      //Releasing matrix file
      release(6);


      if(isCyclic()){
            printf("\nCycle/Deadlock detected\n" );

            int consumer_holding_r1, consumer_holding_r2;

            while(starting != ending){
                  if(adj[10][ending] == 1){
                        consumer_holding_r1 = ending-4;
                  }

                  if(adj[11][ending] == 1){
                        consumer_holding_r2 = ending-4;
                  }
                  ending = par[ending];
            }
            if(adj[10][ending] == 1){
                  consumer_holding_r1 = ending-4;
            }

            if(adj[11][ending] == 1){
                  consumer_holding_r2 = ending-4;
            }
            printf("\nThe cycle detected is as follows:\n\tRresource 1 -> Consumer %d -> Resource 2 -> Consumer %d -> Resource 1\n\n",consumer_holding_r1, consumer_holding_r2);

            return true;
      }

      return false;
}


// This function is a variation of DFSUytil()
bool isCyclicUtil(int v,int p)
{

      int i;
      // Mark the current node as visited and part of recursion stack
      color[v] = 1;
      par[v]=p;
      // Recur for all the vertices adjacent to this vertex
      int j;
      for (j = 0; j < 12; j++) {
            if(adj[v][j] == 1){
                  if(color[j] == 1){
                        starting = j;
                        ending = v;
                        return true;
                  }
                  else if(color[j] == 0 && isCyclicUtil(j,v)){
                        return true;
                  }
            }
      }
      color[v] = 2;
      return false;
}


bool isCyclic(){
      int i;

      for(i = 0; i < 12; i++)
      {
            color[i] = 0;
            par[i] = -1;
      }

      // Call the recursive helper function to detect cycle in different
      // DFS trees
      for(i = 0; i < 12; i++){
            if (color[i] == 0 && isCyclicUtil(i,-1)){

                  return true;
            }
      }
      return false;
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


void exiting(int signum){
      int i;
      for ( i = 0; i < 10; i++) {
            kill(process_pid_array[i], SIGKILL);
      }

      msgctl(resource1, IPC_RMID, NULL);
      msgctl(resource2, IPC_RMID, NULL);
      msgctl(process_pid, IPC_RMID, NULL);
      semctl(main_semaphore,0, IPC_RMID, NULL);
      exit(0);
}
