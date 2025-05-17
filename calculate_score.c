#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_USERS 100
#define MAX_CLUE_LEN 100
#define MAX_USERNAME_LEN 20
#define MAX_PATH_LEN 256

typedef struct{
  int treasureID;
  char userName[MAX_USERNAME_LEN];
  float latitude;
  float longitude;
  char clue[MAX_CLUE_LEN];
  int value;
}Treasure;

typedef struct{
  char userName[MAX_USERNAME_LEN];
  int score;
}userScore;

int main(int argc, char *argv[])
{
  if(argc < 2)
    {
      fprintf(stderr,"Invalid argument inputs\n");
      exit(-1);
    }

  char treasure_path[MAX_PATH_LEN]; //path-ul spre fisierul binar cu informatiile despre hunt, adica treasures.dat
  snprintf(treasure_path, sizeof(treasure_path),"hunts/%s/treasures.dat",argv[1]);

  FILE *f=fopen(treasure_path,"rb");
  if(f==NULL)
    {
      fprintf(stderr,"Couldn't open treasures.dat file\n");
      exit(-1);
    }

  Treasure t;
  userScore users[MAX_USERS];
  int user_count=0;

  while(fread(&t,sizeof(Treasure),1,f)==1) //citim cate o inregistrare de treasure pe rand
    {
      int found=0;
      for(int i=0;i<user_count;i++) //verificam daca gasim userul citit in vectorul creat
	{
	  if(strcmp(users[i].userName,t.userName)==0) //daca l-am gasit inseamna ca se repeta userul si adaugam scorul la restul de scor al persoanei
	    {
	      users[i].score=users[i].score+t.value;
	      found=1;
	      break;
	    }
	}
      if(!found && user_count < MAX_USERS) //daca am citit o persoana noua, o adaugam in vetor si crestem nr de persoane distincte 
	{
	  strcpy(users[user_count].userName,t.userName);
	  users[user_count].score=t.value;
	  user_count++;
	}
    }

  fclose(f);

  if(user_count==0)
    {
      printf("No treasures found for hunt %s\n",argv[1]);
      return 0;
    }

  for(int i=0;i<user_count;i++) //afisam scorul total al fiecarei persoane
    {
      printf("%s: %d\n",users[i].userName,users[i].score);
    }
  return 0;
}
