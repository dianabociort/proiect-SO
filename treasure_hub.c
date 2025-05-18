
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
#include <dirent.h>


#define COMMAND_FILE_PATH "/home/debian/Desktop/SO an2/proiect/command.txt"
#define FILENAME "treasure_manager"
#define COMMAND "./treasure_manager"
#define SCORE_FILENAME "calculate_score"
#define SCORE_COMMAND "./calculate_score"

volatile sig_atomic_t monitor_running = 1;
volatile sig_atomic_t monitor_terminating = 0;
volatile sig_atomic_t terminate = 0;
pid_t pid = -1;

int pipefd[2]={-1,-1}; 


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

void print_monitor_output()
{
  char string[4096];
  ssize_t n;
  while ((n=read(pipefd[0], string,sizeof(string)-1))>0)
    {
      string[n]='\0';
      printf("%s", string);
    }
}


void monitor_handler(int signum) 
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
      else if(exec_pid==0) //proces copil
	{
	  close(pipefd[0]);
	  dup2(pipefd[1], 1);

	  if(strcmp(commandd,"--list_hunts")==0 && strlen(huntID)==0 && strlen(treasureID)==0) 
	    {
	      execl(FILENAME,COMMAND,file_full_command,NULL); //  --list_hunts
	    }
	  else if(strcmp(commandd,"--list")==0 && strlen(treasureID)==0) // --list huntID
	    {
	      execl(FILENAME,COMMAND,commandd,huntID,NULL);
	    }
	  else if(strcmp(commandd,"--view")==0) // --view huntID treasureID
	    {
	      execl(FILENAME,COMMAND,commandd,huntID,treasureID,NULL);	      
	    }
	  else if(strcmp(commandd,"calculate_score")==0) 
	    {
	      DIR *dir=opendir("hunts"); //se deschide directorul hunts
	      if(!dir)
		{
		  perror("opendir hunts");
		  exit(-1);
		}
	      printf("\n");
	      struct dirent *entry;
	      
	      while((entry=readdir(dir))!=NULL)
		{
		 
		  if(entry->d_type == DT_DIR && (strcmp(entry->d_name, ".")) && (strcmp(entry->d_name,"..")))
		    {
		      printf("HUNT %s\n",entry->d_name);
		      pid_t score_pid=fork(); //proces pentru a calcula scorul per hunt
		      if(score_pid<0)
			{
			  perror("Fork error");
			  exit(-1);
			}
		      else if(score_pid==0) //copilul executa score_calculator
			{
			  close(pipefd[0]);
			  dup2(pipefd[1],1);
			  execl(SCORE_FILENAME,SCORE_COMMAND,entry->d_name,NULL); //calculate_score huntID
			  perror("execl score_calculator failed");
			  exit(1);	  
			}
		      else //in parinte
			{
			  close(pipefd[1]);
			  int status;
			  waitpid(score_pid, &status,0);
			  dprintf(pipefd[1], "-------------------------\n");
			}
		      printf("\n");
		    }
		  errno=0;
		}
	      
	      if (errno != 0) //eroare la citire
		{
		  perror("readdir");
		  exit(-1);
		}
	      
	      closedir(dir);
	      exit(0);
	    }
	  else
	    {
	      perror("Invalid command for execl\n");
	      exit(1);
	    }
	  perror("execl failed\n");
	  exit(1);
	}

      close(pipefd[1]);
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

      pid_t p_pid=getppid(); //returneaza pid-ul parintelui procesului curent
      if(kill(p_pid,SIGUSR2)<0) //trimite semnal catre parinte
	{
	  perror("SIGUSRS2 not sent to parent\n");
	  exit(-1);
	    
	}
      
    }
  else if(signum==SIGTERM)
    {
      usleep(5000000);
      terminate=1;
    }  
}


void list_hunts(pid_t pid)
{
  write_command(COMMAND_FILE_PATH,"--list_hunts");
  if(kill(pid,SIGUSR1)<0) //trimite semnalul SIGUSR1 la monitor
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

int monitor() //aici asteptam semnal dupa ce am pornit monitorul
{
  //setam handler-ul

  struct sigaction sa1;
  sa1.sa_handler=monitor_handler; //cand vine un semnal se apeleaza monitor_handler
  sigemptyset(&sa1.sa_mask);
  sa1.sa_flags=0;

  if(sigaction(SIGUSR1, &sa1,NULL)==-1 || sigaction(SIGTERM, &sa1,NULL)==-1) //atasam handlerul la semnalele sigusr1, sigterm
    {
      perror("Sigaction monitor error\n");
      exit(-1);
    }

  while(1)
    {
      if(!terminate) //la SIGTERM handlerul va seta terminate=1 si se va termina monitorul
	{
	  sleep(1);
	}
      else
	{
	  return 0;
	}
    }
  
}

void endcommand_handler(int signum) 
{
  if(signum==SIGUSR2)
    {
      monitor_running=0;
    }
}

void block_while_running() //blocam totul pana monitorul termina comanda, inainte de urmatorul loop al meniului
{
  //setam handler-ul
  struct sigaction sa2;
  sa2.sa_handler=endcommand_handler;
  sigemptyset(&sa2.sa_mask); //niciun semnal nu va fi blocat in timp ce se executa handlerul
  sa2.sa_flags=0;

  if(sigaction(SIGUSR2, &sa2,NULL)==-1) //atasam handler-ul la semnalul sigusr2
    {
      perror("Sigaction error for SIGUSR2");
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
    }

  else if (pid > 0)
    {
      printf("Monitor is already running\n");
    }
  else //daca nu exista deja proces monitor
    {
      if(pipe(pipefd)<0)
	{
	  perror("Pipe error");
	  exit(-1);
	}
      
      printf("Monitor started successfully\n");
      
      pid = fork(); //cream proces copil
      if (pid < 0)
	{
	  perror("Fork error");
	  exit(-1);
	}

      if (pid == 0) //procesul parinte ramane in meniu, copilul merge in monitor()
	{
	  close(pipefd[0]); //monitor scrie in pipefd[1]
	  monitor();
	  close(pipefd[1]); 
	  exit(0);
	}
      close(pipefd[1]); //meniul citeste din pipefd[0]
      
    }

}

void stop_monitor()
{
  if(monitor_terminating)
    {
      printf("Monitor is terminating, please wait\n");
    }
  else if(pid>0) //daca exista proces copil activ
    {
      printf("Monitor terminating in progress...\n"); //se incepe procesul de oprire a monitorului
      kill(pid,SIGTERM); //trimit semnalul de terminare spre procesul monitor (monitor_loop)
      monitor_terminating=1;
      close(pipefd[0]);
    }
  else printf("Monitor is not open\n");
}


void exit_menu()
{
  if (monitor_terminating)
    {
      printf("Monitor is terminating, please wait\n");
    }
  else if (pid > 0) //  exista un proces copil activ
    {
      printf("Monitor is still running. Please stop it first using 'stop_monitor'\n");
    }
  else if (pid <= 0) // nu exista proces copil activ
    {
      printf("Exited successfully\n");
      exit(0); 
    }
}

void calculate_score(pid_t pid)
{
  write_command(COMMAND_FILE_PATH,"calculate_score");

  if(kill(pid,SIGUSR1)<0)
    {
      perror("Send SIGUSR1 to monitor error");
      exit(-1);
    }
}


int main()
{

  char command[100]={'\0'};

  while(1)
    {
      printf("\nCommand options: \n start_monitor\n list_hunts\n list_treasures\n view_treasure\n calculate_score\n stop_monitor\n exit\n");
      printf("Type option:\n");
      printf(">> ");

      fgets(command,sizeof(command),stdin);
      command[strcspn(command, "\n")] = 0;

      if(pid > 0) //verificare daca monitorul a murit
	{ 
	  int status = 0;
	  int result = waitpid(pid, &status, WNOHANG);
	  if(result == -1)
	    {
	      perror("Waitpid error");
	      exit(-1);
	    }
	  else if(result > 0) //daca a murit monitorul
	    {
	      monitor_terminating = 0;
	      pid = -1;
	    }
	}

      if(strcmp(command,"start_monitor")==0)
	  start_monitor();

      else if(strcmp(command,"list_hunts")==0)
	{
	  if(monitor_terminating)
	    {
	      printf("Monitor is terminating, please wait\n");
	    }
	  else if(pid>0)
	    {
	      monitor_running=1;
	      list_hunts(pid);
	      block_while_running(); //asteapta finalul comenzii actuale pana sa putem introduce urmatoarea comanda
	      print_monitor_output();
	    }
	  else printf("Monitor is not open\n");
	}

      else if(strcmp(command,"list_treasures")==0)
	{
	  if(monitor_terminating)
	    {
	      printf("Monitor is terminating, please wait\n");
	    }
	  else if(pid>0)
	    {
	      monitor_running=1;
	      list_treasures(pid);
	      block_while_running();
	      print_monitor_output();
	    }
	  else printf("Monitor is not open\n");
	}

      else if(strcmp(command,"view_treasure")==0)
	{
	  if(monitor_terminating)
	    {
	      printf("Monitor is terminating, please wait\n");
	    }
	  else if(pid>0)
	    {
	      monitor_running=1;
	      view_treasure(pid);
	      block_while_running();
	      print_monitor_output();
	    }
	  else printf("Monitor is not open\n");
	}

      else if(strcmp(command,"stop_monitor")==0)
	{
	  stop_monitor();
	}

      else if(strcmp(command,"exit")==0)
	{
	  exit_menu();
	}
      else if(strcmp(command,"calculate_score")==0)
	{
	  if(monitor_terminating)
	    {
	      printf("Monitor is terminating, please wait\n");
	    }
	  else if(pid>0)
	    {
	      monitor_running=1;
	      calculate_score(pid);
	      block_while_running();
	      print_monitor_output();
	    }
	  else printf("Monitor is not open\n");
	}
      else printf("Unknown command\n");
    }
  return 0;
}
