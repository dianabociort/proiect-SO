#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // pentru open
#include <unistd.h>     // pentru write, close, lseek
#include <sys/stat.h>   // pentru mkdir
#include <time.h>      
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <dirent.h>

#define HUNTS_DIR "hunts"
#define TREASURES_FILE "treasures.dat"
#define LOG_FILE "logged_hunt"
#define LINK_PREFIX "logged_hunt-"
#define MAX_PATH_LEN 256
#define MAX_CLUE_LEN 100
#define MIN_CLUE_LEN 5
#define MAX_USERNAME_LEN 20
#define MIN_USERNAME_LEN 3

typedef struct{
  int treasureID;
  char userName[MAX_USERNAME_LEN];
  float latitude;
  float longitude;
  char clue[MAX_CLUE_LEN];
  int value;
}Treasure;

/*
void create_hunt_directory(const char *hunt_id);
char *get_treasure_filepath(const char *hunt_id,char *file_path);
int open_treasure_file_for_write(const char *hunt_id);
int open_treasure_file_for_read(const char *hunt_id);
Treasure read_treasure_from_input();
void close_file(int fd);
void write_treasure_to_file(int fd,Treasure *treasure);
char *get_logpath(char *log_path,const char *hunt_id);
void log_treasure_add(const char *hunt_id, Treasure *treasure);
void create_symlink_for_log(const char* hunt_id);
void add_treasure(const char *hunt_id);
void print_treasure(Treasure treasure);
void list_treasures(const char* hunt_id);
void view_treasure(const char *hunt_id, int treasure_id);
*/

void create_hunt_directory(const char *hunt_id)
{
  char dir_path[MAX_PATH_LEN];
  snprintf(dir_path,sizeof(dir_path), "%s/%s", HUNTS_DIR, hunt_id);
  //creez directorul daca nu exista
  if(mkdir(dir_path,0777)==-1 && errno !=EEXIST)
    {
      perror("Mkdir error\n");
      exit(-1);
    }
}
char *get_treasure_filepath(const char *hunt_id,char *file_path)
{
  snprintf(file_path,MAX_PATH_LEN,"%s/%s/%s",HUNTS_DIR,hunt_id,TREASURES_FILE);
  return file_path;
}

int open_treasure_file_for_write(const char *hunt_id)
{
  //construiesc calea catre fisierul binar
  char file_path[MAX_PATH_LEN];
  get_treasure_filepath(hunt_id, file_path);

  //deschid fisierul pentru scriere
  int fd = open(file_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
  if (fd == -1)
  {
    perror("Open for write error");
    exit(-1);
  }
  return fd;
}


int open_treasure_file_for_read(const char *hunt_id)
{
  char file_path[MAX_PATH_LEN];
  get_treasure_filepath(hunt_id, file_path);

  int fd = open(file_path, O_RDONLY);
  if (fd == -1)
  {
    perror("Open for read error");
    exit(-1);
  }
  return fd;
}

void trim_whitespace(char *str)
{
  int start = 0;
  int end = strlen(str) - 1;

  while (isspace(str[start]))
    start++; //ajunge pe primul caracter nenul

  while (end >= start && isspace(str[end]))
    end--; //ajunge pe ultimul element nenul

  int i=0;
  //muta tot la începutul stringului, inclusiv '\0'
  while (start <= end)
    {
      str[i] = str[start];
      i++;
      start++;
    }
  str[i] = '\0';
}

int has_valid_length(const char* username,int MIN,int MAX)
{
    int len = strlen(username);
    return (len >= MIN && len <= MAX);
}

int has_valid_characters(const char* username)
{
  int i=0;
  while(username[i]!='\0')
    {
      char c = username[i];
      if (!isalnum(c) && c != '_' && c != '.')
	return 0;
      i++;
    }
  return 1;
}

int has_valid_start_end(const char *username)
{
  int len=strlen(username);
  if(username[0]=='.' || username[0]=='_' || username[len-1]=='.' || username[len-1]=='_')
    return 0;
  return 1;
}

int hasnt_only_digits(const char *username)
{
  int only_digits=1;
  int i=0;
  while(username[i]!='\0')
    {
      if(!isdigit(username[i]))
	only_digits=0;
      i++;
    }
  if(only_digits)
    return 0;
  return 1;
}

int is_valid_username(const char* username)
{
  return has_valid_length(username,MIN_USERNAME_LEN,MAX_USERNAME_LEN) && has_valid_characters(username)
    && hasnt_only_digits(username) && has_valid_start_end(username);
}

void clear_stdin_buffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


float read_latitude()
{
  char buffer[64];
  float latitude;
  int valid = 0;
  while (!valid) {
    printf("Introduce the latitude [-90,90]: ");
    if (!fgets(buffer, sizeof(buffer), stdin))
      {
	printf("Input error. Please try again.\n");
	continue;
      }
    // convertire din string in float
    if (sscanf(buffer, "%f", &latitude) == 1)
      {
	if (latitude < -90.0 || latitude > 90.0)
	  {
	    printf("Invalid latitude. Please try again.\n");
	  }
	else
	  {
	    valid = 1;
	  }
      }
    else
      {
	printf("Invalid input. Please introduce a numeric value.\n");
      }
  }
  return latitude;
}

float read_longitude() {
  char buffer[64];
  float longitude;
  int valid = 0;
  while (!valid)
    {
      printf("Introduce the longitude [-180,180]: ");
      if (!fgets(buffer, sizeof(buffer), stdin))
	{
	  printf("Input error. Please try again.\n");
	  continue;
        }
      if (sscanf(buffer, "%f", &longitude) == 1)
	{
	  if (longitude < -180.0 || longitude > 180.0)
	    {
	      printf("Invalid longitude. Please try again.\n");
            }
	  else
	    {
	      valid = 1;
            }
        }
      else
	{
	  printf("Invalid input. Please introduce a numeric value.\n");
        }
    }
  return longitude;
}

int read_positive_id()
{
  char buffer[64];
  int id;
  int valid = 0;
  while (!valid)
    {
      printf("Introduce the treasure ID (positive number): ");
      if (!fgets(buffer, sizeof(buffer), stdin))
	{
	  printf("Input error. Please try again.\n");
	  continue;
        }
      // convertire din string in int
      if (sscanf(buffer, "%d", &id) == 1)
	{
	  if (id < 0) {
	    printf("Invalid ID. Please try again.\n");
	  }
	  else
	    {
	      valid = 1;
            }
        }
      else
	{
	  printf("Invalid input. Please introduce a positive integer.\n");
        }
    }
  return id;
}

int read_positive_value()
{
  char buffer[64];
  int value;
  int valid = 0;
  while (!valid)
    {
      printf("Introduce the value (positive number): ");
      if (!fgets(buffer, sizeof(buffer), stdin))
	{
	  printf("Input error. Please try again.\n");
	  continue;
        }
      if (sscanf(buffer, "%d", &value) == 1)
	{
	  if (value <= 0) {
	    printf("Invalid value. Please try again.\n");
	  }
	  else
	    {
	      valid = 1;
            }
        }
      else
	{
	  printf("Invalid input. Please introduce a positive integer.\n");
        }
    }
  return value;
}

Treasure read_treasure_from_input() //cu basic input data validation
{
  Treasure treasure;
  //citire date pentru comoara

  //treasure ID validation
              //sa fie un int pozitiv
  treasure.treasureID=read_positive_id(); 
 
  //userName validation
              //dimensiunea mai mare de 2 si mai mica de 19
              //stergere spatiile albe de dinainte sau dupa
              //nu e case-sensitive
              //poate contine doar litere, cifre, _ si . dar nu le avea _ sau . la inceput sau sfarsit
              //nu poate fi format doar din cifre
  int too_long;
  do{
    too_long=0;
    printf("Introduce the userName: ");
    fgets(treasure.userName,sizeof(treasure.userName),stdin);
    if (treasure.userName[strlen(treasure.userName)-1] != '\n')
      {
	clear_stdin_buffer();
	too_long=1;
      }
    treasure.userName[strcspn(treasure.userName, "\n")] = '\0';
    trim_whitespace(treasure.userName);
    for (int i = 0; treasure.userName[i]; i++) //transform totul in litere mici  // DE VERIFICAT FUNCTIONALITATE
      treasure.userName[i] = tolower(treasure.userName[i]);

    if (!has_valid_length(treasure.userName,MIN_USERNAME_LEN,MAX_USERNAME_LEN) || too_long==1)
        printf("Username has to be longer than %d and shorter than %d characters.\n", MIN_USERNAME_LEN, MAX_USERNAME_LEN-1);
    else if (!has_valid_characters(treasure.userName))
        printf("Username can only contain letters, numbers, _ and .\n");
    else if (!has_valid_start_end(treasure.userName))
        printf("Username cannot end or start with a _ or .\n");
    else if (!hasnt_only_digits(treasure.userName))
        printf("Username cannot contain just numbers.\n");
  }while(!is_valid_username(treasure.userName) || too_long==1);


  //latitude and longitude validation
                //trebuie sa fie in intervalul realistic al latitudinii si longitudinii
  treasure.latitude=read_latitude();
  treasure.longitude=read_longitude();

  do{
    too_long=0;
    printf("Introduce the clue: ");
    fgets(treasure.clue,sizeof(treasure.clue),stdin);
    if (treasure.clue[strlen(treasure.clue)-1] != '\n')
      {
	clear_stdin_buffer();
	too_long=1;
      }
    treasure.clue[strcspn(treasure.clue, "\n")] = '\0';
    trim_whitespace(treasure.clue);
    if(!has_valid_length(treasure.clue,MIN_CLUE_LEN,MAX_CLUE_LEN) || too_long==1)
      {
	printf("Clue has to be longer than %d and shorter than %d characters\n",MIN_CLUE_LEN,MAX_CLUE_LEN);
      }
  }while(!has_valid_length(treasure.clue,MIN_CLUE_LEN,MAX_CLUE_LEN) || too_long==1);

  //value validation
                  //sa fie un int pozitiv
  treasure.value=read_positive_value();

  return treasure;
}

void close_file(int fd)
{
  if(close(fd)==-1)
    {
      perror("Close file error\n");
      exit(-1);
    }
}

void write_treasure_to_file(int fd,Treasure *treasure)
{
  if(write(fd,treasure,sizeof(Treasure))!=sizeof(Treasure))
    {
      perror("Write treasure error\n");
      close_file(fd);
      exit(-1);
    }
  close_file(fd);
}
char *get_logpath(char *log_path,const char *hunt_id)
{
  snprintf(log_path,MAX_PATH_LEN, "%s/%s/%s",HUNTS_DIR,hunt_id,LOG_FILE);
  return log_path;
}

void log_treasure_add(const char *hunt_id, Treasure *treasure)
{
  //scriu log in logged_hunt
  char log_path[MAX_PATH_LEN];
  get_logpath(log_path,hunt_id);

  int log_fd=open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
  if(log_fd==-1)
    {
      perror("Open log file error");
      exit(-1);
    }

  time_t now=time(NULL);
  char *timestamp=ctime(&now);
  char log_entry[MAX_PATH_LEN];
  snprintf(log_entry,MAX_PATH_LEN,"[%.*s] Added treasure with ID %d from user %s\n", (int)(strlen(timestamp)-1),timestamp,treasure->treasureID,treasure->userName);

  write(log_fd,log_entry,strlen(log_entry));
  close_file(log_fd);
}

void create_symlink_for_log(const char* hunt_id)
{
  //creez symlink daca nu exista deja
  char log_path[256];
  get_logpath(log_path,hunt_id);

  char symlink_name[MAX_PATH_LEN];
  snprintf(symlink_name,sizeof(symlink_name),"%s%s", LINK_PREFIX,hunt_id);

  
  struct stat st;
  // verifica dacă symlink-ul exista deja
  if (lstat(symlink_name, &st) == 0) {
    return;
  }
  
  if(symlink(log_path,symlink_name)!=0)
    {
      perror("Failed to create symlink");
    }
  
}

void add_treasure(const char *hunt_id)
{
  create_hunt_directory(hunt_id);
  int fd=open_treasure_file_for_write(hunt_id);
  Treasure treasure=read_treasure_from_input();
  write_treasure_to_file(fd,&treasure);
  log_treasure_add(hunt_id,&treasure);
  create_symlink_for_log(hunt_id);

  printf("Successfully added treasure!\n");
 
}

void print_treasure(Treasure treasure) //afisez toate treasureurile dintr-un hunt 
{
  printf("ID: %d\n",treasure.treasureID);
  printf("User: %s\n",treasure.userName);
  printf("Coordonates: %.4f, %.4f\n",treasure.latitude,treasure.longitude);
  printf("Clue: %s\n",treasure.clue);
  printf("Value: %d\n",treasure.value);
  printf("----------------\n");
}

void list_treasures(const char* hunt_id) //listare treasures dintr-un anumit hunt
{
  char file_path[MAX_PATH_LEN];
  get_treasure_filepath(hunt_id,file_path);
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

  //deschid fisierul de treasures, afisare toate treasuresurile, close
  int fd=open_treasure_file_for_read(hunt_id);
  Treasure treasure;
  printf("\nTreasure list: \n\n");
  while(read(fd,&treasure,sizeof(Treasure))==sizeof(Treasure))
    {
      print_treasure(treasure);
    }
  close_file(fd);

  
}

//adaugare functionalitate pentru treasure_hub

int count_treasures(const char *hunt_id) //numara treasure-urile dintr-un hunt
{ 
  int count=0;
  
  int fd=open_treasure_file_for_read(hunt_id);
  
  Treasure treasure;
  while(read(fd,&treasure,sizeof(Treasure))==sizeof(Treasure))
    {
      count++;
    }
  close_file(fd);
  return count;
}

void list_hunts()
{
  DIR *dir;
  struct dirent *entry;

  dir=opendir(HUNTS_DIR);
  if(dir==NULL)
    {
      perror("Couldn't open hunts directory");
      exit(-1);
    }

  printf("Available hunts:\n");
  while((entry=readdir(dir))!=NULL)
    {
      if(strcmp(entry->d_name,".")==0 || strcmp(entry->d_name, "..")==0)
      continue;
      if(entry->d_type==DT_DIR)
	{
	  printf(" - %s (%d treasures)\n",entry->d_name,count_treasures(entry->d_name));
	}
    }
  closedir(dir);
}
//

void view_treasure(const char *hunt_id, int treasure_id)
{
  
  int fd=open_treasure_file_for_read(hunt_id);

  Treasure treasure;
  int found=0;
  while(read(fd,&treasure,sizeof(Treasure))==sizeof(Treasure))
    {
      if(treasure.treasureID==treasure_id)
	{
	  found=1;
	  print_treasure(treasure);
	  break;
	}
    }

  if(found==0)
    {
      printf("Treasure with ID %d was not found in hunt %s.\n",treasure_id,hunt_id);
    }

  close_file(fd);
  
}

void remove_hunt(const char *hunt_id)
{
  char file_path[MAX_PATH_LEN];
  get_treasure_filepath(hunt_id,file_path);

  //sterg fisierul treasures.dat
  if(unlink(file_path)==-1 && errno != ENOENT)
    {
      perror("Error deleting treasures file\n");
      exit(-1);
    }

  //sterg fisierul de log_hunt
  get_logpath(file_path,hunt_id);
  if(unlink(file_path)==-1 && errno!=ENOENT)
    {
      perror("Error deleting log file\n");
      exit(-1);
    }
  

  //sterg symlink-ul
  char symlink_path[MAX_PATH_LEN];
  snprintf(symlink_path,MAX_PATH_LEN, "%s%s", LINK_PREFIX,hunt_id);
  if(unlink(symlink_path)==-1 && errno!=ENOENT)
    {
      perror("Error deleting symlink\n");
      exit(-1);
    }

    //sterg directorul huntului (game)
    snprintf(file_path,MAX_PATH_LEN,"%s/%s",HUNTS_DIR,hunt_id);
    if(rmdir(file_path)==-1 && errno!=ENOENT)
      {
	perror("Error deleting hunt directory");
	exit(-1);
      }
   
}

void log_treasure_removal(const char *hunt_id, int treasure_id)
{
  char log_path[MAX_PATH_LEN];
  get_logpath(log_path,hunt_id);

  int log_fd=open(log_path, O_WRONLY | O_CREAT | O_APPEND, 0666);
  if(log_fd==-1)
    {
      perror("Open log file error");
      exit(-1);
    }

  time_t now=time(NULL);
  char *timestamp=ctime(&now);
  char log_entry[MAX_PATH_LEN];
  snprintf(log_entry,MAX_PATH_LEN,"[%.*s] Removed treasure with ID %d\n",(int)(strlen(timestamp)-1),timestamp,treasure_id);
  write(log_fd,log_entry,strlen(log_entry)); 
  close_file(log_fd);
}

void remove_treasure(const char *hunt_id,int treasure_id)
{
  char file_path[MAX_PATH_LEN];
  get_treasure_filepath(hunt_id,file_path);


  int fd=open(file_path,O_RDWR);
  if(fd==-1)
    {
      perror("Open file for read/write error\n");
      exit(-1);
    }

  int pos=0;
  int found=0;
  Treasure treasure;
  //gasesc unde se afla comoara care trebuie stearsa
  while(read(fd,&treasure,sizeof(Treasure))==sizeof(Treasure))
    {
      if(treasure.treasureID==treasure_id)
	{
	  found=1;
	  break;
	}
      pos=pos+sizeof(Treasure); //de fiecare data cand trec peste o comoara care nu e cea cautata
    }

  if(found==0)
    {
      printf("Treasure not found\n");
      close_file(fd);
      exit(-1);
    }

  //mut in fata toate comorile de dupa comoara stearsa
  Treasure next;
  int src_pos=pos+sizeof(Treasure); //pozitia comorii urmatoare
  int dest_pos=pos; //pozitia unde vrem sa scriem

  while(1)
    {
      lseek(fd,src_pos,SEEK_SET);
      int bytes=read(fd,&next,sizeof(Treasure));
      if(bytes!=sizeof(Treasure))
	break;

      lseek(fd,dest_pos,SEEK_SET);
      write(fd,&next,sizeof(Treasure));

      src_pos=src_pos+sizeof(Treasure);
      dest_pos=dest_pos+sizeof(Treasure);
    }

  ftruncate(fd,dest_pos);
  close_file(fd);
  log_treasure_removal(hunt_id,treasure_id);
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
      remove_hunt(argv[2]);
    }
  else if(argc==4 && strcmp(argv[1],"--remove")==0)
    {
      int id=atoi(argv[3]);
      remove_treasure(argv[2],id);
    }
  else if(argc==3 && strcmp(argv[1],"--list")==0)
    {
      list_treasures(argv[2]);
    }
  else if(argc==4 && strcmp(argv[1],"--view")==0)
    {
      int id=atoi(argv[3]);
      view_treasure(argv[2],id);
    }
  else if(argc==2 && strcmp(argv[1], "--list_hunts")==0)
    {
      list_hunts();
    }
  else
    printf("The option you have entered does not exist\n");
}
