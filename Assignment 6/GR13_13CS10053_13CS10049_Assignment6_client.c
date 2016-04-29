/*
 *                Group - 13
 *        Vishwas Jain      13CS10053
 *        Rahul Sonais      13CS10049
 *                Assignment-6
 *                Client Code
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
#include <errno.h>


#define BUFF_SIZE 1024
#define RED     "\033[31m"      /* Red */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define ANSI_COLOR_RESET   "\x1b[0m"       /* For Font Colour Reset */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */
#define WITHDRAW  1
#define DEPOSIT   2
#define VIEW      3
#define LEAVE     4


void print_welcome_text();
bool isdigitmy(char);
bool isInteger(char *);
bool acquire_atm(int);
bool inquire_atm(int);
void interact_with_client(int);
int get_user_choice(int* );
int get_atm_id();

typedef struct msgbuff {
      long mtype;               /* message mtype, must be > 0 */
      char mtext[BUFF_SIZE];    /* message data */
}msgbuff;


struct sembuf sop;
msgbuff message;
pid_t process_id;
int   msgq_key, shem_key, main_semaphore, main_msgqueue;
char user_input[BUFF_SIZE];

int main(int argc, char* argv[], char* env[]){

      process_id = getpid();
      print_welcome_text();

      while(1){
            printf("\n> Currently you are not connected to any ATM.\n  To enter any of the ATMs, type 'ENTER ATM<" RED "id" ANSI_COLOR_RESET ">'\n\n");
            int atm_no = get_atm_id();

            if(!acquire_atm(atm_no))      continue;

            interact_with_client(atm_no);
      }
      return 1;
}


void interact_with_client(int atm_no){
      int value;

      while(1){
            int user_choice = get_user_choice(&value);
            switch(user_choice){
                  case DEPOSIT:
                        message.mtype = DEPOSIT;
                        sprintf(message.mtext,"%d",value);
                        if(msgsnd(main_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to ATM\n");
                        }
                        msgrcv(main_msgqueue, &message, BUFF_SIZE, 500, 0);
                        printf(">%s\n",message.mtext);
                        break;
                  case LEAVE:
                        message.mtype = LEAVE;
                        sprintf(message.mtext,"LEAVE");
                        if(msgsnd(main_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to ATM\n");
                        }
                        msgrcv(main_msgqueue, &message, BUFF_SIZE, 500, 0);
                        printf(">%s\n",message.mtext);
                        //Release lock
                        sop.sem_num=0;
                        sop.sem_op=1;
                        sop.sem_flg=0;
                        int rst = semop(main_semaphore, &sop, 1);
                        return;
                        break;
                  case WITHDRAW:
                        message.mtype = WITHDRAW;
                        sprintf(message.mtext,"%d",value);
                        if(msgsnd(main_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to ATM\n");
                        }
                        msgrcv(main_msgqueue, &message, BUFF_SIZE, 500, 0);
                        printf(">%s\n",message.mtext);
                        break;
                  case VIEW:
                        message.mtype = VIEW;
                        sprintf(message.mtext,"VIEW");
                        if(msgsnd(main_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                              perror("\nError in connecting to ATM\n");
                        }
                        msgrcv(main_msgqueue, &message, BUFF_SIZE, 500, 0);
                        printf(">%s\n",message.mtext);
                        break;
            }
      }
}


int get_user_choice(int* value){
      printf(BOLDWHITE "\n\tYou can perform the following operations:\n" ANSI_COLOR_RESET);
      printf(BOLDWHITE "\t\tWITHDRAW <"  RED "amount" BOLDWHITE ">\n\t\tDEPOSIT <" RED "amount" BOLDWHITE ">\n\t\tVIEW\n\t\tLEAVE\n\n" ANSI_COLOR_RESET);

      while(1){
            printf(BOLDBLUE    "Client: "    ANSI_COLOR_RESET);

            fgets(user_input,BUFF_SIZE,stdin);

            char* token = strtok(user_input," ");
            if(token == NULL){
                  printf("> Error in user input, try again!\n");
                  continue;
            }
            else if(!strcasecmp(token,"withdraw")){
                  token = strtok(NULL,"\n");
                  if(!isInteger(token)){
                        printf("> Error in user input, try again!\n");
                        continue;
                  }
                  *value = atoi(token);
                  if(*value <= 0){
                        printf("> Incorrect amount value, try again!\n");
                        continue;
                  }
                  return WITHDRAW;
            }
            else if(!strcasecmp(token,"deposit")){
                  token = strtok(NULL,"\n");
                  if(!isInteger(token)){
                        printf("> Error in user input, try again!\n");
                        continue;
                  }
                  *value = atoi(token);
                  if(*value <= 0){
                        printf("> Incorrect amount value, try again!\n");
                        continue;
                  }
                  return DEPOSIT;
            }
            else if(!strcasecmp(token,"view\n")){
                  return VIEW;
            }
            else if(!strcasecmp(token,"leave\n")){
                  return LEAVE;
            }
            else{
                  printf("> Error in user input, try again!\n");
            }
      }
}

bool acquire_atm(int atm_no){
      printf("Trying to connect to ATM%d...\n", atm_no);
      if(inquire_atm(atm_no)){
            main_semaphore =  semget(ftok("/tmp",shem_key), 1 ,IPC_CREAT | 0666); // main semaphore
            //Checking empty for queue1
            sop.sem_num=0;
            sop.sem_op=-1;
            sop.sem_flg=IPC_NOWAIT;
            int rst = semop(main_semaphore, &sop, 1);

            if(rst == -1 && errno == EAGAIN){
                  printf("> ATM%d is already occupied!\n",atm_no);
                  return false;
            }
            main_msgqueue = msgget(ftok("/tmp",msgq_key), IPC_CREAT | 0666);  // master to ATM processes
            message.mtype = 5;
            sprintf(message.mtext,"%d",process_id);
            if(msgsnd(main_msgqueue,&message,strlen(message.mtext)+1,0) == -1){
                  perror("\nError in connecting to ATM\n");
            }

            msgrcv(main_msgqueue, &message, BUFF_SIZE, 500, 0);
            printf(">%s\n",message.mtext);
            return true;
      }
      else{
            printf("> Invalid ATM id!\n");
            return false;
      }
}


bool inquire_atm(int atm_no){
      char file_input[BUFF_SIZE];
      FILE* file_locator = fopen("atm_locator.txt","r");
      if(file_locator == NULL)      return false;
      int counter = 0;

      while(fgets(file_input,BUFF_SIZE,file_locator) != NULL){
            counter++;
            if(counter == atm_no){
                  strtok(file_input," ");
                  msgq_key = atoi(strtok(NULL," "));
                  shem_key = atoi(strtok(NULL," "));
                  return true;
            }
      }
      fclose(file_locator);
      return false;
}


int get_atm_id(){
      char* token;
      while(1){
            printf(BOLDBLUE    "Client: "    ANSI_COLOR_RESET);
            fgets(user_input,BUFF_SIZE,stdin);

            token = strtok(user_input," ");
            if(token == NULL || strcasecmp(token,"ENTER")){
                  printf("> Error in user input, try again!\n");
                  continue;
            }

            token = strdup(strtok(NULL,"\n"));
            if(token == NULL || (token[0] != 'A' && token[0] != 'a') || (token[1] != 'T' && token[1] != 't') || (token[2] != 'M' && token[2] != 'm')){
                  printf("> Error in user input, try again!\n");
                  continue;
            }

            if(!isInteger(token+3)){
                  printf("> Error in user input, try again!\n");
                  continue;
            }

            return atoi(token+3);
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

void print_welcome_text(){
      printf("\n\t----------------------------");
      printf("\n\t\tWelcome client\n\tYour Account Number is %d\n",process_id);
      printf("\t----------------------------\n");
}
