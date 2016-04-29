//  Assignment 3 - Message Queue (Client Code)
//  Name- Rahul Sonanis
//  Roll No- 13CS10049
//  Name- Vishwas Jain
//  Roll No- 13CS10053

// Libraries required
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <fcntl.h>

#define BUFF_SIZE 8192

typedef struct msgbuff 
{
   long mtype;       		/* message type, must be > 0 */
   char mtext[BUFF_SIZE];    /* message data */
}msgbuff;

// id se bhejna hai
// pid se accept karna hai
char input[BUFF_SIZE];
char nextToken[200];
void getNextToken();

int main()
{
	char * arguments[100];							//Store the arguments of a program used in execvp
	int argcount;									//Argumnet Count
	int err;
	pid_t mainpid = getpid();						//Pid of the main process
	
	msgbuff message;								//Instance of the msgbuff structure to send msg to message queue.


	// accessing message queue
	int msgid = msgget(ftok("/tmp",100),IPC_CREAT | 0666);
	//Error Handling if the message queue coudn't be formed
	if(msgid == -1)
	{
		perror("Error in accessing message queue");
		exit(0);
	}

	//Pipe for syncing the cwd in both the processes.
	int fdsend[2];
	err = pipe2(fdsend,O_NONBLOCK);
	if(err == -1)
	{
		perror("Error in pipeing");
		exit(0);
	}



	int ChildPid = fork();										//Forking a child
	if(ChildPid > 0)											//Parent takes care of the execution of the commands in the terminal
	{
		bool couple = false;									//Boolean to maintain if this terminal is coupled.

		while(1)
		{
			close(fdsend[0]);									//Closing the read end of the pipe.

			char* cwd = getcwd(NULL,0);
			printf("%s> ",cwd);									//Printing the cwd on the terminal.
			
			fgets(input,sizeof(input),stdin);					// Taking input from the terminal.

			input[strlen(input)-1] = '\0';						//Removing the extra \n form the end.

			if(!strcmp(input,""))	continue;					//Handling the case for no input

			if(!strcmp(input,"couple"))							//If the terminal requests for coupling
			{
				
				message.mtype = 101;							// creating "couple" message to be sent to the server
																// Type 101 is exclusively defined for the server.
				sprintf(message.mtext,"couple<%d>",mainpid);

				
				err = msgsnd(msgid,&message,strlen(message.mtext)+1,0);		//sending "couple" message to the server

				if(err == -1)	
					perror("Error in sending couple message to the server");
				else 	
					couple = true;
				
			}
			else if(!strcmp(input,"uncouple") || !strcmp(input,"exit"))
			{
				if(couple)
				{
					message.mtype = 101;							//creating "uncouple" message to be sent to the server
					sprintf(message.mtext,"uncouple<%d>",mainpid);
					
					err = msgsnd(msgid,&message,strlen(message.mtext)+1,0);	//sending "uncouple" message to the server
					if(err == -1)
						perror("Error in sending uncouple message to the server");

					couple = false;
				}
				if(!strcmp(input,"exit")){
					kill(ChildPid,SIGKILL);
					close(fdsend[0]);
					close(fdsend[1]);
					exit(0);
				}
			}
			else
			{	
				char dupinput[BUFF_SIZE];
				strcpy(dupinput,input);
				strcat(dupinput,"\n");

				//Extracting the arguments for the program to be executed using the execvp
				char* firstToken = strtok(input," ");
				if(firstToken == NULL)	continue;
				strcpy(nextToken,firstToken);
				argcount = 0;
				while(strcmp(nextToken,""))
				{
					arguments[argcount++] = strdup(nextToken);
					getNextToken();	
				}
				arguments[argcount] = NULL;	
				//making the last argument null

				//Handling the case of change directory
				if(!strcmp(arguments[0],"cd"))
				{
					err = chdir(arguments[1]);
					strcpy(message.mtext,dupinput);
					if(err == -1)
					{
						perror("Error in changing directory");
						strcat(message.mtext,"Error in changing directory: No such file or directory");
						//exit(0);
					}


					//Passing directory change information to the other process.
					char temp1[BUFF_SIZE];
					sprintf(temp1,"%s",getcwd(NULL,0));
					write(fdsend[1],temp1,BUFF_SIZE);

					if(couple){
						char temp[20];
						sprintf(temp,"<%d>",mainpid);
						strcat(message.mtext,temp);

						// sending message to the server
						err = msgsnd(msgid,&message,strlen(message.mtext)+1,0);
						if(err == -1)
						{
							perror("Error in sending messages to the server");
							couple = false;
						}
					}
				}
				else
				{
					char filename[200];
					sprintf(filename,"%d temp.txt",mainpid);

					int exechild = fork();
					if(exechild == 0)			// executing child of sendChildProcess
					{
						FILE *fp = fopen(filename,"w");
						int fd = fileno(fp);

	                	dup2(fd,1);
	                	dup2(fd,2);

						err = execvp(arguments[0],arguments);
						if(err == -1)        // executing and error handling
			            {
			                perror("Process can't be executed!");
			                exit(0);
			            }
					}
					else				// sendChildProcess
					{
						wait(NULL);

						FILE *fp = fopen(filename,"r");

						strcpy(message.mtext,dupinput);

						char readPipe[BUFF_SIZE];
						while(fgets(readPipe,BUFF_SIZE,fp) != NULL)
						{
							strcat(message.mtext,readPipe);
						}
						fclose(fp);
						remove(filename);

						printf("%s\n",&(message.mtext[strlen(dupinput)]));
						if(couple){
							char temp[20];
							sprintf(temp,"<%d>",mainpid);
							strcat(message.mtext,temp);

							// printf("\nMessage sent to server = %s\n",message.mtext);
							
							// sending message to the server
							err = msgsnd(msgid,&message,strlen(message.mtext)+1,0);
							if(err == -1)
							{
								perror("Error in sending messages to the server");
								couple = false;
							}
						}	
					}
				}
			}
		}
	}
	else if(ChildPid == 0)
	{
		// child process (receiving)
		close(fdsend[1]);
		while(1)
		{
			message.mtext[0]='\0';
			err = msgrcv(msgid,&message,BUFF_SIZE,mainpid,0);
			if(err == -1)
			{
				perror("Error in receiving message");
				exit(0);
			}

			int r = err - 3;
			int l = err - 3;
			while(message.mtext[l] != '<')	l--;
			if(r-l <= 0){
				printf("Error in the received message!\nExiting!");
				exit(0);
			}

			char recMsgPid[10];
			strncpy(recMsgPid,&(message.mtext[l+1]),r-l);
			recMsgPid[r-l] = '\0';
			int recMsgPidInt = atoi(recMsgPid);
			message.mtext[l] = '\0';

			if(strlen(message.mtext) == 0)
			{
				char display1[BUFF_SIZE];
				sprintf(display1,"ID : %d",recMsgPidInt);
				write(1, display1, strlen(display1));

				char temp2[BUFF_SIZE];
				err = read(fdsend[0],temp2,BUFF_SIZE);
				if(err == -1)
				{
					sprintf(display1,"\n%s>",getcwd(NULL,0));
					write(1, display1,strlen(display1));
				}
				else
				{
					err = chdir(temp2);
					if(err == -1)
					{
						perror("Error in changing dir child");
						exit(0);
					}
					sprintf(display1,"\n%s>",getcwd(NULL,0));
					write(1, display1,strlen(display1));
				}
			}
			else
			{
				char display2[BUFF_SIZE];
				sprintf(display2,"\nTerminal %d:%s",recMsgPidInt,message.mtext );
				write(1, display2, strlen(display2));

				char temp2[BUFF_SIZE];
				err = read(fdsend[0],temp2,BUFF_SIZE);
				if(err == -1)
				{
					sprintf(display2,"\n%s>",getcwd(NULL,0));
					write(1, display2,strlen(display2));
				}
				else
				{
					err = chdir(temp2);
					if(err == -1)
					{
						perror("Error in changing dir child");
						exit(0);
					}
					sprintf(display2,"\n%s>",getcwd(NULL,0));
					write(1, display2,strlen(display2));
				}
			}
		}
	}
}


void getNextToken(){

	nextToken[0] = '\0';
	char* token;

	token = strtok(NULL," ");
	if(token == NULL)
		return;
	strcat(nextToken,token);

	// Handling when there is a space in file path
	while(token[strlen(token) - 1] == '\\'){

		nextToken[strlen(nextToken)-1] = ' ';
		token = strtok(NULL," ");
		if(token == NULL)
			return;
		strcat(nextToken,token);
	}
}