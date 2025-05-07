#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>     
#include <unistd.h>     
#include <sys/stat.h>  
#include <time.h>      
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>


#define COMMAND_FILE_PATH "/home/debian/Desktop/SO an2/proiect/command.txt"
#define FILE "treasure_manager"
#define COMMAND "./treasure_manager"

volatile sig_atomic_t monitor_running = 1;
volatile sig_atomic_t monitor_terminating = 0;
volatile sig_atomic_t terminate = 0;
pid_t pid = -1;


int open_file_for_write(char *file)
{
  int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (fd == -1)
  {
    perror("Open for write error");
    exit(-1);
  }
  return fd;
}


int open_file_for_read(char *file)
{

  int fd = open(file, O_RDONLY);
  if (fd == -1)
  {
    perror("Open for read error");
    exit(-1);
  }
  return fd;
}

void close_file(int fd)
{
  if(close(fd)!=0)
    {
      perror("Close file error\n");
      exit(-1);
    }
}

int get_file_size(char *file)
{
  struct stat st;
  if(stat(file, &st)==-1) 
    {
      perror("Stat error\n");
      exit(-1);
    }
  
  return st.st_size;
}

void write_command(char *file, char *text)
{
  int fd=open_file_for_write(file);
  if(write(fd,text,strlen(text))==-1)
    {
      perror("Write in file error\n");
      close_file(fd);
      exit(-1);
    }
  close_file(fd);
}

void get_content_from_file(char *file, char *content) //in content vom pune continutul fisierului
{
  int fd=open_file_for_read(file);
  int f_size=get_file_size(file);

  if(read(fd,content,f_size)!=f_size)
    {
      perror("Read from file error\n");
      exit(-1);
    }

  content[f_size]='\0';
  close_file(fd);
}


void signal1_handler(int signum) //monitor handler
{
  if (signum == SIGUSR1)
    {
      char file_full_command[50]={'\0'};
      get_content_from_file(COMMAND_FILE_PATH,file_full_command);
      printf("Command read from file: %s\n", file_full_command); //verifica comanda citita 

      char commandd[20]={'\0'};
      char huntID[20]={'\0'};
      char treasureID[10]={'\0'};

      char *token=strtok(file_full_command," ");
      if(token!=NULL) strcpy(commandd,token);

      token=strtok(NULL," ");
      if(token!=NULL) strcpy(huntID,token);

      token=strtok(NULL," ");
      if(token!=NULL) strcpy(treasureID,token);

      pid_t exec_pid=fork(); //incepe child pt a executa comanda
      
      if(exec_pid<0)
	{
	  perror("Fork error\n");
	  return;
	}
      else if(exec_pid==0)
	{
	  if(strcmp(commandd,"--list_hunts")==0 && strlen(huntID)==0 && strlen(treasureID)==0) 
	    {
	      execl(FILE,COMMAND,file_full_command,NULL); //  --list_hunts
	    }
	  else if(strcmp(commandd,"--list")==0 && strlen(treasureID)==0) // --list huntID
	    {
	      execl(FILE,COMMAND,commandd,huntID,NULL);
	    }
	  else if(strcmp(commandd,"--view")==0) // --view huntID treasureID
	    {
	      execl(FILE,COMMAND,commandd,huntID,treasureID,NULL);	      
	    }
	  else
	    {
	      perror("Invalid command for execl\n");
	      exit(1);
	    }
	  perror("execl failed\n");
	  exit(1);
	}

      int status=0;
      if(waitpid(exec_pid, &status,0) ==-1) //asteptam sa se termine procesul copil (care a facut execl)
	{
	  perror("Waitpid error\n"); 
	  exit(-1);
	}

      if(!WIFEXITED(status)) //verificam daca s-a incheiat normal procesul copil
	{
	  perror("Abnormal child exit\n");
	  exit(-1);
	}

      
      pid_t p_pid=getppid();
      if(kill(p_pid,SIGUSR2)<0) //trimite semnal catre parinte
	{
	  perror("[ERROR] SIGUSRS2 not sent to parent\n");
	  exit(-1);
	    
	}
    }
  else if(signum==SIGTERM)
    {
      terminate=1;
    }  
}


void list_hunts(pid_t pid)
{
  write_command(COMMAND_FILE_PATH,"--list_hunts");
  if(kill(pid,SIGUSR1)<0) 
    {
      perror("Send SIGUSR1 to monitor error\n");
      exit(-1);
    }
}

void list_treasures(pid_t pid)
{
  printf("From which hunt? Hunt ID: ");
  char hunt_id[25]={'\0'};
  fgets(hunt_id,sizeof(hunt_id),stdin);
  hunt_id[strcspn(hunt_id, "\n")] = '\0';

  char command[50]={'\0'};
  strcpy(command,"--list");
  strcat(command," ");
  strcat(command,hunt_id);

  write_command(COMMAND_FILE_PATH,command);
  if(kill(pid,SIGUSR1)<0) 
    {
      perror("Send SIGUSR1 to monitor error\n");
      exit(-1);
    }
}

void view_treasure(pid_t pid)
{
  printf("From which hunt? Hunt ID: ");
  char hunt_id[25]={'\0'};
  fgets(hunt_id, sizeof(hunt_id),stdin);
  hunt_id[strcspn(hunt_id, "\n")] = '\0';

  printf("Which treasure? Treasure ID: ");
  int treasure_id;
  scanf("%d",&treasure_id);
  while(getchar() != '\n');

  char command[50]={'\0'};
  strcpy(command,"--view");
  strcat(command," ");
  strcat(command,hunt_id);
  strcat(command," ");
  char id_str[10];
  sprintf(id_str, "%d", treasure_id);  
  strcat(command,id_str);

  write_command(COMMAND_FILE_PATH,command);
  if(kill(pid,SIGUSR1)<0) 
    {
      perror("Send SIGUSR1 to monitor error\n");
      exit(-1);
    }
}

int monitor_loop()
{
  //setam handler-ul

  struct sigaction sa1;
  sa1.sa_handler=signal1_handler; //cand vine un semnal se apeleaza signal1_handler
  sigemptyset(&sa1.sa_mask);
  sa1.sa_flags=0;

  if(sigaction(SIGUSR1, &sa1,NULL)==-1 || sigaction(SIGTERM, &sa1,NULL)==-1) //atasam handlerul la semnalele sigusr1, sigterm
    {
      perror("Sigaction monitor error\n");
      exit(-1);
    }

  while(1) //asteapta semnale
    {
      if(!terminate) //la SIGTERM handlerul seteaza terminate=1
	{
	  sleep(1);
	}
      else
	{
	  return 0;
	}
      
    }
  
}

void monitor_exit_handler(int signum) 
{
  if(signum==SIGUSR2)
    {
      monitor_running=0;
      
    }
}

void wait_for_monitor() //asteapta monitorul inainte de urmatorul loop al meniuli
{
  //setam handler-ul
  
  struct sigaction sa2;
  sa2.sa_handler=monitor_exit_handler;
  sigemptyset(&sa2.sa_mask);
  sa2.sa_flags=0;

  if(sigaction(SIGUSR2, &sa2,NULL)==-1) //atasam handler-ul la semnalul sigusr2
    {
      perror("Sigaction parent error\n");
      exit(-1);
    }

  while(monitor_running)
    {
      sleep(1);
    }
}


void start_monitor()
{
  if (monitor_terminating)
    {
      printf("Monitor is terminating, please wait\n");
      exit(0);
    }

  if (pid > 0)
    {
      printf("Monitor is already running (PID %d)\n", pid);
      exit(0);
    }

  pid = fork();
  if (pid < 0)
    {
      perror("Fork error\n");
      exit(-1);
    }

  if (pid == 0)
    {
      monitor_loop();
      exit(0);
    }

  //printf("Monitor started in process %d\n", pid);

}

void stop_monitor()
{
  if(monitor_terminating)
    {
      printf("Monitor is terminating, please wait\n");
    }
  else if(pid>0) //daca e in parinte
    {
      printf("Monitor terminating in progress...\n");
      //sleep(5);
      kill(pid,SIGTERM);
      monitor_terminating=1;

      int status;
      if(waitpid(pid,&status,0)==-1)
	printf("Waitpid error in stop_monitor\n");
      else
	printf("Monitor has terminated\n");
      monitor_terminating=0;
      pid=-1;
    }
  else printf("Monitor is not open\n");
}


void exit_loop()
{
  if (monitor_terminating)
    {
      printf("Monitor is terminating, please wait\n");
    }
  else if (pid > 0) //  exista un proces copil activ
    {
      printf("[ERROR] Monitor is still running. Please stop it first using 'stop_monitor'\n");
    }
  else if (pid <= 0) // procesul copil nu mai exista
    {
      printf("Exited successfully\n");
      exit(0); 
    }
}


int main()
{
  //printf("\nCommand options: \n start_monitor\n list_hunts\n list_treasures\n view_treasure\n stop_monitor\n exit\n");
  char command[100]={'\0'};


  while(1)
    {
      printf("\nCommand options: \n start_monitor\n list_hunts\n list_treasures\n view_treasure\n stop_monitor\n exit\n");
      int monitor_status=0;
      int waitpid_return=-1;

      printf("Type option:\n");
      printf(">> ");

      fgets(command,sizeof(command),stdin);
      command[strcspn(command, "\n")] = 0;

      if(pid>0)
	{
	  waitpid_return=waitpid(pid,&monitor_status,WNOHANG); 
	  if(waitpid_return==-1)
	    {
	      perror("Waitpid error\n");
	      exit(-1);
	    }
	  else if(waitpid_return !=0)
	    {
	      monitor_terminating=0; 
	      pid=-1;
	    }
	}


      if(strcmp(command,"start_monitor")==0)
	  start_monitor();

      else if(strcmp(command,"list_hunts")==0)
	{
	  if(monitor_terminating)
	    {
	      printf("[ERROR] Monitor is terminating, please wait\n");
	    }
	  else if(pid>0)
	    {
	      monitor_running=1;
	      list_hunts(pid);
	      wait_for_monitor(); 
	    }
	  else printf("Monitor is not open\n");
	}

      else if(strcmp(command,"list_treasures")==0)
	{
	  if(monitor_terminating)
	    {
	      printf("[ERROR] Monitor is terminating, please wait\n");
	    }
	  else if(pid>0)
	    {
	      monitor_running=1;
	      list_treasures(pid);
	      wait_for_monitor(); 
	    }
	  else printf("Monitor is not open\n");
	}

      else if(strcmp(command,"view_treasure")==0)
	{
	  if(monitor_terminating)
	    {
	      printf("[ERROR] Monitor is terminating\n");
	    }
	  else if(pid>0)
	    {
	      monitor_running=1;
	      view_treasure(pid);
	      wait_for_monitor(); // --//--
	    }
	  else printf("Monitor is not open\n");
	}

      else if(strcmp(command,"stop_monitor")==0)
	{
	  stop_monitor();
	}

      else if(strcmp(command,"exit")==0)
	{
	  exit_loop();
	}
      else printf("Unknown command\n");
    }
  return 0;
}
