//  Assignment 3 - Message Queue (Server Code)
//  Name- Rahul Sonanis
//  Roll No- 13CS10049
//  Name- Vishwas Jain
//  Roll No- 13CS10053

// Libraries required
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <map>
#include <signal.h>
#define BUFF_SIZE 8192
using namespace std;


struct msgbuffer {
   long mtype;       /* message type, must be > 0 */
   char mtext[BUFF_SIZE];    /* message data */
};


bool availID[100] = {true};					//ID available for clients
struct msgbuffer msgbuff;					//Instance of the msgbuffer structure to send msg to message queue.
int msgqid, sentMsgTerID;					
map<int,int> mymap;							//Map used to store the clients
map<int,int>::iterator it;					//Iterator for the map.
void sendID(int );							//Fucntion to send the ID to the newly connected client
void exiting(int );							//Signal Handler to quit the server
void printClients();						//Printing the updated list of the clients



int main(){

	signal(SIGINT,exiting);					//Establishing the signal handler

	int i;
	for (i = 0; i < 100; ++i)
	{
		availID[i] = true;
	}

	//Creating a message queue exclusively for the server witha unique key.
	msgqid = msgget(ftok("/tmp", 100), IPC_CREAT| IPC_EXCL| 0666);

	//Error Handling if the message queue coudn't be formed
	if(msgqid == -1){
		perror("Error in creating message queue with the given key, try again!");
		msgctl(msgqid, IPC_RMID, NULL);
		exit(0);
	}


	while(1){
		//Server only receives the message with the message type = 101
		int msgSize = msgrcv(msgqid, &msgbuff, BUFF_SIZE, 101, 0);

		if(msgSize == -1){
			perror("Error in the received message!\nExiting!");
			msgctl(msgqid, IPC_RMID, NULL);
			exit(0);
		}

		//printf("msgSize = %d\n", msgSize);
		//printf("message = %s\n", msgbuff.mtext);
		//printf("lol\n");
		//Message Format for receiving  =   [Message<ClientProcessPID>]
		int r = msgSize - 3;
		int l = msgSize - 3;
		while(msgbuff.mtext[l] != '<')	l--;
		if(r-l <= 0){
			printf("Error in the received message!\nExiting!");
			msgctl(msgqid, IPC_RMID, NULL);
			exit(0);
		}

		char recMsgPid[10];
		strncpy(recMsgPid,&(msgbuff.mtext[l+1]),r-l);	//Extracting the client process pid
		recMsgPid[r-l] = '\0';
		int recMsgPidInt = atoi(recMsgPid);				//Converting the client process pid to integer
		msgbuff.mtext[l] = '\0';

		//If it is couple message i.e. new client want to join the server
		if(!strcmp(msgbuff.mtext,"couple")){
			it = mymap.find(recMsgPidInt);
			//Checking if the client is already the connected to the server
			if(it == mymap.end()){
				
				for (i = 0; i < 100; ++i)
				{
					if(availID[i])	break;
				}
				availID[i] = false;
				//Allocating new ID to the client
				mymap[recMsgPidInt] = i;
				sentMsgTerID = i;
				printf("\nNew client got connected");
			}
			//sending the ID to the client
			printClients();
			sendID(recMsgPidInt);
		}
		else{
			it = mymap.find(recMsgPidInt);
			//Verifying the client connection who has sent the message
			if(it == mymap.end()){
				printf("Server1 encountered an Error while executing!\nExiting!");
				msgctl(msgqid, IPC_RMID, NULL);
				exit(0);
			}
			//Storing the client ID
			sentMsgTerID = mymap.at(recMsgPidInt);
			//Checking if the client wants to uncouple
			if(!strcmp(msgbuff.mtext,"uncouple")){
				mymap.erase(it);
				availID[sentMsgTerID] = true;
				printf("\nA client got disconnected");
				printClients();
			}
		}


		sprintf(msgbuff.mtext,"%s<%d>",msgbuff.mtext,sentMsgTerID);
		//Sending the received message to all the other coupled terminals for mirroring.
		for(it = mymap.begin(); it != mymap.end(); it++) {
		    if(it->first != recMsgPidInt){
		    	msgbuff.mtype = it->first;
		    	if(msgsnd(msgqid, &msgbuff, strlen(msgbuff.mtext)+1, 0) == -1){
					perror("Server2 encountered an Error while executing!\nExiting!");
					msgctl(msgqid, IPC_RMID, NULL);
					exit(0);
				}
		    }
		}

	}
	return 1;
}


//Sending the ID to the newly connected client
void sendID(int terPID){
	struct msgbuffer messagebuff;
	messagebuff.mtype = terPID;
	sprintf(messagebuff.mtext, "<%d>",mymap[terPID]);
	//printf("lol = %s\n",messagebuff.mtext );
	if(msgsnd(msgqid, &messagebuff, strlen(messagebuff.mtext)+1, 0) == -1){
		msgctl(msgqid, IPC_RMID, NULL);
		perror("Server encountered an Error while executing!\nExiting!");
		exit(0);
	}
	return;
}


//Function for exiting and closing the message queue.
void exiting(int signum){
	if(msgctl(msgqid, IPC_RMID, NULL) == -1){
		perror("Server2 encountered an Error while executing!\nExiting!");		exit(0);
	}
	printf("Closing the server and removing the entry created for the message queue.\n");
	exit(0);
}

void printClients(){
	map<int,int>::iterator iter;
	printf("\n\t\t\t\tUpdated list of the clients\n");
	printf("\t\t------------------------------------------------------\n");
	printf("\t\t\tClient PID\t\tClient ID\n");
	printf("\t\t------------------------------------------------------\n");
	for(iter = mymap.begin(); iter != mymap.end(); iter++) {
	    printf("\t\t\t%d\t\t\t%d\n",iter->first, iter->second );
	}
	printf("\t\t------------------------------------------------------\n\n\n");
}