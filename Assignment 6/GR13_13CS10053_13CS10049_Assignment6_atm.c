/*
 *                Group - 13
 *        Vishwas Jain      13CS10053
 *        Rahul Sonais      13CS10049
 *                Assignment-6
 *                ATM Code
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
#include <sys/shm.h>
#include <time.h>

#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#define BUFF_SIZE 1024
#define WITHDRAW  1
#define DEPOSIT   2
#define VIEW      3
#define LEAVE     4


void get_keys();
void atm_exit(int);
void wait_for_client();
void handle_client_requests();
bool local_consistency_check(int);
void print_transactions();


typedef struct msgbuff {
      long mtype;               /* message mtype, must be > 0 */
      char mtext[BUFF_SIZE];    /* message data */
}msgbuff;

typedef struct transaction{
      time_t time_stamp;
      int account_no;
      int value;
      short type;
}transaction;

typedef struct account_entry{
      int account_no;
      int balance;
      time_t time_stamp;
}account_entry;



msgbuff message;
char file_input[BUFF_SIZE];
int atm_no, master_msgqueue, msgq_key, shm_key, client_msgqueue, local_shared_memid;
transaction* transactions, *other_transactions;
account_entry* accounts, *other_accounts;
int curren_client_pid;
long long* no_of_transactions, *other_no_of_transactions;
long long* no_of_clients, *other_no_of_clients;

/*
* argv[1] -> atm_id
*/
int main(int argc, char* argv[])
{

      no_of_transactions = 0;
      no_of_clients = 0;
      signal(SIGINT, atm_exit);

      atm_no = atoi(argv[1]);
      printf(BOLDGREEN "\n\t\tATM%d Started\n\n" RESET,atm_no);
      get_keys();

      master_msgqueue = msgget(ftok("/tmp",90), IPC_CREAT | 0666);  // master to ATM processes
      client_msgqueue = msgget(ftok("/tmp",msgq_key), IPC_CREAT | IPC_EXCL | 0666);  // clinet to ATM processes
      //First 4320(180*24) bytes for transactions and rest(16*49 = 784) bytes for the account_entry
      local_shared_memid = shmget(ftok("/tmp",shm_key), 5120, IPC_CREAT | IPC_EXCL | 0666);

      if(master_msgqueue == -1|| client_msgqueue == -1 || local_shared_memid == -1)
      {
            if(local_shared_memid == -1){
                  printf("local_shared_memid\n" );
            }
            if(master_msgqueue == -1){
                  printf("master_msgqueue\n");
            }
            if(client_msgqueue == -1){
                  printf("client_msgqueue");
            }
            msgctl(master_msgqueue, IPC_RMID, NULL);
            msgctl(client_msgqueue, IPC_RMID, NULL);
            shmctl(local_shared_memid, IPC_RMID, 0);
            perror("\nError in creating the message queue or shared memory\n");
            printf("Exiting...\n\n");
            exit(0);
      }


      transactions = (transaction *)shmat(local_shared_memid,NULL,0);
      accounts = (account_entry *)(transactions + 180);
      no_of_transactions = (long long*)(accounts + 49);
      no_of_clients = (long long *)(no_of_transactions + 1);

      while(1){
            printf(BOLDBLUE "ATM%d: ",atm_no);
            printf(RESET "Waiting for some client to enter the ATM.\n");
            wait_for_client();
            printf(BOLDBLUE "ATM%d: ",atm_no);
            printf(RESET"Client %d entered the ATM.\n",curren_client_pid );
            handle_client_requests();
      }

      return 0;
}

void handle_client_requests(){
      while(1){
            msgrcv(client_msgqueue, &message, BUFF_SIZE, -5, 0);
            switch(message.mtype){

                  case DEPOSIT:
                        time(&(transactions[*no_of_transactions].time_stamp));
                        transactions[*no_of_transactions].account_no = curren_client_pid;
                        transactions[*no_of_transactions].type = DEPOSIT;
                        transactions[*no_of_transactions].value = atoi(message.mtext);
                        (*no_of_transactions)++;
                        // print_transactions();
                        printf(BOLDBLUE "ATM%d: ",atm_no);
                        printf(RESET"Client %d credited Rs.%d in the ATM.\n",curren_client_pid, atoi(message.mtext));
                        sprintf(message.mtext," Rs.%d credited into your account successfully.",atoi(message.mtext));
                        message.mtype = 500;
                        if(msgsnd(client_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to Client\n");
                        }
                        break;


                  case LEAVE:
                        message.mtype = 103;
                        sprintf(message.mtext,"%d %d",curren_client_pid,atm_no);
                        if(msgsnd(master_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to ATM\n");
                        }
                        sprintf(message.mtext," Good Bye Client %d.",curren_client_pid);
                        message.mtype = 500;
                        if(msgsnd(client_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to Client\n");
                        }
                        printf(BOLDBLUE "ATM%d: ",atm_no);
                        printf(RESET "Client %d left the ATM.\n",curren_client_pid );
                        return;
                        break;


                  case WITHDRAW:
                        printf(BOLDBLUE "ATM%d: ",atm_no);
                        printf(RESET "Running a local consistency for account no. %d\n",curren_client_pid );
                        if(local_consistency_check(atoi(message.mtext))){
                              printf(BOLDBLUE "ATM%d: ",atm_no);
                              printf(RESET "Client %d debited Rs.%d from the ATM.\n",curren_client_pid, atoi(message.mtext));
                              time(&(transactions[*no_of_transactions].time_stamp));
                              transactions[*no_of_transactions].account_no = curren_client_pid;
                              transactions[*no_of_transactions].type = WITHDRAW;
                              transactions[*no_of_transactions].value = atoi(message.mtext);
                              (*no_of_transactions)++;
                              // print_transactions();
                              sprintf(message.mtext," Rs.%d debited from your account successfully.",atoi(message.mtext));
                              message.mtype = 500;
                              if(msgsnd(client_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                                    perror("\nError in connecting to Client\n");
                              }


                        }
                        else{
                              printf(BOLDBLUE "ATM%d: ",atm_no);
                              printf(RESET"Client %d withdrawl of Rs.%d was unsuccessful.\n",curren_client_pid, atoi(message.mtext));
                              sprintf(message.mtext," Balance Insufficient");
                              message.mtype = 500;
                              if(msgsnd(client_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                                    perror("\nError in connecting to Client\n");
                              }


                        }
                        printf(BOLDBLUE "ATM%d: ",atm_no);
                        printf(RESET "Done\n");
                        break;


                  case VIEW:
                        printf(BOLDBLUE "ATM%d: ",atm_no);
                        printf(RESET "Requested Master Process for global consistency for account no. %d\n",curren_client_pid );
                        message.mtype = 102;
                        sprintf(message.mtext,"%d %d",curren_client_pid,atm_no);
                        if(msgsnd(master_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to ATM\n");
                        }
                        msgrcv(master_msgqueue, &message, BUFF_SIZE, atm_no, 0);
                        int i;
                        printf("no of clients = %lld\n",*no_of_clients );
                        for ( i = 0; i < *no_of_clients; i++) {
                              if(accounts[i].account_no == curren_client_pid){
                                    sprintf(message.mtext," Current balance of your account %d is Rs.%d",curren_client_pid,accounts[i].balance);
                                    break;
                              }
                        }
                        message.mtype = 500;
                        if(msgsnd(client_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to Client\n");
                        }
                        printf(BOLDBLUE "ATM%d: ",atm_no);
                        printf(RESET "Done\n");
                        break;
            }
      }
}

void print_transactions(){
      int i;
      for ( i = 0; i < *no_of_transactions; i++) {
            printf("transaction No - %d\n",i+1 );
            printf("Account No - %d\n",transactions[i].account_no );
            printf("Value - %d\n",transactions[i].value );
            printf("Type = %d\n",transactions[i].type );
            printf("Time - %lld\n",(long long)transactions[i].time_stamp );
            printf("---------------\n" );
      }
}



bool local_consistency_check(int asked_amount){
      time_t latest_time = 0;
      int latest_balance= 0;
      int left_balance;
      int iter;
      double diff_t;
      for (iter = 0; iter < *no_of_clients; iter++) {
            if(accounts[iter].account_no == curren_client_pid){
                  latest_time = accounts[iter].time_stamp;
                  latest_balance = accounts[iter].balance;
                  break;
            }
      }

      FILE* file_locator = fopen("atm_locator.txt","r");
      int counter = 0;

      while(fgets(file_input,BUFF_SIZE,file_locator) != NULL){
            counter++;
            if(counter != atm_no){
                  strtok(file_input," ");
                  strtok(NULL," ");
                  strtok(NULL," ");
                  int temp_shm_key = atoi(strtok(NULL," "));
                  int other_shared_memid = shmget(ftok("/tmp",temp_shm_key), 5120, IPC_CREAT | 0666);

                  other_transactions = (transaction *)shmat(other_shared_memid,NULL,0);
                  other_accounts = (account_entry *)(other_transactions + 180);
                  other_no_of_transactions = (long long*)(other_accounts + 49);
                  other_no_of_clients = (long long *)(other_no_of_transactions + 1);

                  for (iter = 0; iter < *other_no_of_clients; iter++) {
                        if(other_accounts[iter].account_no == curren_client_pid){
                              diff_t = difftime(other_accounts[iter].time_stamp, latest_time);
                              if(diff_t > 0){
                                    latest_time = other_accounts[iter].time_stamp;
                                    latest_balance = other_accounts[iter].balance;
                              }
                              break;
                        }
                  }
                  if(shmdt((transaction *)other_transactions) == -1){
                        printf("\nError in detaching the shared memory\n");
                  }
            }
      }
      fclose(file_locator);

      left_balance = latest_balance;

      for (iter = 0; iter < *no_of_transactions; iter++) {
            if(transactions[iter].account_no == curren_client_pid){
                  diff_t = difftime(transactions[iter].time_stamp, latest_time);
                  if(diff_t >= 0){
                        if(transactions[iter].type == DEPOSIT){
                              left_balance += transactions[iter].value;
                        }
                        else{
                              left_balance -= transactions[iter].value;
                        }
                  }
            }
      }


      file_locator = fopen("atm_locator.txt","r");
      counter = 0;

      while(fgets(file_input,BUFF_SIZE,file_locator) != NULL){
            counter++;
            if(counter != atm_no){
                  strtok(file_input," ");
                  strtok(NULL," ");
                  strtok(NULL," ");
                  int temp_shm_key = atoi(strtok(NULL," "));
                  int other_shared_memid = shmget(ftok("/tmp",temp_shm_key), 5120, IPC_CREAT | 0666);

                  other_transactions = (transaction *)shmat(other_shared_memid,NULL,0);
                  other_accounts = (account_entry *)(other_transactions + 180);
                  other_no_of_transactions = (long long*)(other_accounts + 49);
                  other_no_of_clients = (long long *)(other_no_of_transactions + 1);

                  for (iter = 0; iter < *other_no_of_transactions; iter++) {
                        if(other_transactions[iter].account_no == curren_client_pid){
                              diff_t = difftime(other_transactions[iter].time_stamp, latest_time);
                              if(diff_t >= 0){
                                    if(other_transactions[iter].type == DEPOSIT){
                                          left_balance += other_transactions[iter].value;
                                    }
                                    else{
                                          left_balance -= other_transactions[iter].value;
                                    }
                              }
                        }
                  }
                  if(shmdt((transaction *)other_transactions) == -1){
                        printf("\nError in detaching the shared memory\n");
                  }
            }
      }
      fclose(file_locator);



      if(left_balance >= asked_amount)    return true;
      return false;
}



void wait_for_client(){
      char reply[BUFF_SIZE];
      msgrcv(client_msgqueue, &message, BUFF_SIZE, -5, 0);
      curren_client_pid = atoi(message.mtext);
      sprintf(message.mtext,"%s %d",message.mtext,atm_no);
      message.mtype = 101; //user verify
      if(msgsnd(master_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
            perror("\nError in connecting to Master\n");
      }

      msgrcv(master_msgqueue, &message, BUFF_SIZE, atm_no, 0);

      if(!strcmp(message.mtext,"User Already Exists")){
            sprintf(message.mtext," Welcome Client %d",curren_client_pid);
      }
      else{
            sprintf(message.mtext," Account created\n  Welcome Client %d",curren_client_pid);
      }
      message.mtype = 500;
      if(msgsnd(client_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
            perror("\nError in connecting to Client\n");
      }
}


void atm_exit(int signum){
      msgctl(client_msgqueue, IPC_RMID, NULL);
      shmctl(local_shared_memid, IPC_RMID, 0);
      exit(0);
}

void get_keys(){
      FILE* file_locator = fopen("atm_locator.txt","r");
      int counter = 0;

      while(fgets(file_input,BUFF_SIZE,file_locator) != NULL){
            counter++;
            if(counter == atm_no){
                  strtok(file_input," ");
                  msgq_key = atoi(strtok(NULL," "));
                  strtok(NULL," ");
                  shm_key = atoi(strtok(NULL," "));
            }
      }
      fclose(file_locator);
}
