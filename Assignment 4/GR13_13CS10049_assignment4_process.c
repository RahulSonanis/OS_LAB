/*  Group -13
	Vishwas Jain	13CS10053
	Rahul Sonanis	13CS10049
	Assignment		4
	Process Code
*/


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
#include <time.h>

#define BUFFSIZE 1024
int scheduler_pid;

typedef struct proc{
	int 	priority;
	int 	no_of_iterations;
	double 	sleep_probab;
	int 	sleep_time;
}process_parameters;


typedef struct msgbuff
{
   long mtype;       								/* message type, must be > 0 */
   char mtext[BUFFSIZE];
}msgbuff;

void sigquit_handler(int,siginfo_t*, void*);		// notify signal
void sigint_handler(int );							// suspend signal

int main(int argc,char * argv[])
{
	int err,i;
	int pid = getpid();
	msgbuff message;								//Instance of the msgbuff structure to send msg to message queue.
	process_parameters proc_param;


	struct sigaction action;
	action.sa_sigaction = sigquit_handler;
	action.sa_flags = SA_SIGINFO;
	sigaction(SIGQUIT,&action,NULL);				// for catching notify signal
	signal(SIGINT,sigint_handler);					// for catching suspend signal


	// storing process information
	proc_param.priority = atoi(argv[1]);
	proc_param.no_of_iterations = atoi(argv[2]);
	proc_param.sleep_probab = atof(argv[3]);
	proc_param.sleep_time = atoi(argv[4]);

	printf("\n\tNo of iterations => \t%d\n",proc_param.priority);
	printf("\tPriority => \t%d\n",proc_param.no_of_iterations);
	printf("\tSleep Probability => \t%f\n",proc_param.sleep_probab);
	printf("\tSleep Time => \t%d\n\n\n",proc_param.sleep_time);


	// accessing message queue
	int msgqid = msgget(ftok("/tmp",100),IPC_CREAT | 0666);
	//Error Handling if the message queue coudn't be formed
	if(msgqid == -1)
	{
		perror("Error in accessing message queue");
		exit(0);
	}

	message.mtype = 101;
	sprintf(message.mtext,"%d %d",pid,proc_param.priority);

	printf("MESSAGE SENT TO SCHEDULER\n");
	err = msgsnd(msgqid,&message,strlen(message.mtext)+1,0);		//sending process parameters to scheduler
	if(err == -1)
		perror("Error in sending couple message to the server");

	pause();														// waiting for notify signal

	srand(time(NULL));

	for(i = 1 ; i <= proc_param.no_of_iterations ; i++)
	{
		printf("PID: <%d> , %d\n",pid,i);

		int ran = rand()%100 + 1;
		double prob = (double)ran/100;

		if(prob < proc_param.sleep_probab)							// rand < sleep_probability
		{
			// Send I/O signal to scheduler
			printf("Sending I/O signal to parent with process id = %d\n",scheduler_pid);
			kill(scheduler_pid,SIGUSR1);

			printf("PID: <%d> Going to I/O\n",pid);
			sleep(proc_param.sleep_time);
			printf("PID: <%d> Came back from I/O\n",pid);

			// I/O completion information sent to scheduler
			message.mtype = 102;
			sprintf(message.mtext,"%d %d",pid,proc_param.priority);

			err = msgsnd(msgqid,&message,strlen(message.mtext)+1,0);
			if(err == -1)
				perror("Error in sending couple message to the server");

			// printf("pause\n");
			pause();												// waiting for notify signal
		}
	}

	printf("Sending terminate signal to parent with process id = %d\n",scheduler_pid);

	kill(scheduler_pid,SIGUSR2);									// sending terminate signal to scheduler

	return 0;
}


// notify signal handler
void sigquit_handler(int sig, siginfo_t* info, void* vp)
{
	scheduler_pid = info->si_pid;
	// printf("resume\n");
	return;
}

// suspend signal handler
void sigint_handler(int signum)
{
	// suspend signal arrived
	// printf("pause\n");
	pause();														// waiting for notify signal
	return;
}
