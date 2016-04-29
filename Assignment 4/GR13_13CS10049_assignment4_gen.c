/*  Group -13
	Vishwas Jain	13CS10053
	Rahul Sonanis	13CS10049
	Assignment		4
	Generator Code
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


int main()
{
	int mainpid = getpid();
	int cpid,i;

	for(i=0 ; i < 4 ; i++)
	{
		if(i<2)
		{
			cpid = fork();

			if(cpid == 0)
		    {
		    	int err = execlp("/usr/bin/xterm","/usr/bin/xterm","-hold","-e","./process","10","1000","0.3","1",(const char*)NULL);
				if(err == -1)
				{
					perror("Error in exec");
					exit(0);
				}
		    }
		}
		else
		{
			cpid = fork();

			if(cpid == 0)
		    {
		    	int err = execlp("/usr/bin/xterm","/usr/bin/xterm","-hold","-e","./process","5","400","0.7","3",(const char*)NULL);
				if(err == -1)
				{
					perror("Error in exec");
					exit(0);
				}
		    }
		}

		sleep(1);
	}
	return 0;
}
