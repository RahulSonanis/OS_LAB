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

typedef struct msgbuff {
      long mtype;               /* message mtype, must be > 0 */
      char mtext[BUFF_SIZE];    /* message data */
}msgbuff;


void exiting(int);
bool check_deadlock();
bool isCyclic();
bool isCyclicUtil(int,int);
void grab();
void release();


struct sembuf sop;
msgbuff message;
FILE* sequence, *matrix;
char train_sequence[BUFF_SIZE], ch;
int c, total_trains, i, current_train, process_pid, main_semaphore, total_verices, no_exit_process;
int process_pid_array[BUFF_SIZE];
float prob, p;
int **adj;
int* color, starting, ending, *par;
bool dead_lock;

int main(int argc, char* argv[], char* env[]){


      printf("\nEnter the probility p = " );
      scanf("%f",&p);
      printf("\n" );
      signal(SIGINT, exiting);

      srand(time(NULL));


      sequence = fopen("sequence.txt","r");

      if(sequence == NULL){
            printf("\nNo file names sequence.txt\n");
            exit(0);
      }
      train_sequence[0] = '\0';
      while((c = fgetc(sequence)) != EOF){
            ch = (char)c;
            if(ch == 'N' || ch == 'E' || ch == 'W' || ch == 'S')
            {
                  strcat(train_sequence, &ch);
            }
            else
            break;
      }
      fclose(sequence);

      process_pid = msgget(ftok("/tmp",100), IPC_CREAT | IPC_EXCL | 0666);
      main_semaphore =  semget(ftok("/tmp",200), 6 ,IPC_CREAT | IPC_EXCL | 0666);

      if(main_semaphore==-1 || process_pid == -1){
            semctl(main_semaphore,0, IPC_RMID, NULL);
            msgctl(process_pid, IPC_RMID, NULL);
            perror("\nError in creating the message queues");
            printf("Exiting...\n\n");
            exit(0);
      }



      semctl(main_semaphore, 0, SETVAL, 1);
      printf("value of semaphore 0 - %d\n", semctl(main_semaphore, 0, GETVAL, 0));
      semctl(main_semaphore, 1, SETVAL, 1);
      printf("value of semaphore 1 - %d\n", semctl(main_semaphore, 1, GETVAL, 0));
      semctl(main_semaphore, 2, SETVAL, 1);
      printf("value of semaphore 2 - %d\n", semctl(main_semaphore, 2, GETVAL, 0));
      semctl(main_semaphore, 3, SETVAL, 1);
      printf("value of semaphore 3 - %d\n", semctl(main_semaphore, 3, GETVAL, 0));
      semctl(main_semaphore, 4, SETVAL, 1);
      printf("value of semaphore 4 - %d\n", semctl(main_semaphore, 4, GETVAL, 0));
      semctl(main_semaphore, 5, SETVAL, 1);
      printf("value of semaphore 5 - %d\n", semctl(main_semaphore, 5, GETVAL, 0));


      total_trains = strlen(train_sequence);

      total_verices = total_trains + 4;
      adj = (int **)malloc(total_verices*sizeof(int *));
      for ( i = 0; i < total_verices; i++) {
            adj[i] = (int *)malloc(total_verices*sizeof(int));
      }
      color = (int *)malloc(total_verices*sizeof(int));
      par = (int *)malloc(total_verices*sizeof(int));

      matrix = fopen("matrix.txt","w");
      for ( i = 0; i < total_trains; i++) {
            fprintf(matrix, "0000\n");
      }
      fclose(matrix);


      dead_lock = false;
      current_train = 0;
      printf("\n\t..........\n");
      no_exit_process = 0;
      while(1){

            if(current_train >= total_trains){
                  if(msgrcv(process_pid, &message, BUFF_SIZE, 200, IPC_NOWAIT) != -1)
                        no_exit_process++;
                  if(no_exit_process == total_trains){
                        printf("\n\nAll trains successfully passed the junction\n" );
                        free(color);
                        free(par);
                        for ( i = 0; i < total_verices; i++) {
                              free(adj[i]);
                        }
                        free(adj);

                        msgctl(process_pid, IPC_RMID, NULL);
                        semctl(main_semaphore,0, IPC_RMID, NULL);
                        exit(0);
                  }
                  dead_lock = check_deadlock();
                  sleep(1);
            }
            else{
                  prob = (float)(rand()%101)/100;
                  if(prob <= p){
                        //create new train process
                        char argument[3][100];
                        sprintf(argument[0],"./train");
                        sprintf(argument[1],"%d",current_train+1);
                        sprintf(argument[2],"%c",train_sequence[current_train]);
                        if(!fork()){
                              if(execlp(argument[0],argument[0],argument[1],argument[2],(const char*)NULL) == -1)
                              exit(0);
                        }
                        msgrcv(process_pid, &message, BUFF_SIZE, 100, 0);
                        process_pid_array[current_train] = atoi(message.mtext);
                        // printf("\npid found = %d\n", process_pid_array[current_train]);
                        current_train++;
                  }
                  else{
                        dead_lock = check_deadlock();
                  }
            }
            if(dead_lock)     break;
      }

      printf("\n\t..........\n\tSystem Deadlocked\n");

      int train_from_north, train_from_east, train_from_west, train_from_south;

      while(starting != ending){
            if(ending <= total_trains-1){
                  if(train_sequence[ending] == 'N'){
                        train_from_north = process_pid_array[ending];
                  }
                  else if(train_sequence[ending] == 'E'){
                        train_from_east = process_pid_array[ending];
                  }
                  else if(train_sequence[ending] == 'W'){
                        train_from_west = process_pid_array[ending];
                  }
                  else if(train_sequence[ending] == 'S'){
                        train_from_south = process_pid_array[ending];
                  }
            }
            ending = par[ending];
      }
      if(ending <= total_trains-1){
            if(train_sequence[ending] == 'N'){
                  train_from_north = process_pid_array[ending];
            }
            else if(train_sequence[ending] == 'E'){
                  train_from_east = process_pid_array[ending];
            }
            else if(train_sequence[ending] == 'W'){
                  train_from_west = process_pid_array[ending];
            }
            else if(train_sequence[ending] == 'S'){
                  train_from_south = process_pid_array[ending];
            }
      }

      for (i = 0; i < current_train; i++) {
            // printf("\nself = %d pid = %d\n", getpid(), process_pid_array[i] );
            kill(process_pid_array[i], SIGKILL);
      }

      // kill(train_from_west, SIGKILL);
      // kill(train_from_north, SIGKILL);
      // kill(train_from_south, SIGKILL);

      printf("\n\t  Train<%d> from North is waiting for Train<%d> from West  ----\n", train_from_north, train_from_west);
      printf("--------->Train<%d> from West  is Waiting for Train<%d> from South ----\n",train_from_west, train_from_south );
      printf("--------->Train<%d> from South is Waiting for Train<%d> from East  ----\n",train_from_south, train_from_east);
      printf("--------->Train<%d> from East  is Waiting for Train<%d> from North \n\n",train_from_east, train_from_north );

      free(color);
      free(par);
      for ( i = 0; i < total_verices; i++) {
            free(adj[i]);
      }
      free(adj);

      msgctl(process_pid, IPC_RMID, NULL);
      semctl(main_semaphore,0, IPC_RMID, NULL);
      exit(0);
}

bool check_deadlock(){
      for (i = 0; i < total_verices; i++) {
            int j;
            for ( j = 0; j < total_verices; j++) {
                  adj[i][j] = 0;
            }
      }

      // printf("\n Before seg fault\n");
      grab(5);
      char line[BUFF_SIZE];
      matrix = fopen("matrix.txt","r");
      i=0;
      while(fgets(line, sizeof(line), matrix) != NULL){

            if(line[0] == '1'){
                  adj[i][total_verices-4] = 1;
            }
            else if(line[0] == '2'){
                  adj[total_verices-4][i] = 1;
            }

            if(line[1] == '1'){
                  adj[i][total_verices-3] = 1;
            }
            else if(line[1] == '2'){
                  adj[total_verices-3][i] = 1;
            }

            if(line[2] == '1'){
                  adj[i][total_verices-2] = 1;
            }
            else if(line[2] == '2'){
                  adj[total_verices-2][i] = 1;
            }

            if(line[3] == '1'){
                  adj[i][total_verices-1] = 1;
            }
            else if(line[3] == '2'){
                  adj[total_verices-1][i] = 1;
            }
            i++;
      }
      fclose(matrix);
      release(5);
      // printf("\n After seg fault\n");
      return isCyclic();
}

// This function is a variation of DFSUytil()
bool isCyclicUtil(int v,int p)
{
      // Mark the current node as visited and part of recursion stack
      color[v] = 1;
      par[v]=p;
      // Recur for all the vertices adjacent to this vertex
      int j;
      for (j = 0; j < total_verices; j++) {
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
      color[v] =2;
      return false;
}


bool isCyclic(){
      for(i = 0; i < total_verices; i++)
      {
            color[i] = 0;
            par[i] = -1;
      }

      // Call the recursive helper function to detect cycle in different
      // DFS trees
      for(i = 0; i < total_verices; i++){
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
      msgctl(process_pid, IPC_RMID, NULL);
      semctl(main_semaphore,0, IPC_RMID, NULL);
      exit(0);
}
