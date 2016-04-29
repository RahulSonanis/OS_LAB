/*
 *                Group - 13
 *        Vishwas Jain      13CS10053
 *        Rahul Sonais      13CS10049
 *                Assignment-6
 *                Master Code
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

#define BUFF_SIZE 1024
#define WITHDRAW  1
#define DEPOSIT   2
#define VIEW      3
#define LEAVE     4

int no_of_ATM_processes,main_msgqueue,main_semaphore,global_memid,i;

typedef struct account_entry{
	int account_no;
	int balance;
	time_t up_time;
}account_entry;


typedef struct account_entry_list{
	account_entry info[50];
}account_entry_list;


account_entry_list * ptr;
int total_users = 0;


typedef struct msgbuff {
    long mtype;               /* message mtype, must be > 0 */
    char mtext[BUFF_SIZE];    /* message data */
}msgbuff;

msgbuff message;


typedef struct transaction{
      time_t time_stamp;
      int account_no;
      int value;
      short type;
}transaction;

transaction *other_transactions;
account_entry *other_accounts;
long long *other_no_of_transactions;
long long *other_no_of_clients;


void exiting(int signum);

/*
	DOUBT ========================>>>>>>>>>>>>>>>>>   What happens when local and global consistency happens at the same time?

*/

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		printf("Usage: ./master No_of_ATM_processes\n");
		exit(0);
	}

	signal(SIGINT, exiting);

	no_of_ATM_processes = atoi(argv[1]);

	main_msgqueue = msgget(ftok("/tmp",90), IPC_CREAT | IPC_EXCL | 0666);  // master to ATM processes
	global_memid = shmget(ftok("/tmp",90), BUFF_SIZE, IPC_CREAT | IPC_EXCL | 0666);

	ptr = (account_entry_list *)shmat(global_memid,NULL,0);

	if(main_msgqueue == -1 || global_memid == -1)
	{
	      msgctl(main_msgqueue, IPC_RMID, NULL);
	      shmctl(global_memid, IPC_RMID, 0);
	      perror("\nError in creating the message queue or semaphore\n");
	      printf("Exiting...\n\n");
	      exit(0);
	}


	FILE *atm_locator;
	atm_locator = fopen("atm_locator.txt","w");

	// ATM ID x ; msgqueue(k) for client to ATM ; semaphore(k) ; key of shared memory
	for (i = 0; i < no_of_ATM_processes; ++i)
	{
		fprintf(atm_locator,"%d %d %d %d\n",i+1,(i+1)*100,(i+1)*100,(i+1)*100);
		main_semaphore =  semget(ftok("/tmp",(i+1)*100),1,IPC_CREAT | IPC_EXCL | 0666);
		semctl(main_semaphore, 0, SETVAL, 1);
	}
	fclose(atm_locator);


	for(i = 0; i < no_of_ATM_processes; ++i)
	{
		char argument[BUFF_SIZE];
		sprintf(argument,"./atm %d",i+1);
		if(!fork())
		{
		    // if(execlp("xterm","xterm","-hold","-e",argument,(const char*)NULL) == -1)
		    if(execlp("gnome-terminal","gnome-terminal","-e",argument,(const char*)NULL) == -1)
		        exit(0);
		}
	}

	while(1)
	{
		msgrcv(main_msgqueue, &message, BUFF_SIZE, 0, 0);

		if(message.mtype == 101)	// ENTER
		{
			char *token = strtok(message.mtext," ");
			int client_id = atoi(token);
			token = strtok(NULL," ");
			int atm_id = atoi(token);

			for (i = 0; i < total_users; ++i)
			{
				if(ptr->info[i].account_no == client_id)
				{
					message.mtype = atm_id;
					sprintf(message.mtext,"User Already Exists");
					if(msgsnd(main_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
					      perror("\nError in sending from master to atm");
					}
					printf("ATM%d: Client %d entered\n",atm_id,client_id);
					break;
				}
			}
			if(i == total_users)
			{
				ptr->info[i].account_no = client_id;
				ptr->info[i].balance = 0;
				time(&ptr->info[i].up_time);
				total_users++;

				message.mtype = atm_id;
				sprintf(message.mtext,"New User Created");
				if(msgsnd(main_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
				      perror("\nError in sending from master to atm");
				}
				printf("ATM%d: Client %d entered\n",atm_id,client_id);
			}

		}
		else if(message.mtype == 102)						// VIEW
		{
			time_t latest_timestamp;
			int user_id_mem;
			int new_balance;
			double diff_t;

			char *token = strtok(message.mtext," ");
			int client_id = atoi(token);
			token = strtok(NULL," ");
			int atm_id = atoi(token);

			printf("ATM%d: Running a global consistency check for a/c %d\n",atm_id,client_id);

			for (i = 0; i < total_users; ++i)
			{
				if(ptr->info[i].account_no == client_id)
				{
					latest_timestamp = ptr->info[i].up_time;
					user_id_mem = i;
					new_balance = ptr->info[i].balance;
					break;
				}
			}


			int other_shared_memid;

			FILE *file_locator = fopen("atm_locator.txt","r");
			int counter = 0;

			char file_input[BUFF_SIZE];

			while(fgets(file_input,BUFF_SIZE,file_locator) != NULL){

			    counter++;
				strtok(file_input," ");
				strtok(NULL," ");
				strtok(NULL," ");
				int temp_shm_key = atoi(strtok(NULL," "));
				// printf("KEY = %d\n",temp_shm_key);
				int other_shared_memid = shmget(ftok("/tmp",temp_shm_key), 5120, IPC_CREAT | 0666);

				other_transactions = (transaction *)shmat(other_shared_memid,NULL,0);
				other_accounts = (account_entry *)(other_transactions + 180);
				other_no_of_transactions = (long long*)(other_accounts + 49);
				other_no_of_clients = (long long *)(other_no_of_transactions + 1);

				// printf("other_no_of_clients = %lld\n",*other_no_of_clients );
				// printf("other_no_of_transactions = %lld\n",*other_no_of_transactions );
				for (i = 0; i < *other_no_of_transactions; i++)
				{
				    if(other_transactions[i].account_no == client_id)
				    {
				          diff_t = difftime(other_transactions[i].time_stamp, latest_timestamp);
				          if(diff_t >= 0){
				                if(other_transactions[i].type == DEPOSIT){
				                      new_balance += other_transactions[i].value;
				                }
				                else{
				                      new_balance -= other_transactions[i].value;
				                }
				          }
				    }
				}
				if(shmdt((transaction *)other_transactions) == -1){
				    printf("\nError in detaching the shared memory\n");
				}

			}
			fclose(file_locator);

			ptr->info[user_id_mem].balance = new_balance;
			time(&ptr->info[user_id_mem].up_time);

			file_locator = fopen("atm_locator.txt","r");
			counter = 0;

			while(fgets(file_input,BUFF_SIZE,file_locator) != NULL){

			    counter++;

			    if(counter == atm_id){
			    	strtok(file_input," ");
			    	strtok(NULL," ");
			    	strtok(NULL," ");
			    	int temp_shm_key = atoi(strtok(NULL," "));
			    	int other_shared_memid = shmget(ftok("/tmp",temp_shm_key), 5120, IPC_CREAT | 0666);

			    	other_transactions = (transaction *)shmat(other_shared_memid,NULL,0);
			    	other_accounts = (account_entry *)(other_transactions + 180);
			    	other_no_of_transactions = (long long*)(other_accounts + 49);
			    	other_no_of_clients = (long long *)(other_no_of_transactions + 1);

			    	for (i = 0; i < *other_no_of_clients; i++)
			    	{
			    	    if(other_accounts[i].account_no == client_id)
			    	    {
			    	    	other_accounts[i].balance = new_balance;
			    	    	time(&other_accounts[i].up_time);
			    	    	break;
			    	    }
			    	}
			    	if(i == *other_no_of_clients)
			    	{
			    		other_accounts[i].account_no = client_id;
			    		other_accounts[i].balance = new_balance;
			    	    time(&other_accounts[i].up_time);
			    	    (*other_no_of_clients)++;
			    	}

			    	transaction temp_transactions[180];
			    	int temp_no= *other_no_of_transactions;

			    	*other_no_of_transactions = 0;

			    	for(i = 0; i < temp_no; i++)
			    	{
			    	    if(other_transactions[i].account_no != client_id)
			    	    {
			    	    	temp_transactions[i] = other_transactions[i];
			    	        (*other_no_of_transactions)++;
			    	    }
			    	}

			    	for(i = 0; i < *other_no_of_transactions; i++)
			    	{
			    	    other_transactions[i] = temp_transactions[i];
			    	}
			    	if(shmdt((transaction *)other_transactions) == -1){
			    	    printf("\nError in detaching the shared memory\n");
			    	}
			    	break;
			    }
			}
			fclose(file_locator);

			message.mtype = atm_id;
			sprintf(message.mtext,"Done");
			if(msgsnd(main_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
			      perror("\nError in sending from master to atm");
			}
			printf("Master: Done\n");

		}
		else			// LEAVE
		{
			char *token = strtok(message.mtext," ");
			int client_id = atoi(token);
			token = strtok(NULL," ");
			int atm_id = atoi(token);

			printf("ATM%d: Client %d left\n",atm_id,client_id);
		}
	}
}


void exiting(int signum){

    shmdt((account_entry *)ptr);
	msgctl(main_msgqueue, IPC_RMID, NULL);
	for (i = 0; i < no_of_ATM_processes; ++i)
	{
		main_semaphore =  semget(ftok("/tmp",(i+1)*100),1,IPC_CREAT | 0666);
		semctl(main_semaphore,0, IPC_RMID, NULL);
	}
	shmctl(global_memid, IPC_RMID, 0);
    exit(0);
}
