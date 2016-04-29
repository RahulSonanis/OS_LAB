//  Assignment 2(b) - 13CS10049_primepipe.c
//  Name- Rahul Sonanis
//  Roll No- 13CS10049
//  Name- Vishwas Jain
//  Roll No- 13CS10053

// Libraries required
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>


#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define ANSI_COLOR_RESET   "\x1b[0m"       /* For Font Colour Reset */

//Global Variables
extern char **environ;                      // The variable "environ" points to an array of strings called the 'environment' and is used in displayEnv() function

int error;                                  // flag for error handling
char fPath[100];                            // Path of the "history.txt" file
int histCount = 0;                          // Number of total command inputs onto the shell
char nextToken[512];                        // Name of the token returned by getNextToken() function
char * arguments[64];                       // Null terminated array of "arguments" used as a parameter for function "execvp"
                                            // filled up while tokenising the input, keep in mind the functionality will change 
                                            // case of redirection using pipes.
int argcount;                               // Number of arguments read in above character array
bool background;                            // flag for remembering background process
char* OutToFile;                            // File name to which the output of a process will be returned in case of using '>'
char* inputFile;                            // File name to which the input of a process will be returned in case of using '>'
bool isFileOut;                             // flag to remember that output of the program is to be stored in a file.
bool isFileInput;                           // flag to remember that input of the program is to be read from a file.                            
jmp_buf JumpBuffer;                         // Used in set_jump and long_jump


// Function Prototypes
void clearScreen();							/* Used for implementing "clear" builtin command */
void displayEnv();							/* Used for implementing "env" builtin command */
void changeDir(char* );						/* Used for implementing "cd <dir>" builtin command */
void currentDir();							/* Used for implementing "pwd" builtin command */
void makeDir(char* );						/* Used for implementing "mkdir <dir>" builtin command */
void removeDir(char* );						/* Used for implementing "rmdir <dir>" builtin command */
void listDirContents();						/* Used for implementing "ls" builtin command */
void listDirContentsAll();					/* Used for implementing "ls -l" builtin command */
void history();								/* Used for implementing "history" builtin command */
void nhistory(int);							/* Used for implementing "history <int>" builtin command */
void exitBash();							/* Used for implementing "exit" builtin command */
void createProcess();                       /* Used for implementing the execution of the executable from the shell */
void redirectionProcess();                  /* Used for implementing the redirection using pipe. */
void getNextToken();						/* Used a intermediate function for getting the next token */
bool isdigitmy(char );						/* Used to check whether a given character is a numerical digit */
bool isInteger(char* );						/* Used to check whether a given string is a integer */
void reverseSearch(int );                   /* Signal Handler for SIGQUIT: Used to implement the reverse history search */




int main(){

    sprintf(fPath, "%s/history.txt",getcwd(NULL,0));                // creating path for "history.txt" file
    remove(fPath);                                                  // removing the file "history.txt" if it already exists
    
    signal(SIGQUIT,reverseSearch);
    while(1){
        background = false;                                         // No background process
        char* inputError;                                           // pointer variable for checking error in input
        char command[512];                                          // entered command

        sigsetjmp(JumpBuffer,SIGQUIT);
        char* cwd = getcwd(NULL,0);                                 // path of current working directory
        printf(BOLDBLUE    "%s"    ANSI_COLOR_RESET ">",cwd);       // printing that path onto shell

        inputError = fgets(command,sizeof(command),stdin);          // taking input from shell and storing it in 
        if(inputError == NULL)                                      // error  checking
        {
            printf("error");
        }
        else
        {
            if(!strcmp(command,"\n") || strlen(command)==0)   continue;                   // condition for "\n" command line input

            FILE* hist = fopen(fPath,"a+");                         // creating file "history.txt" for storing command history 
            fprintf(hist,"%s",command);
            fclose(hist);
            histCount++;

            command[strlen(command) - 1] = '\0';
            char* token;
            token = strtok(command, " ");                           // tokenizing the command line input

            // checking which command is entered and taking action accordingly by calling functions
            if(!strcmp(token,"clear"))
            {
                clearScreen();
            }  
            else if(!strcmp(token,"env"))
            {
                displayEnv();
            }
            else if(!strcmp(token,"cd"))
            {
                getNextToken();
                changeDir(nextToken);
            }
            else if(!strcmp(token,"pwd"))
            {
                currentDir();
            }
            else if(!strcmp(token,"mkdir"))
            {
                getNextToken();
                makeDir(nextToken);
            }
            else if(!strcmp(token,"rmdir"))
            {
                getNextToken();
                removeDir(nextToken);
            }
            else if(!strcmp(token,"history"))
            {
                token = strtok(NULL, " ");
                if(token != NULL)               // Cheking if there is a integer argumnet with history command
                {
                    if(isInteger(token))
                    {
                        if(atoi(token) > histCount)
                            history();
                        else nhistory(atoi(token));
                    }
                    else    printf("history: %s: numeric argument required\n", token);
                }
                else    history();
            }
            else if(!strcmp(token,"ls"))
            {
                token = strtok(NULL, " ");
                if(token != NULL)            // Cheking if there is a -l argumnet with ls command
                {
                    if(!strcmp(token,"-l"))
                    {
                        listDirContentsAll();
                    }
                    else printf("ls: %s: Command not found!\n",token);
                }
                else listDirContents();
            }
            else if(!strcmp(token,"exit"))
            {
                exitBash();
            }
            else                                                    // condition for executable commands                                           
            {
                if(token != NULL)
                {    

                    isFileOut = false;
                    isFileInput = false;
                    argcount = 1;
                    arguments[0] = strdup(token);
                    //Checking if the given commnad is to run in the background
                    //Not applicable(will not matter) in case of any redirection (pipes or files)
                    if(token[strlen(token) - 1] == '&')
                    {
                        arguments[0][strlen(arguments[0])- 1] = '\0';
                        background = true;
                    }

                    //Tokenising the input and storing the tokens in arguments string array.
                    getNextToken();
                    while(strcmp(nextToken,""))
                    {
                        arguments[argcount++] = strdup(nextToken);
                        getNextToken();
                    }
                    arguments[argcount] = NULL;
                    createProcess();
                }
            }
        }
    }
    return 0;
}



/* Used for implementing the redirection using pipe. */
void redirectionProcess(int input){
    int i,j;
    int fd[1000][2];             //Used to store the file descriptors of created pipes for transfering data
                                //between differer child processes
    int totalPrograms = 1;      //Total number of executables in the command

    if(j >= argcount){          //Error Handling
        printf("Syntax error\n");
        return;
    }

    char* redirArgs[100][100];      //Used to store all the arguments of different executables using pointers to 2D char array

    //Storing the argumnets of first execetable in redirArgs
    for(i=0 ; arguments[i] != NULL ; i++){
        redirArgs[0][i] = strdup(arguments[i]);
    }   
    redirArgs[0][i] = NULL;         //Making the last argument null
    
    int progargcount = 0;           //Keeping argumnet count o f respective executable
    bool pipelastseen = true;       //Flag to store if the last token seen was the pipe( | ) symbol


    //Checkin how many executables are there and if there is any output file to dump the data.
    for (j = input; j < argcount; ++j)
    {
        if(!strcmp(arguments[j],">")){
            if(pipelastseen){
                printf(" syntax error\n");
                return;
            }
            else{
                isFileOut = true;
                redirArgs[totalPrograms-1][progargcount] = NULL;
                j++;
                break;
            }
            
        }
        else if(!strcmp(arguments[j],"|")){
            if(pipelastseen){
                printf(" syntax error\n");
                return;
            }
            else{
                redirArgs[totalPrograms-1][progargcount] = NULL;
                pipelastseen = true;
            }
        }
        else{
            if(pipelastseen){

                totalPrograms++;
                pipelastseen = false;
                progargcount = 0;
            }
            redirArgs[totalPrograms-1][progargcount++] = strdup(arguments[j]);
        }
        
    }

    //If there is  a out File to dump the output
    if(isFileOut){

        if(j >= argcount){
            printf(" syntax error\n");
            return;
        }

        OutToFile = strdup(arguments[j++]);

        if(j < argcount){
            printf(" syntax error\n");
            return;
        }
    }

    //Creating the pipes for communication
    for(i=0 ; i < totalPrograms - 1 ; i++)
    {     
        if(pipe(fd[i]) == -1)
        {
            perror("pipe not created\n");
            return;
        }
        
    }

    //Executing the different executables in different processes froked by the this process.
    for (i = 0; i < totalPrograms; ++i)
    {
        int pid = fork();

        if(pid < 0)
        {
            perror("Error in forking");
            exit(0);
            
        }
        else if(pid == 0)
        {
            //Interconnecting all the pipes.
            if(i != 0)
            {
                dup2(fd[i-1][0], 0);
            }
            if(i != totalPrograms-1)
            {
                dup2(fd[i][1], 1);
            }

            //For taking input for the first executable from the file.
            if(i == 0 && isFileInput){
                int fd = open(inputFile,O_RDONLY,S_IRWXO|S_IRWXU|S_IRWXG);
                dup2(fd,0);
            }
            //For putting  output for the last executable into the file.
            if(i == totalPrograms-1 && isFileOut){
                int fd = open(OutToFile,O_CREAT|O_WRONLY|O_TRUNC,S_IRWXO|S_IRWXU|S_IRWXG);
                dup2(fd,1);
            }

            if(execvp(redirArgs[i][0],redirArgs[i]) == -1)        // executing and error handling
            {
                perror("Process can't be executed!");
                exit(0);
            }
        }
        else
        {
            waitpid(pid,NULL,0);        // waiting for the child process to finish.
        }
    }
}



// Used for implementing the execution of the executable from the shell 
void createProcess(){

    bool pipeFound = false;             // Falg to store if  a pipe was found
    
    int i;         

    //Checking if there is any input fiel and if there is any output file to dump the data.
    for (i = 0; i < argcount; ++i)
    {
        if(!strcmp(arguments[i],"<")){
            isFileInput = true;
            arguments[i] = NULL;
            i++;
            break;
        }
        else if(!strcmp(arguments[i],">")){
            isFileOut = true;
            arguments[i] = NULL;
            i++;
            break;
        }
        else if(!strcmp(arguments[i],"|")){
            arguments[i] = NULL;
            i++;
            pipeFound = true;
            break;
        }
    }


    if(pipeFound){
        redirectionProcess(i);
        return;
    }

    if(isFileInput){
        if(i >= argcount){
            printf(" syntax error\n");
            return;
        }
        inputFile = strdup(arguments[i++]);
        if(i < argcount){
            if(!strcmp(arguments[i],"|")){
                i++;
                pipeFound = true;
            }
            else if(!strcmp(arguments[i],">")){
                isFileOut = true;
                i++;
            }
            else{
                printf("syntax error\n");
                return;
            }
        }
        
    }

    if(pipeFound){
        redirectionProcess(i);
        return;
    }


    if(isFileOut){
        if(i >= argcount){
            printf(" syntax error\n");
            return;
        }
        OutToFile = strdup(arguments[i++]);
        if(i < argcount){
            printf(" syntax error\n");
            return;
        }
    }

    int cpid = fork();          // Creating a new child process for executing the executable

    if(cpid == 0)
    {
        if(isFileOut){
            close(1);
            int fd = open(OutToFile,O_CREAT|O_WRONLY|O_TRUNC,S_IRWXO|S_IRWXU|S_IRWXG);
            dup(fd);
        }
        if(isFileInput){
            close(0);
            int fd = open(inputFile,O_RDONLY,S_IRWXO|S_IRWXU|S_IRWXG);
            dup(fd);
        }
        
        if(execvp(arguments[0],arguments) == -1)        // executing and error handling
        {
            perror("Process can't be executed!");
            exit(0);
        }
    }
    else{
        if(!background)                                 // waiting if not a background process
        {                        
            waitpid(cpid,NULL,0);
        }
    }
}


// Used for implementing "clear" builtin command 
void clearScreen(){
    error = system("clear");
    if(error == -1){
        perror("clear");					// Error Checking
    } 
}

// Used for implementing "env" builtin command
void displayEnv(){
    char** ab = environ;					
    
    while(*ab != NULL)
        printf("%s\n",*(ab++));
}

// Used for implementing "cd <dir>" builtin command 
void changeDir(char* path){
    error = chdir(path);                    // using chdir() function call to change the 
    if(error == -1){
        perror("cd");
    } 
}

// Used for implementing "pwd" builtin command 
void currentDir(){
    char* cwd = getcwd(NULL,0);             // using getcwd() function call to get current working directory path
    
    if(cwd == NULL){
        perror("pwd");
    }
    else{
        printf("%s\n",cwd);
        free(cwd);
    }
}

// Used for implementing "mkdir <dir>" builtin command 
void makeDir(char* dirName){
    error = mkdir(dirName,ACCESSPERMS);
    if(error == -1)
        perror("mkdir");
}

// Used for implementing "rmdir <dir>" builtin command 
void removeDir(char* dirName){
    error = rmdir(dirName);
    if(error == -1) perror("rmdir");
}

// function used in listDirContents() to skip "." and  ".." directory name
int selecterer(const struct dirent *a){
    if(!strcmp(a->d_name,".") || !strcmp(a->d_name,".."))
        return 0;
    return 1;
}

// Used for implementing "ls" builtin command 
void listDirContents()
{
    struct dirent** dirEntry;                   // pointer to dirent structure for printing contents of a directory
    char* filename;
    char* path = getcwd(NULL,0);
    struct stat thestat;

    if(path == NULL)
    {
        perror("ls");
        return;
    }

    // using scandir() system call to print contents of a directory
    int count = scandir(path, &dirEntry, &selecterer, &alphasort);
    if(count == -1){
        perror("ls");
        return;
    }

    char buff[512];
    int al = 0;

    // Printing Directory Contents
    while(al != count)
    {
        filename = ((dirEntry)[al++])->d_name;
        sprintf(buff, "%s/%s", path, filename);
        stat(buff, &thestat);

        if((thestat.st_mode & S_IFMT) == S_IFDIR){
            printf(BOLDBLUE    "%s   "    ANSI_COLOR_RESET,filename);          // printing directory name in blue colour
        }
        else{
            printf("%s   ",filename);
        } 
    }
    printf("\n");
}

// Used for implementing "ls -l" builtin command 
void listDirContentsAll(){
    struct dirent** dirEntry;
    char* path = getcwd(NULL,0);
    char* filename;
    //The stat: It's how we'll retrieve the stats associated to the file. 
    struct stat thestat;
    //will be used to determine the file owner & group
    struct passwd *pd; 
    struct group *gp;

    if(path == NULL){
        perror("ls");
        return;
    }

    int count = scandir(path, &dirEntry, &selecterer, &alphasort);

    if(count == -1){
        perror("ls");
        return;
    }

    char buff[512];
    int al = 0;

    // Printing Directory Contents with all details
    while(al != count){
        filename = ((dirEntry)[al++])->d_name;
        sprintf(buff, "%s/%s", path, filename);
        stat(buff, &thestat);

        // Printing file type
        switch (thestat.st_mode & S_IFMT)
         {
            case S_IFBLK:  printf("b"); break;
            case S_IFCHR:  printf("c"); break; 
            case S_IFDIR:  printf("d"); break;  //It's a (sub)directory 
            case S_IFIFO:  printf("p"); break;  //fifo
            case S_IFLNK:  printf("l"); break;  //Sym link
            case S_IFSOCK: printf("s"); break;
            //Filetype isn't identified
            default:       printf("-"); break;
        }

        printf(" ");

        // Printing permissions
        printf( (thestat.st_mode & S_IRUSR) ? "r" : "-");
        printf( (thestat.st_mode & S_IWUSR) ? "w" : "-");
        printf( (thestat.st_mode & S_IXUSR) ? "x" : "-");
        printf( (thestat.st_mode & S_IRGRP) ? "r" : "-");
        printf( (thestat.st_mode & S_IWGRP) ? "w" : "-");
        printf( (thestat.st_mode & S_IXGRP) ? "x" : "-");
        printf( (thestat.st_mode & S_IROTH) ? "r" : "-");
        printf( (thestat.st_mode & S_IWOTH) ? "w" : "-");
        printf( (thestat.st_mode & S_IXOTH) ? "x" : "-");
        printf("  ");

        // Printing number of hard links
        printf("%d\t", (int)thestat.st_nlink);

        // Printing owner name
        pd = getpwuid(thestat.st_uid);
        printf("%s\t", pd->pw_name);

        // Printing group name
        gp = getgrgid(thestat.st_gid);
        printf("%s\t", gp->gr_name);

        // Printing size in bytes
        printf("%zu\t",thestat.st_size);

        // Printing latest modified time
        char formattedTime[13];
        strncpy(formattedTime, &ctime(&thestat.st_mtime)[4], 12);
        formattedTime[12] = '\0';
        printf("%s\t", formattedTime);

        // Printing File name
        if((thestat.st_mode & S_IFMT) == S_IFDIR){
            printf(BOLDBLUE    "%s\n"    ANSI_COLOR_RESET,filename);
        }
        else{
            printf("%s\n",filename);
        }
    }    
}

// Used for implementing "history" builtin command 
void history(){
    FILE* hist = fopen(fPath,"a+");
    char line[128];
    int line_number = 1;

    while (fgets(line, sizeof(line), hist) != NULL) 
    {
       printf("  %d\t%s",line_number++, line);
    }

    fclose(hist);
}

// Used for implementing "history <int>" builtin command 
void nhistory(int n){

    if(n < 0){
        printf("history: -1: Invalid Option\n");
        return;
    }

    int skips = 0;
    FILE* hist = fopen(fPath,"a+");
    char line[128];
    int line_number = 1;

    while (fgets(line, sizeof(line), hist) != NULL) {
        if(skips >= histCount - n)
            printf("  %d\t%s",skips+1, line);
        skips++;
    }

    fclose(hist);
}

// Used for implementing "exit" builtin command 
void exitBash(){
    exit(0);
}


// Used a intermediate function to get the path of the corresponding directory 
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


/* Signal Handler for SIGQUIT: Used to implement the reverse history search */
void reverseSearch(int signum){

    char command[512];              //To store the user input
    FILE* hist = fopen(fPath,"r");  //Opening the history.txt file in read mode
    char line[512];
    char *prevMatch = NULL;
    char* inputError;
    
    printf("\n(reverse-i-search):");

    inputError = fgets(command,sizeof(command),stdin);          // taking input from shell and storing it in command

    if(inputError == NULL)                                      // error  checking
    {
        printf("error");
    }
    else
    {
        command[strlen(command)-1]='\0';
        while (fgets(line, sizeof(line), hist) != NULL) 
        {
            if(strstr(line, command)){
                line[strlen(line)-1]='\0';                      // Using substring matching criteria for recommending previous
                                                                // commands from the history.
                prevMatch = strdup(line);
            }
        }
    }


    if(prevMatch == NULL){              //No previous matching commanding found.
        printf("Command not found!\n");
    }
    else{                               //Printing the recent matched command.
        printf("Recommended Command: %s\n",prevMatch);
    }

    fclose(hist);               //Opening the history.txt file
    siglongjmp(JumpBuffer, 1);  // Used to skip the running fgets in the main process.
}
