// 	Assignment 1(a) - dls.c
//  Name- Rahul Sonanis
//  Roll No- 13CS10049
//  Name- Vishwas Jain
//  Roll No- 13CS10053

// Libraries required
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

int array[1000]; //Assuming the array size is no more than 1000.
int count = 0;   //Variable used to count the intergers read from the file.

bool isInteger(char *);
//Assuming all numbers are distinct in the array.
int main(int argc, char* argv[]){
    FILE* file;
    int status1, status2, mainpid, number, left, right, firstChildPid, secondChildPid;

    //Checking if the provided arguments are in the correct format.
    if(argc != 3 || !isInteger(argv[2])){
        printf( "usage: %s filename search_no\n", argv[0]);
        exit(0);
    }
    
    file = fopen(argv[1],"r");

    if(file == NULL){
        printf( "Error in opening the file\n");
        exit(0); 
    }
   
    while(fscanf(file, "%d", &array[count]) == 1  && count < 1000){
        count++;
    }

    //Checking if the array size is not exceeded
    if(count >= 1000){
        printf("Error: File size too large to be processed\n");
        exit(0);
    }
    
    mainpid = getpid();
    number = atoi(argv[2]); // number to be searched
    left = 0;
    right = count-1;
    
    while( right -left +1 > 5){ // enter the loop if array size is greater than 5
        firstChildPid = fork();  // create first child process with first half of the array
        if(firstChildPid == 0){
            right = left + (right- left)/2;
        }
        else{
            secondChildPid = fork(); // create second child process with second half of the array
            if(secondChildPid == 0){
                left = left + (right- left)/2 + 1;
            }
            else{
                waitpid(firstChildPid,&status1,0);   //wait for first child process and getting its status.
                waitpid(secondChildPid,&status2,0);  //wait for second child process and getting its status.
                //retrieving status using WEXITSTATUS
                status1 = WEXITSTATUS(status1);      
                status2 = WEXITSTATUS(status2);
                if(mainpid == getpid()) // This fragment of code will only be executed in the main process.
                {
                    if(status1 == status2)
                        printf("NUMBER %d NOT FOUND\n", number);
                    else
                        if(status1)
                        {
                            printf("The number %d is found at index %d\n",number,status1-1);
                        }
                        else
                        {
                            printf("The number %d is found at index %d\n",number,status2-1);
                        }
                }   

                exit(status1 | status2);
            }
        }
    }
  
    //IF array size if less than 5 then  then it searches in the segment by itself
    while(left <= right){
        if(array[left] == number){
            exit(left+1);   //returning the index found using exit.
        }
        left++;
    }

    exit(0);

    return 0;
}

// Used to check whether a given character is a numerical digit 
bool isdigitmy(char c){
    if(c <= '9' && c >= '0')
        return true;
    else return false;
}

// Used to check whether a given string is a integer 
bool isInteger(char *str){
    if (!*str)
      return false;

    if(*str == '-')  ++str;     // checking for negative number

   // Check for non-digit chars in the rest of the stirng.
   while (*str)
   {
      if (!isdigitmy(*str))
         return false;
      else
         ++str;
   }

   return true;
}
