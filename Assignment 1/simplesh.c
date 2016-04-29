// 	Assignment 1(b) - simplesh.c
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

#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define ANSI_COLOR_RESET   "\x1b[0m"       /* For Font Colour Reset */

extern char **environ;                      // The variable "environ" points to an array of strings called the 'environment' and is used in displayEnv() function
int error;                                  // flag for error handling

char fPath[100];                            // Path of the "history.txt" file
int histCount = 0;                          // Number of total command inputs onto the shell
char dirName[512];                          // Name of the directory returned by getDirName() function
char * arguments[64];                       // Null terminated array of "arguments" used as a parameter for function "execvp"
bool background;                            // flag for remembering background process


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

void getDirName();							/* Used a intermediate function to get the path of the corresponding directory */


bool isdigitmy(char );						/* Used to check whether a given character is a numerical digit */
bool isInteger(char* );						/* Used to check whether a given string is a integer */


// main function
int main(){

    sprintf(fPath, "%s/history.txt",getcwd(NULL,0));                // creating path for "history.txt" file
    remove(fPath);                                                  // removing the file "history.txt" if it already exists
    

    while(1){
        background = false;                                         // No background process
        char* inputError;                                           // pointer variable for checking error in input
        char command[512];                                          // entered command

        char* cwd = getcwd(NULL,0);                                 // path of current working directory
        printf(BOLDBLUE    "%s"    ANSI_COLOR_RESET ">",cwd);       // printing that path onto shell
        inputError = fgets(command,sizeof(command),stdin);          // taking input from shell and storing it in 

        if(inputError == NULL)                                      // error  checking
        {
            printf("error");
        }
        else
        {
            if(!strcmp(command,"\n"))   continue;                   // condition for "\n" command line input

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
                getDirName();
                changeDir(dirName);
            }
            else if(!strcmp(token,"pwd"))
            {
                currentDir();
            }
            else if(!strcmp(token,"mkdir"))
            {
                getDirName();
                makeDir(dirName);
            }
            else if(!strcmp(token,"rmdir"))
            {
                getDirName();
                removeDir(dirName);
            }
            else if(!strcmp(token,"history"))
            {
                token = strtok(NULL, " ");
                if(token != NULL)
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
                if(token != NULL)
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
            else                                                    // condition for executable command                                            
            {
                if(token != NULL)
                {    
                    int argcount = 1;
                    arguments[0] = strdup(token);
                    if(token[strlen(token) - 1] == '&')
                    {
                        arguments[0][strlen(arguments[0])- 1] = '\0';
                        background = true;
                    }
                    getDirName();
                    while(strcmp(dirName,""))
                    {
                        arguments[argcount++] = strdup(dirName);
                        getDirName();
                    }
                    arguments[argcount] = NULL;
                    createProcess();
                }
            }
        }
    }
    return 0;
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

// Used for implementing the execution of the executable from the shell 
void createProcess(){
    int cpid = fork();                          // Creating a new child process for executing the executable

    if(cpid == 0)
    {
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

// Used a intermediate function to get the path of the corresponding directory 
void getDirName(){
    
    dirName[0] = '\0';
    char* token;

    token = strtok(NULL," ");
    if(token == NULL)
        return;
    strcat(dirName,token);

    // Handling when there is a space in file path
    while(token[strlen(token) - 1] == '\\'){

        dirName[strlen(dirName)-1] = ' ';
        token = strtok(NULL," ");
        if(token == NULL)
            return;
        strcat(dirName,token);
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
