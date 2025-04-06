#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // pentru open
#include <unistd.h>     // pentru write, close, lseek
#include <sys/stat.h>   // pentru mkdir
#include <time.h>      
#include <errno.h>

#define HUNTS_DIR "hunts"
#define TREASURES_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define LINK_PREFIX "logged_hunt-"

typedef struct{
  int treasureID;
  char userName[20];
  float latitude;
  float longitude;
  char clue[100];
  int value; //price
}Treasure;


void add_treasure(const char *hunt_id)
{
  char dir_path[256];
  snprintf(dir_path,sizeof(dir_path), "%s/%s", HUNTS_DIR,hunt_id);

  //creez directorul daca nu exista
  if(mkdir(dir_path,0777)==-1 && errno !=EEXIST)
    {
      perror("Mkdir error\n");
      exit(-1);
    }

  //construiesc calea catre fisierul binar
  char file_path[300];
  snprintf(file_path,sizeof(file_path), "%s/%s", dir_path, TREASURES_FILE);

  //deschid fisierul pentru scriere la final 
  int fd=open(file_path,O_WRONLY | O_CREAT | O_APPEND, 0666);
  if(fd==-1)
    {
      perror("Open file treasures.dat error\n");
      exit(-1);
    }

  //citesc datele pentru comoara
  
  Treasure treasure;
  printf("Introduce the treasure ID: ");
  scanf("%d",&treasure.treasureID);

  printf("Introduce the username: ");
  scanf("%s",treasure.userName);

  printf("Introduce the latitude: ");
  scanf("%f",&treasure.latitude);

  printf("Introduce the longitude: ");
  scanf("%f",&treasure.longitude);

  printf("Introduce the clue: ");
  while (getchar() != '\n');  
  fgets(treasure.clue, sizeof(treasure.clue), stdin);
  treasure.clue[strcspn(treasure.clue, "\n")] = '\0';
  
  printf("Introduce the value: ");
  scanf("%d",&treasure.value);

  if(write(fd, &treasure, sizeof(Treasure))!=sizeof(Treasure))
    {
      perror("Write in file error\n");
      close(fd);
      exit(-1);
    }

  close(fd);

  //Scriu si log in logged_hunt

  char log_path[300];
  snprintf(log_path,sizeof(log_path), "%s/%s", dir_path, LOG_FILE);

  int log_fd=open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
  if(log_fd==-1)
    {
      perror("Write in log file error\n");
      exit(-1);
    }

  time_t now=time(NULL);
  char *timestamp=ctime(&now);
  char log_entry[300];
  snprintf(log_entry,sizeof(log_entry), "[%.*s] Added treasure with ID %d from user %s\n",(int)(strlen(timestamp)-1),timestamp,treasure.treasureID,treasure.userName);
  write(log_fd,log_entry,strlen(log_entry));
  close(log_fd);

  //creez symlink daca nu exista deja

  char symlink_name[300];
  snprintf(symlink_name, sizeof(symlink_name),"%s%s", LINK_PREFIX, hunt_id);

  symlink(log_path, symlink_name);

  printf("Successfully added treasure!\n");
  
}

void list_treasures(const char* hunt_id)
{
  char file_path[256];
  snprintf(file_path,sizeof(file_path),"%s/%s/%s",HUNTS_DIR,hunt_id,TREASURES_FILE);

  //info despre fisier
  struct stat st;
  if(stat(file_path, &st)==-1)
    {
      perror("Stat error\n");
      exit(-1);
    }

  //printare info generale despre hunt
  printf("Hunt: %s\n",hunt_id);
  printf("File size: %ld bytes\n",st.st_size);
  printf("Last modified: %s\n",ctime(&st.st_mtime));

  //deschid fisierul
  int fd=open(file_path,O_RDONLY);
  if(fd==-1)
    {
      perror("Open file error\n");
      exit(-1);
    }

  //citesc si afisez treasureurile
  Treasure treasure;
  printf("\nTreasure list: \n\n");
  while(read(fd,&treasure,sizeof(Treasure))==sizeof(Treasure))
    {
      printf("ID: %d\n",treasure.treasureID);
      printf("User: %s\n",treasure.userName);
      printf("Coordonates: %.4f, %.4f\n",treasure.latitude,treasure.longitude);
      printf("Clue: %s\n",treasure.clue);
      printf("Value: %d\n",treasure.value);
      printf("----------------\n");
    }

  close(fd);
  
}

int main(int argc,char *argv[])
{
  mkdir(HUNTS_DIR, 0777);
  if(argc==3 && strcmp(argv[1],"--add")==0)
    {
      add_treasure(argv[2]);
    }
  else if(argc==3 && strcmp(argv[1],"--remove")==0)
    {
      
    }
  else if(argc==3 && strcmp(argv[1],"--list")==0)
    {
      list_treasures(argv[2]);
    }
  else if(strcmp(argv[1],"--view")==0)
    {
      
    }
}
