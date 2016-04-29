// 	Assignment 2(a) - 13CS10049_primepipe.c
//  Name- Rahul Sonanis
//  Roll No- 13CS10049
//  Name- Vishwas Jain
//  Roll No- 13CS10053

// Libraries required
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>

#define MAX_SIZE 500					// Maximum size of buffer.
#define AVAILABLE 30001					// Available Signal
#define BUSY 30002						// Busy Signal
#define _GNU_SOURCE

int k,n,primeCount=0;					// Number of child processes,primes,foundprimes
int FoundPrimes[500] = {0};				// array of found primes
bool isprime[30001];					// array of primes from 1 to  30000

void generatePrime();					// Function to generate primes from 1 to 30000
void insertintoprimes(int);				// Function to insert primes in array FoundPrimes

int main(){

	generatePrime();					// generating primes from 1 to 30000

	srand((unsigned int)time(NULL));	// For rand() function to generate unique random number

	pid_t childPID[250], mainPID;		// pids of all childs and parent
	int partochild[500];				// File descriptors of parent to child pipes
	int childtopar[500];				// File descriptors of child to parent pipes
	int indexTopipe;					// Index of the corresponding pipe of the child
	int i,j;
	int err;							// Error number for error checking
	char buff[MAX_SIZE];				// Buff used for reading and writing to the pipe

	// Reading input
	printf("Enter the value of k(Number of child processes) and n = Number of Primes to be found\n");

	while(1){
		scanf("%d %d",&k,&n);

		if(k > 250 || k <=0 || n > 500 || n <=0){
			printf("Invalid input(out of scope) try again!\n");

		}
		else	break;
	}
	
	// n = 2*k;

	mainPID = getpid();				// Storing pid of the parent
	
	// Creating k childs
	for (i = 0; i < k; ++i)
	{
		if(pipe(&partochild[2*i]) == -1)				// creating pipes from parent to child
		{
			perror("Error in creating pipe!");
			exit(0);
		}

		if(pipe2(&childtopar[2*i], O_NONBLOCK) == -1)	// Creating pipes from child to parent in Non Blocking mode
		{
			perror("Error in creating pipe!");
			exit(0);
		}

		childPID[i] = fork();							// Creating child

		if(childPID[i] < 0){							// Error
			perror("Error in forking children!");
			exit(0);
		}
		else if(!childPID[i]){							// Child process
			indexTopipe = 2*i;							// Assigning pipe index in child
			break;
		}	
	}

	if(getpid() == mainPID){							// Parent Process
		while(1){
			for (i = 0; i < k; ++i)
			{
				if(read(childtopar[2*i], buff, MAX_SIZE) == -1){		// Skipping the read if pipe empty
					continue;
				}

				int status = atoi(buff);						// Reading from child

				if(status == AVAILABLE){						// Available
					for (j = 0; j < k; ++j)
					{
						sprintf(buff,"%d",rand()%30001);		// Generating random numbers to sent to child

						if(write(partochild[2*i+1], buff, MAX_SIZE) == -1){
							perror("Error in writing in pipes!");
							exit(0);
						}
					}
				}
				else if(status == BUSY){					// Busy
					//Skip this child process
				}
				else{										
					insertintoprimes(status);				// Checking if prime exists from first

					if(primeCount >= n){					// N primes found so killing all child processes and printing n primes
						for (j = 0; j < k; ++j)
						{
							if(kill(childPID[j],SIGKILL) == -1){
								perror("");
							}
						}

						printf("\nRespective %d prime numbers are as follows:\n",n);

						for (j = 0; j < n; ++j)
						{
							printf("%d. %d\n",j+1,FoundPrimes[j]);
						}
						printf("\n");
						exit(0);
					}

				}
			}
		}
	}
	else{											// Child Process
		int numbers[100];							// Input from parent
		close(partochild[indexTopipe+1]);			// Closing write end of parent to child in child process as not needed
		close(childtopar[indexTopipe]);				// Closing read end of child to parent in child process as not needed

		while(1){
			if(write(childtopar[indexTopipe+1],"30001",MAX_SIZE) == -1){	// Sending Available signal
				perror("Error in writing in pipes!");
				exit(0);
			}

			for (i = 0; i < k; ++i)											// reading k numbers from parent
			{
				if(read(partochild[indexTopipe], buff, MAX_SIZE) == -1){	
					perror("Error in reading pipes!");
					exit(0);
				}
				numbers[i] = atoi(buff);
			}

			if(write(childtopar[indexTopipe+1],"30002",MAX_SIZE) == -1){	// Sending Busy Signal
				perror("Error in writing in pipes!");
				exit(0);
			}

			for (i = 0; i < k; ++i)											// Checking for the primality of the k numbers and sending them to parent one by one
			{
				if(isprime[numbers[i]]){
				    //printf("Prime = %d child = %d\n",numbers[i],getpid());
					sprintf(buff,"%d",numbers[i]);
					if(write(childtopar[indexTopipe+1], buff, MAX_SIZE) == -1){
						perror("Error in writing in pipes!");
						exit(0);
					}
				}
			}
		}
	}

	return 0;
}

// Function to insert primes in array FoundPrimes
void insertintoprimes(int k){
	int i;
	for (i = 0; i < primeCount; ++i)
	{
		if(FoundPrimes[i] == k)
			return;
	}
	FoundPrimes[primeCount++] = k;
}

// Function to generate primes from 1 to 30000
void generatePrime(){
    int p,i;
   	isprime[0] = false;
   	isprime[1] = false;

    for(i=2 ; i <= 30000 ; i++)
    {
        isprime[i]=true;
    }

    for (p=2; p*p<=30000; p++)
    {
        if (isprime[p] == true)
        {
            for (i=p*2; i<=30000; i += p)
                isprime[i] = false;
        }
    }
}
