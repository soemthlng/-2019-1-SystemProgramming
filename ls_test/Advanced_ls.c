//////////////////////////////////////////////////////////////////////////////
// File Name   : Advanced_ls.c                                             	//
// Date      : 2019/04/18                                                   //
// Os      : Ubuntu 16.04 64bits                                            //
// Author   : Park Jeong Yong                                               //
// Student ID   : 2016722098                                                //
// ======================================================================== //
// Title : System Programming Assignment #2-2 (Advanced ls function)        //
// Description : Making 'advanced_ls' function which distinguish options.   //
// a option prints hidden files, l option prints details of files.          //
// Options can be written at once, in any order.                            //
// Outputs the files of the path if it exists,                              //
// and the files of the current path if path does not exist                 //
//////////////////////////////////////////////////////////////////////////////


//Include header
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
//End header


//File info struct
typedef struct Dif_Info{
  char							 filename[255];	  // File name
  char               permission[11];  // File permission
  char               userid[255];     // User id
  char               groupid[255];    // Group id
  int                numoflink;       // Number of links
  int                filesize;        // File size
  int                time[2];         // Time
  char               date_str[5];     // Date for monthname
  int                date_int[2];     // Date for day
  char               symlink[4096];   // Symbolic link path
}DInfo;
//End file info struct


//Node struct
typedef struct Node{
  void                *Data;
  struct Node         *NextNode;
}Node;
//End node struct


//Linkedlist struct
typedef struct List{
  int                 size;
  Node                *Head;
}List;
//End linkedlist struct

//Total blocksize
int     Totalsize = 0;

//Linkedlist initialize
void    List_Init(List *list);

//Linkedlist insert
int     List_Insert(List* list, const char *Dirname);

//Linkedlist sorting
void    Sorting(List *list, Node *NewNode);

//Dot files sorting
int     DotCompare(char *file1, char *file2);

//Print function
void    PrintInfo(List *list, char c);

//Miss input check
int     IsWrong(char* FileName);

/ls function
int     ls_function(int Check, int aflag, int lflag, int argc, char **argv);

//Permission check function
void    CheckFilePermission(struct stat *buf, DInfo *dinfo);


////////////////////////////////////////////////////////////////////////////
// main                                                                   //
// ====================================================================== //
// Input: int argc    -> number of parameter                              //
//      : char** argv -> name of directory                                //
// Output: int 0 default                                                  //
// Purpose: Check options and count number of paths to call ls_function   //
////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv){
  int             aflag = 0, lflag = 0, CheckOpt = 0; // Option flags
  int             c = 0;

  while((c = getopt(argc, argv, "al")) != -1) // Get options
  {
     switch(c){
        case 'a' :
                       aflag = 1; // a option
                       break;
        case 'l' :
                       lflag = 1; // l option
                       break;
        case '?' :
                       printf("Error\n"); // Error
                       return 0;
        default  :
                       break;
     }
  }
  if(argc == optind) CheckOpt = 1; // No path
  ls_function(CheckOpt, aflag, lflag, argc, argv);
  return 0;
}


//////////////////////////////////////////////////////
// List_Init                                        //
// ================================================ //
// Input: List *list -> structure pointer of List   //
// Output: void                                     //
// Purpose: Initialize linked list                  //
//////////////////////////////////////////////////////

void List_Init(List *list){
  list->size  = 0;
  list->Head  = NULL;
  return;
}
//End List start


////////////////////////////////////////////////////////////////
// List_Insert                                                //
// ========================================================== //
// Input: List *list            -> structure pointer of List  //
//      : const char *Dirname   -> Directory name pointer     //
// Output: int  0 default                                     //
//       : int -1 error                                    		//
// Purpose: Insert filedatas in linkedlist and sort them      //
////////////////////////////////////////////////////////////////
int List_Insert(List* list, const char *Dirname){
  ///////////////////////////Declare variables//////////////////////////////////
  struct stat       buf;          // Get file's data
  struct group      *grp;         // Get groupname
  struct passwd     *pwd;         // Get username
  struct tm         *time;        // Get time
  DInfo             *dinfo;       // Filedata structure
  char              Monthname[5]; // Save month from int to char
  /////////////////////////End declare variables////////////////////////////////

  ////////////////////////Create new node and acllocate datas//////////////////
  Node *NewNode;
  if((NewNode = (Node*)malloc(sizeof(Node))) == NULL) return -1;

  if(!lstat(Dirname, &buf)){ // buf gets filename
    dinfo = (DInfo*)malloc(sizeof(DInfo));
  ////////////////////////Set username and group name///////////////////////
    pwd = getpwuid(buf.st_uid);
    grp = getgrgid(buf.st_gid);
    strcpy(dinfo->userid, pwd->pw_name);
    strcpy(dinfo->groupid, grp->gr_name);

  ////////////////////////Set number of link and filesize//////////////////////
    dinfo->numoflink = buf.st_nlink;
    dinfo->filesize = buf.st_size;

  ////////////////////////Set time of file///////////////////////

    time = localtime(&buf.st_mtime);
    strftime(Monthname, 5, "%b", time);
    strcpy((dinfo->date_str),Monthname);
    dinfo->date_int[0]=time->tm_mday;
    dinfo->time[0]=time->tm_hour;
    dinfo->time[1]=time->tm_min;

  ////////////////////////Set file permission, save data into node and set nextnode to NULL///////////////////////
    CheckFilePermission(&buf, dinfo);
    strcpy(dinfo->filename,Dirname);
    NewNode->Data = dinfo;
    NewNode->NextNode = NULL;

  ////////////////////////Set blocksize///////////////////////
    Totalsize += buf.st_blocks;
  }
  ////////////////////////Sorting///////////////////////
  Sorting(list, NewNode);

  return 0;
}
///End List_Insert


//////////////////////////////////////////////////
// Sorting                                      //
// ============================================ //
// Input: List *list 		 -> pointer of List     //
//      : Node *NewNode  -> Newly inputted node	//
// Output: void                                 //
// Purpose: Sorting linkedlists  				        //
//////////////////////////////////////////////////
void Sorting(List *list, Node *NewNode){
  Node		*ptr, *cur, *prev; 			// Pointing nodes
	char 		*str1, *str2;						// Dot compare variable

	if(list->Head == NULL)	list->Head = NewNode; // Head is NULL
	else{ // Head is not NULL

////////////////////////////////Initialize pointers///////////////////////////
		ptr = list->Head;
		prev = ptr;
		cur = ptr;

//////////////////////////Sorting in ascend order/////////////////////////////
		while(cur != NULL){
			str1 = ((DInfo*)cur->Data)->filename;
			str2 = ((DInfo*)NewNode->Data)->filename;

			if(DotCompare(str1, str2) > 0){	// Sort regardless of .
				if(cur == list->Head){
					list->Head = NewNode;
					NewNode->NextNode = cur;
				}
				else{
					prev->NextNode = NewNode;
					NewNode->NextNode = cur;
				}
				break;
			}
			else{
				prev = cur;
				cur = cur->NextNode;
			}
		}

///////////////////////////////Input newnode at end///////////////////////////
		if(cur == NULL){
			prev->NextNode = NewNode;
		}
	}
/////////////////////////////////End sorting nodes////////////////////////////
	list->size++;
	return ;
}
// End sorting


//////////////////////////////////////////////////////////////
// DotCompare                                      					//
// ======================================================== //
// Input: char *file1 	-> former filename     							//
//      : char *file2  	-> rear filename 										//
// Output: int str1 - str2		-> check which one is bigger	//
// Purpose: Compare filenames regardless of .  							//
//////////////////////////////////////////////////////////////

int DotCompare(char *file1, char *file2){
  char 		*filename1 = file1;		// former filename
	char		*filename2 = file2;		// rear filename
	char 		str1, str2;						// variable for change index

	do{
//////////////////////////////Search for the next letter////////////////////////
		 str1 = *filename1++;
		 str2 = *filename2++;

/////////////////////////////Search for the next letter/////////////////////////
		 if(str1 == '.') str1 = *filename1++;
		 if(str2 == '.') str2 = *filename2++;

/////////////////////////////Change ACSII in lowercase//////////////////////////
		 if(str1 >= 65 && str1 <= 90) str1 += 32;
		 if(str2 >= 65 && str2 <= 90) str2 += 32;

///////////////Search end of filename while two names are equal/////////////////
		 if(str1 == '\0' || str2 == '\0') return str1 - str2;	// One is short
		 }while(str1 == str2);

		 return str1 - str2;
}
//End dot DotCompare


////////////////////////////////////////////////////////////
// PrintInfo                                              //
// ====================================================== //
// Input: List *list    -> pointer of List                //
//      : char c        -> Option check character         //
// Output: void                                           //
// Purpose: Check options and print each option's format  //
////////////////////////////////////////////////////////////

void    PrintInfo(List *list, char c){
  int       i = 0;            // Index
  Node      *ptr;             // Node pointer
  ///////////////////////////////////print format//////////////////////////////
  switch(c){
  ////////////////////////////////////l option/////////////////////////////////
    case 'l':
              for(i=0, ptr=list->Head; i<list->size; i++, ptr=ptr->NextNode){
                printf("%s", ((DInfo*)ptr->Data)->permission);
                printf("\t%d", ((DInfo*)ptr->Data)->numoflink);
                printf("\t%s", ((DInfo*)ptr->Data)->userid);
                printf("\t%s", ((DInfo*)ptr->Data)->groupid);
                printf("\t%d", ((DInfo*)ptr->Data)->filesize);
                printf("\t%-2s %-2d",((DInfo*)ptr->Data)->date_str,((DInfo*)ptr->Data)->date_int[1]);
                printf("\t%02d:%02d",((DInfo*)ptr->Data)->time[0],((DInfo*)ptr->Data)->time[1]);
                printf("\t%s",((DInfo*)ptr->Data)->filename);
                printf("\n");
              }
              break;
  ///////////////////////////////a and non option//////////////////////////////
    case 'n':
              for(i=0, ptr=list->Head; i<list->size; i++, ptr=ptr->NextNode){
                printf("%s\n", ((DInfo*)ptr->Data)->filename);
              }
              break;
            }
}
//End PrintInfo


/////////////////////////////////////////////////////////////
// IsWrong                                                 //
// ======================================================= //
// Input: char *FileName                                   //
// Output: int 1 default                                   //
//         int 0 filename match                            //
// Purpose: Check inputted dirname is filename or dirname  //
/////////////////////////////////////////////////////////////
int IsWrong(char *FileName){
  DIR             *dirp;
  struct dirent   *dir;
  char            CurPath[255]; // Set path
  int             rt = 1;       // return value

  strcpy(CurPath, "."); // Set path '.'

  if((dirp = opendir(CurPath)) == NULL) return 0; // Error
  else{
    while((dir = readdir(dirp)) != NULL){ // Read current dir
      if(strcmp(dir->d_name, FileName) == 0) rt = 0; // Filename exists
    }
  }
  return rt;
}
//End IsWrong


//////////////////////////////////////////////////////////////////////////////
// ls_function                                                              //
// ======================================================================== //
// Input : int  Check     -> Check pathname's input                         //
//       : int  aflag     -> a option flag                                  //
//       : int  lflag     -> l option flag                                  //
//       : int  argc      -> number of parameter                            //
//       : char **argv    -> parameter double pointer                       //
// Output: int  0 default                                                   //
//       : int -1 ERROR                                                     //
// Purpose: Read directory names and send filenames to List_Insert function //
//           if filename input, check it exists                             //
//////////////////////////////////////////////////////////////////////////////

int ls_function(int Check, int aflag, int lflag, int argc, char** argv){
  DIR             *dirp;            // DIR pointer
  struct dirent   *dir;             // dirent pointer
  char            Dirname[255];     // Directory name array
  char            FileName[255];    // Error input array
  List            *list;            // List pointer
  char            Abspath[255];     // Absolute path array

  ////////////////////////////////Initialize list///////////////////////////////
  list = (List*)malloc(sizeof(List));
  List_Init(list);

  ///////////////////////////////////al option//////////////////////////////////
  if(aflag && lflag){
    if(Check){ // NO path
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0; // ERROR
      else{
        if(chdir(Dirname) == -1) return -1; //ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // ERROR
        while((dir = readdir(dirp)) != NULL){ // Read dir
          if((List_Insert(list, dir->d_name)) == -1){ // Insert to Linkedlist
            printf("List insert error\n");
            return 0;
          }
        }
      }
      printf("Directory path : %s\n", Abspath); // Print Absolute path
      printf("total : %d\n", Totalsize/2); // Print blocksize
      Totalsize = 0;  // Initialize blocksize
      PrintInfo(list, 'l'); // Call print format
    }
    else if(!Check){ // YES path
      for(optind; optind < argc; optind++){
        strcpy(Dirname, argv[optind]); // Set path

        if((dirp = opendir(Dirname)) == NULL){ // Input is not dir
          if(IsWrong(Dirname)) printf("cannot access '%s' : No such file or directory\n", Dirname); // Check if it is filename
          else if((List_Insert(list, Dirname)) == -1){ // Insert to Linkedlist
            printf("List insert error\n");
            return 0;
          }
        }
        else{ // Input is dir
          if(chdir(Dirname) == -1) return -1; // ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // ERROR
          while((dir = readdir(dirp)) != NULL){ // Read dir
            if((List_Insert(list, dir->d_name)) == -1){ // Insert to Linkedlist
              printf("List insert error\n");
              return 0;
            }
          }
          printf("Directory path : %s\n", Abspath); // Print Absolute path
          printf("total : %d\n", Totalsize/2); // Print blocksize
          Totalsize = 0;  // Initialize blocksize
          PrintInfo(list, 'l'); // Call print format
        }
      }
    }
    return 0;
  }
  ////////////////////////////////////l option//////////////////////////////////
  else if(lflag){
    if(Check){ // NO path
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0; // ERROR
      else{
        if(chdir(Dirname) == -1) return -1; // ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // ERROR
        while((dir = readdir(dirp)) != NULL){ // Read dir
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name)) == -1){ // Insert to Linkedlist
            printf("List insert error\n");
            return 0;
          }
        }
      }
      printf("Directory path : %s\n", Abspath); // Print Absolute path
      printf("total : %d\n", Totalsize/2); // Print blocksize
      Totalsize = 0;  // Initialize blocksize
      PrintInfo(list, 'l'); // Call print format
    }
    else if(!Check){ //YES path
      for(optind; optind < argc; optind++){
        strcpy(Dirname, argv[optind]); // Set path

        if((dirp = opendir(Dirname)) == NULL){ // Input is not dir
          if(IsWrong(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname); // Check if it is filename
          else if((List_Insert(list, Dirname)) == -1){ // Insert to Linkedlist
            printf("List insert error\n");
            return 0;
          }
        }
        else{ // Input is dir
          if(chdir(Dirname) == -1) return -1; // ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // ERROR
          while((dir = readdir(dirp)) != NULL){ // Read dir
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name)) == -1){ // Insert to Linkedlist
              printf("List insert error\n");
              return 0;
            }
          }
          printf("Directory path : %s\n", Abspath); // Print Absolute path
          printf("total : %d\n", Totalsize/2); // Print blocksize
          Totalsize = 0;  // Initialize blocksize
          PrintInfo(list, 'l'); // Call print format
        }
      }
    }
    return 0;
  }
  ////////////////////////////////////a option//////////////////////////////////
  else if(aflag){
    if(Check){ // NO path
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0; // ERROR
      else{
        if(chdir(Dirname) == -1) return -1; // ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // ERROR
        while((dir = readdir(dirp)) != NULL){ // Read dir
          if((List_Insert(list, dir->d_name)) == -1){ // Insert to Linkedlist
            printf("List insert error\n");
            return 0;
          }
        }
      }
      Totalsize = 0; // Initialize blocksize
      PrintInfo(list, 'n'); // Call print format
    }
    else if(!Check){ // YES path
      for(optind; optind < argc; optind++){
        strcpy(Dirname, argv[optind]); // Set path

        if((dirp = opendir(Dirname)) == NULL){ // Input is not dir
          if(IsWrong(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname); // Check if it is filename
          else if((List_Insert(list, Dirname)) == -1){ // Insert to Linkedlist
            printf("List insert error\n");
            return 0;
          }
        }
        else{ // Input is dir
          if(chdir(Dirname) == -1) return -1; // ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // ERROR
          while((dir = readdir(dirp)) != NULL){ // Read dir
            if((List_Insert(list, dir->d_name)) == -1){ // Insert to Linkedlist
              printf("List insert error\n");
              return 0;
            }
          }
          Totalsize = 0; // Initialize blocksize
          PrintInfo(list, 'n'); // Call print format
        }
      }
    }
    return 0;
  }
  //////////////////////////////////non option//////////////////////////////////
  else{
    if(Check){ // NO path
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0; // ERROR
      else{
        while((dir = readdir(dirp)) != NULL){ // Read dir
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name)) == -1){ // Insert to Linkedlist
            printf("List insert error\n");
            return 0;
          }
        }
      }
      Totalsize = 0; // Initialize blocksize
      PrintInfo(list, 'n'); // Call print format
    }
    else if(!Check){ // YES path
      for(optind; optind < argc; optind++){ // Repeat for the number of parameters
        strcpy(Dirname, argv[optind]); // Set path

        if((dirp = opendir(Dirname)) == NULL){ // Input is not dir
          if(IsWrong(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname); // Check if it is filename
          else if((List_Insert(list, Dirname)) == -1){ // Insert to Linkedlist
            printf("List insert error\n");
            return 0;
          }
        }
        else{ // Input is dir
          while((dir = readdir(dirp)) != NULL){ // ERROR
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name)) == -1){ // Insert to Linkedlist
              printf("List insert error\n");
              return 0;
            }
          }
        }
      }
      Totalsize = 0; // Initialize blocksize
      PrintInfo(list, 'n'); // Call print format
    }
    return 0;
  }
}


//////////////////////////////////////////////////////////////
// CheckFilePermission                                    	//
// ======================================================== //
// Input: struct stat* buf    -> structure pointer of List  //
//      : DInfo *dinfo        -> filedata structure pointer //
// Output: void                                             //
// Purpose: Check file permission and store into dinfo      //
//////////////////////////////////////////////////////////////
void CheckFilePermission(struct stat *buf, DInfo *dinfo){
  /////////////////////////////////Check file type/////////////////////////////
  if(S_ISDIR(buf->st_mode))
     dinfo->permission[0]='d';
  else if(S_ISLNK(buf->st_mode))
     dinfo->permission[0]='l';
  else if(S_ISCHR(buf->st_mode))
     dinfo->permission[0]='c';
  else if(S_ISBLK(buf->st_mode))
     dinfo->permission[0]='b';
  else if(S_ISSOCK(buf->st_mode))
     dinfo->permission[0]='s';
  else if(S_ISFIFO(buf->st_mode))
     dinfo->permission[0]='P';
  else
     dinfo->permission[ 0]='-';

  ///////////////////////////////Check user permission//////////////////////////
  if(buf->st_mode & S_IRUSR)
     dinfo->permission[1]='r';
  else
     dinfo->permission[1]='-';
  if(buf->st_mode & S_IWUSR)
     dinfo->permission[2]='w';
  else
     dinfo->permission[2]='-';
  if(buf->st_mode & S_IXUSR)
     dinfo->permission[3]='x';
  else if(buf->st_mode & S_ISUID) // User stikcy bit
     dinfo->permission[3]='s';
  else
     dinfo->permission[3]='-';

  //////////////////////////////Check group permission//////////////////////////
  if(buf->st_mode & S_IRGRP)
     dinfo->permission[4]='r';
  else
     dinfo->permission[4]='-';
  if(buf->st_mode & S_IWGRP)
     dinfo->permission[5]='w';
  else
     dinfo->permission[5]='-';
  if(buf->st_mode & S_IXGRP)
     dinfo->permission[6]='x';
  else if(buf->st_mode & S_ISGID)// Group sticky bit
     dinfo->permission[6]='s';
  else
     dinfo->permission[6]='-';

  //////////////////////////////Check other permission//////////////////////////
  if(buf->st_mode & S_IROTH)
     dinfo->permission[7]='r';
  else
     dinfo->permission[7]='-';
  if(buf->st_mode & S_IWOTH)
     dinfo->permission[8]='w';
  else
     dinfo->permission[8]='-';

  /////////////////////////////////Set sticky bit///////////////////////////////
  if(buf->st_mode & S_IXOTH)
  {
     if(buf->st_mode & S_ISVTX)
        dinfo->permission[9]='t';
     else
        dinfo->permission[9]='x';
  }
  else
  {
     if(buf->st_mode & S_ISVTX)
        dinfo->permission[9]='T';
     else
        dinfo->permission[9]='-';
  }

  ////////////////////////////////Set end of array//////////////////////////////
  dinfo->permission[10]='\0';
}
//End CheckFilePermission


/////////////////////////////
//Makefile							   //
/////////////////////////////
//OBJS = Advanced_ls.c		 //
//CC = gcc							   //
//EXEC  = Advanced_ls			 //
//										     //
//all: $(OBJS)						 //
//	$(CC) -o $(EXEC) $^    //
//clean:								   //
//	rm -rf $(EXEC)				 //
/////////////////////////////
