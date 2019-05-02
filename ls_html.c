//////////////////////////////////////////////////////////////////////////////
// File Name   : ls_html.c                                             	  //
// Date      : 2019/05/1s                                                   //
// Os      : Ubuntu 16.04 64bits                                            //
// Author   : Park Jeong Yong                                               //
// Student ID   : 2016722098                                                //
// ======================================================================== //
// Title : System Programming Assignment #2-3 (Final ls function)           //
// Description : Making 'final_ls' function with h, r, S options and        //
// wildcard added to Assignment #2-2.                                       //
// We consider three wildcards, [seq], ?, *.                                //
// Any wildcards only written at the end of parameter.                      //
//////////////////////////////////////////////////////////////////////////////


//Include headers
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
#include <fnmatch.h>
#include <errno.h>
//End Include headers


//File information struct
typedef struct Dif_Info{
  char							 filename[255];	   // File name
  char               pathname[255];    // Path name
  char               permission[11];   // File permission
  char               userid[255];      // User id
  char               groupid[255];     // Group id
  int                numoflink;        // Number of links
  int                filesize;         // File size
  int                time[2];          // Time
  char               date_str[5];      // Date for monthname
  int                date_int[2];      // Date for day
  char               symlink[4096];    // Symbolic link path
}DInfo;
//End File information struct


//Node struct
typedef struct Node{
  void                *Data;            // Store file informations
  struct Node         *NextNode;        // Index of nextnode
  struct Node         *Prev;            // Index of previousnode
}Node;
//End Node struct


//List struct
typedef struct List{
  int                 size;             // List size
  Node                *Head;            // List head
  Node                *Tail;            // List tail
}List;
//End List struct


// Total blocksize variable
int     Totalsize = 0;

// Init list function
void    List_Init(List *list);

// Insert node in list function
int     List_Insert(List* list, const char *Dirname, char flag);

// Delete node function
void    List_Delete(List *list);

// Sorting files in ascending order by string
void    Sorting(List *list, Node *NewNode); // a b c d....

// Sorting files in descending order by size, if size equals ascemding order by string
void    SortingBySize(List *list, Node *NewNode); // 4 3 2 1... / a b c d...

// Sorting files in decending order by string
void    SortingReverse(List *list, Node *NewNode); // d c b a....

// Sorting files in ascending order by size, if size equals decemding order by string
void    SortingReverseBySize(List *list, Node *NewNode); // 1 2 3 4... / d c b a...

//Dot compare
int     DotCompare(char *file1, char *file2);

// Print filedata function
void    PrintInfo(List *list, char c, FILE *file);

// Check input is dir of file
int     IsDir(char *Dirname);

// Tokenizer
char*     Tokenizer(char *path);

// Main purpose function
int     ls_function(int Check, int aflag, int lflag, int hflag, int sflag, int rflag, int argc, char **argv, FILE *file);

// Checking file permission function
void    CheckFilePermission(struct stat *buf, DInfo *dinfo);


////////////////////////////////////////////////////////////////////////////
// main                                                                   //
// ====================================================================== //
// Input: int argc       -> number of parameter                           //
//          : char** argv -> name of directory                            //
// Output: int 0 default                                                  //
// Purpose: Check options and count number of paths to call ls_function   //
////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv){
  int             aflag = 0, lflag = 0, hflag = 0, sflag = 0, rflag = 0;  // flags
  int             CheckOpt = 0;                                           // check number of parameter, not options
  int             c = 0;

  FILE *file = fopen("html_ls.html", "w");
  if(file == 0){
    printf("File size is o\n");
    return -1;
  }
// html header
  fputs("<html>\n<body><h1>", file);
  for(int i=0; i<argc; i++){
    fputs(argv[i], file);
    fputs("&nbsp;", file);
  }
  fputs("</h1>\n", file);
// html header
//////////////////////////////// Check options /////////////////////////////////
  while((c = getopt(argc, argv, "alhSr")) != -1)
  {
     switch(c){
        case 'a' :
                       aflag = 1; // a option
                       break;
        case 'l' :
                       lflag = 1; // l option
                       break;
        case 'h' :
                       hflag = 1; // h option
                       break;
        case 'S' :
                       sflag = 1; // S option
                       break;
        case 'r' :
                       rflag = 1; // r option
                       break;
        case '?' :
                       printf("Error\n"); // ERROR
                       return 0;
        default  :
                       break;
     }
  }
///////////////////////////// End check options ////////////////////////////////

  if(argc == optind) CheckOpt = 1;                                            // No path
  ls_function(CheckOpt, aflag, lflag, hflag, sflag, rflag, argc, argv, file); // call ls function
  return 0;
}
// End Main


//////////////////////////////////////////////////////
// List_Init                                        //
// ================================================ //
// Input: List *list -> structure pointer of List   //
// Output: void                                     //
// Purpose: Initialize lists with doubly linkedlist //
// Warning!!! Head and Tail nodes are dummy nodes.  //
// Do not save values in Head and Tail              //
//////////////////////////////////////////////////////
void List_Init(List *list){
///////////////////////// Initialize Head ( Dummy node) ////////////////////////
  Node* HeadNode = (Node*)malloc(sizeof(Node));
  HeadNode->Data = "H";
  HeadNode->NextNode = NULL;
  HeadNode->Prev = NULL;
///////////////////////// Initialize Tail ( Dummy node) ////////////////////////
  Node* TailNode = (Node*)malloc(sizeof(Node));
  TailNode->Data = "T";
  TailNode->NextNode = NULL;
  TailNode->Prev = NULL;
///////////////////// Initialize list with doubly linkedlist ///////////////////
  list->size  = 0;
  list->Head  = HeadNode;
  list->Tail  = TailNode;
  list->Head->NextNode = list->Tail;
  list->Tail->Prev = list->Head;
  return;
}
// End List_Init


////////////////////////////////////////////////////////////////
// List_Insert                                                //
// ========================================================== //
// Input: List        *list     -> structure pointer of List  //
//      : const char  *Dirname  -> filename character pointer //
//      : char        flag      -> select sorting function    //
// Output: int  0 default                                     //
//       : int -1 error                                    		//
// Purpose: Insert filedatas in linkedlist and sort them      //
////////////////////////////////////////////////////////////////
int List_Insert(List* list, const char *Dirname, char flag){
  struct stat     buf;          // stat variable for file data
  struct group    *grp;         // group variable for groupname
  struct passwd   *pwd ;        // passwd variable for username
  struct tm       *time;        // tm variable for time
  DInfo           *dinfo;       // Information struct variable
  char            Monthname[5]; // convert month int to string variable
/////////////////////// Create and allocate newnode ///////////////////////////
  Node *NewNode;
  if((NewNode = (Node*)malloc(sizeof(Node))) == NULL) return -1;
  if(!lstat(Dirname, &buf)){
    dinfo = (DInfo*)malloc(sizeof(DInfo));
//////////////////////// Set username and group name //////////////////////////
    pwd = getpwuid(buf.st_uid);
    grp = getgrgid(buf.st_gid);
    strcpy(dinfo->userid, pwd->pw_name);
    strcpy(dinfo->groupid, grp->gr_name);
//////////////////////// Set number of link and filesize ///////////////////////
    dinfo->numoflink = buf.st_nlink;
    dinfo->filesize = buf.st_size;
///////////////////////////// Set time of file /////////////////////////////////
    time = localtime(&buf.st_mtime);
    strftime(Monthname, 5, "%b", time);
    strcpy((dinfo->date_str),Monthname);
    dinfo->date_int[0]=time->tm_mday;
    dinfo->time[0]=time->tm_hour;
    dinfo->time[1]=time->tm_min;
//////////////////////// Set file permission, save data into node and set nextnode to NULL ///////////////////////
    CheckFilePermission(&buf, dinfo);
    strcpy(dinfo->filename,Dirname);
    NewNode->Data = dinfo;
    NewNode->NextNode = NULL;
    NewNode->Prev = NULL;
//////////////////////////////// Set blocksize /////////////////////////////////
    Totalsize += buf.st_blocks;
////////////////////////// Call sortinf function ///////////////////////////////
    if(flag == 's') SortingBySize(list, NewNode);
    else if(flag == 'r') SortingReverse(list, NewNode);
    else if(flag == 'q') SortingReverseBySize(list, NewNode);
    else   Sorting(list, NewNode);
  }
////////////////////////////////// ERROR ///////////////////////////////////////
  else{
    printf("%s->\t", Dirname);
    printf("list insert error : %s\n", strerror(errno));
    return -1;
  }
  return 0;
}
//End List_Insert


////////////////////////////////////////////////////////////////
// List_Delete                                                //
// ========================================================== //
// Input: List        *list     -> structure pointer of List  //
// Output: void                                               //
// Purpose: Clear all linkedlist                              //
////////////////////////////////////////////////////////////////
void List_Delete(List *list){
  Node *ptr;
////////////////// Loop while Head's nextnode is Tail //////////////////////////
  while(list->Head->NextNode != list->Tail){
/////////////////////////// Initialize ptr /////////////////////////////////////
    ptr = list->Head->NextNode;
/////////////////////////// Move to Tail's Prev ////////////////////////////////
    while(ptr != list->Tail->Prev){
      ptr = ptr->NextNode;
    }
////////////////////////// Free node ///////////////////////////////////////////
    (ptr->Prev)->NextNode = list->Tail;
    (list->Tail)->Prev = ptr->Prev;
    free(ptr);
  }

  return;
}
//End List_Delete


////////////////////////////////////////////////////////////////////////////
// Sorting                                                                //
// ====================================================================== //
// Input: List *list 		 -> pointer of List                               //
//      : Node *NewNode  -> maximum size array 	                          //
// Output: void                                                           //
// Purpose: Get linkedlist and sorting in ascending order by string				//
//          Head and Tail nodes are dummy node!!!!!                       //
////////////////////////////////////////////////////////////////////////////
void Sorting(List *list, Node *NewNode){ // a b c d...
  Node		*ptr; 			     // Pointing nodes
	char 		*str1, *str2;		 // Dot compare variable
////////////////////////// Head is NULL ////////////////////////////////////////
	if(list->Head->NextNode == list->Tail){
    NewNode->NextNode = list->Tail;
    NewNode->Prev = list->Head;
    list->Head->NextNode = NewNode;
    list->Tail->Prev = NewNode;
  }
//////////////////////////// Head is not NULL //////////////////////////////////
	else{
//////////////////////////////// Initialize pointers ///////////////////////////
		ptr = list->Head->NextNode;
///////////////////////// Sorting in ascending order ///////////////////////////
		while(ptr != list->Tail){
			str1 = ((DInfo*)ptr->Data)->filename;
			str2 = ((DInfo*)NewNode->Data)->filename;

			if(DotCompare(str1, str2) > 0){ // str1 is bigger than str2
				if(ptr == list->Head->NextNode){ // Insert before head
					NewNode->NextNode = ptr;
          NewNode->Prev = list->Head;
          list->Head->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				else{ // Insert middle of nodes
					NewNode->NextNode = ptr;
          NewNode->Prev = ptr->Prev;
          (ptr->Prev)->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				break;
			}
			else ptr = ptr->NextNode; // Move to nextnode
		}

//////////////////////////// Input newnode before Tail /////////////////////////
		if(ptr == list->Tail){
      NewNode->NextNode = list->Tail;
      NewNode->Prev = (list->Tail)->Prev;
      ((list->Tail)->Prev)->NextNode = NewNode;
      (list->Tail)->Prev = NewNode;
		}
	}
	list->size++; // Increase list size
	return ;
}
//End Sorting


////////////////////////////////////////////////////////////////////////////
// SortingBySize                                                          //
// ====================================================================== //
// Input: List *list 		 -> pointer of List                               //
//      : Node *NewNode  -> maximum size array 	                          //
// Output: void                                                           //
// Purpose: Get linkedlist and sorting in descending order by filesize		//
//          if size equals, sorting ascending order by string             //
//          Head and Tail nodes are dummy node!!!!!                       //
////////////////////////////////////////////////////////////////////////////
void SortingBySize(List *list, Node *NewNode){ // 100 99 98 ... / a b c d...
  Node		*ptr; 			           // Pointing nodes
  int     size1 = 0, size2 = 0;  // Size variable
  char    *str1, *str2;          // Dot compare variable
////////////////////////// Head is NULL ////////////////////////////////////////
	if(list->Head->NextNode == list->Tail){ // Head is NULL
    NewNode->NextNode = list->Tail;
    NewNode->Prev = list->Head;
    list->Head->NextNode = NewNode;
    list->Tail->Prev = NewNode;
  }
//////////////////////////// Head is not NULL //////////////////////////////////
	else{
//////////////////////////////// Initialize pointers ///////////////////////////
		ptr = list->Head->NextNode;
//////////////////////// Sorting in descending order ///////////////////////////
		while(ptr != list->Tail){

      size1 = ((DInfo*)ptr->Data)->filesize;
      size2 = ((DInfo*)NewNode->Data)->filesize;
			if(size1 < size2){ // ptr is smaller than newnode
				if(ptr == list->Head->NextNode){ // Insert before head
					NewNode->NextNode = ptr;
          NewNode->Prev = list->Head;
          list->Head->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				else{ // Insert middle of nodes
					NewNode->NextNode = ptr;
          NewNode->Prev = ptr->Prev;
          (ptr->Prev)->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				break;
			}
      else if(size1 == size2){ // size equal  -> a b c d...
        str1 = ((DInfo*)ptr->Data)->filename;
        str2 = ((DInfo*)NewNode->Data)->filename;

        if(DotCompare(str1, str2) > 0){ // ptr is bigger than newnode
          if(ptr == list->Head->NextNode){ // Insert before head
            NewNode->NextNode = ptr;
            NewNode->Prev = list->Head;
            list->Head->NextNode = NewNode;
            ptr->Prev = NewNode;
          }
          else{ // Insert middle of nodes
            NewNode->NextNode = ptr;
            NewNode->Prev = ptr->Prev;
            (ptr->Prev)->NextNode = NewNode;
            ptr->Prev = NewNode;
          }

        }
        else{ // newnode is bigger than ptr
//////////////////// Insert middle of nodes ////////////////////////////////////
          NewNode->NextNode = ptr->NextNode;
          NewNode->Prev = ptr;
          (ptr->NextNode)->Prev = NewNode;
          ptr->NextNode = NewNode;
        }
        break;
      }
      else  ptr = ptr->NextNode; // Move to nextnode
		}
//////////////////////////// Input newnode before Tail /////////////////////////
		if(ptr == list->Tail){
      NewNode->NextNode = list->Tail;
      NewNode->Prev = (list->Tail)->Prev;
      ((list->Tail)->Prev)->NextNode = NewNode;
      (list->Tail)->Prev = NewNode;
		}
	}
	list->size++; // Increase list size
	return ;
}
//End SortingBySize


////////////////////////////////////////////////////////////////////////////
// Sorting                                                                //
// ====================================================================== //
// Input: List *list 		 -> pointer of List                               //
//      : Node *NewNode  -> maximum size array 	                          //
// Output: void                                                           //
// Purpose: Get linkedlist and sorting in descending order by string			//
//          Head and Tail nodes are dummy node!!!!!                       //
////////////////////////////////////////////////////////////////////////////
void SortingReverse(List *list, Node *NewNode){ // d c b a...
  Node		*ptr; 			     // Pointing nodes
	char 		*str1, *str2;		 // Dot compare variable
////////////////////////// Head is NULL ////////////////////////////////////////
	if(list->Head->NextNode == list->Tail){
    NewNode->NextNode = list->Tail;
    NewNode->Prev = list->Head;
    list->Head->NextNode = NewNode;
    list->Tail->Prev = NewNode;
  }
//////////////////////////// Head is not NULL //////////////////////////////////
	else{
//////////////////////////////// Initialize pointers ///////////////////////////
		ptr = list->Head->NextNode;
///////////////////////// Sorting in descending order //////////////////////////
		while(ptr != list->Tail){
			str1 = ((DInfo*)ptr->Data)->filename;
			str2 = ((DInfo*)NewNode->Data)->filename;

			if(DotCompare(str1, str2) < 0){ // ptr is smaller than newnode
				if(ptr == list->Head->NextNode){ // Insert before head
					NewNode->NextNode = ptr;
          NewNode->Prev = list->Head;
          list->Head->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				else{ // Insert middle of nodes
					NewNode->NextNode = ptr;
          NewNode->Prev = ptr->Prev;
          (ptr->Prev)->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				break;
			}
			else ptr = ptr->NextNode; // Move to nextnode
		}

//////////////////////////// Input newnode before Tail /////////////////////////
		if(ptr == list->Tail){
      NewNode->NextNode = list->Tail;
      NewNode->Prev = (list->Tail)->Prev;
      ((list->Tail)->Prev)->NextNode = NewNode;
      (list->Tail)->Prev = NewNode;
		}
	}
	list->size++; // Increase list size
	return ;
}
//End SortingReverse


////////////////////////////////////////////////////////////////////////////
// SortingReverseBySize                                                   //
// ====================================================================== //
// Input: List *list 		 -> pointer of List                               //
//      : Node *NewNode  -> maximum size array 	                          //
// Output: void                                                           //
// Purpose: Get linkedlist and sorting in ascending order by filesize		  //
//          if size equals, sorting descending order by string            //
//          Head and Tail nodes are dummy node!!!!!                       //
////////////////////////////////////////////////////////////////////////////
void SortingReverseBySize(List *list, Node *NewNode){ // 1 2 3 4... / d c b a...
  Node		*ptr, *next; 	 // Pointing nodes
  int     size1 = 0, size2 = 0;  // Size variable
  char 		*str1, *str2;		       // Dot compare variable
////////////////////////// Head is NULL ////////////////////////////////////////
	if(list->Head->NextNode == list->Tail){ // Head is NULL
    NewNode->NextNode = list->Tail;
    NewNode->Prev = list->Head;
    list->Head->NextNode = NewNode;
    list->Tail->Prev = NewNode;
  }
//////////////////////////// Head is not NULL //////////////////////////////////
	else{
//////////////////////////////// Initialize pointers ///////////////////////////
		ptr = list->Head->NextNode;
//////////////////////// Sorting in ascending order ////////////////////////////
		while(ptr != list->Tail){
//////////////////////////////// Size variable /////////////////////////////////
      size1 = ((DInfo*)ptr->Data)->filesize;
      size2 = ((DInfo*)NewNode->Data)->filesize;
//////////////////////////////// Name variable /////////////////////////////////
      str1 = ((DInfo*)ptr->Data)->filename;
      str2 = ((DInfo*)NewNode->Data)->filename;

			if(size1 > size2){ // Newnode is smaller than ptr
				if(ptr == list->Head->NextNode){ // Insert before head
					NewNode->NextNode = ptr;
          NewNode->Prev = list->Head;
          list->Head->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				else{ // Insert middle of nodes
					NewNode->NextNode = ptr;
          NewNode->Prev = ptr->Prev;
          (ptr->Prev)->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				break;
			}
      else if(size1 == size2 && DotCompare(str1, str2) < 0){ // Size is equal and str1 is smaller than str2
        if(ptr == list->Head->NextNode){ // Insert before head
					NewNode->NextNode = ptr;
          NewNode->Prev = list->Head;
          list->Head->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				else{ // Insert middle of nodes
					NewNode->NextNode = ptr;
          NewNode->Prev = ptr->Prev;
          (ptr->Prev)->NextNode = NewNode;
          ptr->Prev = NewNode;
				}
				break;
      }
			else ptr = ptr->NextNode; // Move to nextnode
		}
//////////////////////////// Input newnode before Tail /////////////////////////
		if(ptr == list->Tail){
      NewNode->NextNode = list->Tail;
      NewNode->Prev = (list->Tail)->Prev;
      ((list->Tail)->Prev)->NextNode = NewNode;
      (list->Tail)->Prev = NewNode;
		}
	}
	list->size++; // Increase list size
	return ;
}
//End SortingReverseBySize


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
//End DotCompare


////////////////////////////////////////////////////////////
// PrintInfo                                              //
// ====================================================== //
// Input: List *list    -> pointer of List                //
//      : char c        -> Option check character         //
// Output: void                                           //
// Purpose: Check options and print each option's format  //
////////////////////////////////////////////////////////////
void    PrintInfo(List *list, char c, FILE *file){
  int       i = 0;
  Node      *ptr;
  char       *name;
/////////////////////////////////// ERROR check ////////////////////////////////
  if(list->size == 0){
     printf("Empty\n");
     return ;
  }
//////////////////////////////////// Print /////////////////////////////////////
  switch(c){
/////////////////////////////////// -l option //////////////////////////////////
    case 'l':
                ptr = list->Head->NextNode;
                while(ptr != list->Tail){
                  if(!strncmp(((DInfo*)ptr->Data)->permission, "d", 1)){
                    fputs("<tr><td style=\"color:blue\">", file);
                    fputs("<a href=\"", file);
                    fputs(((DInfo*)ptr->Data)->filename, file);
                    fputs("\">", file);
printf("filename : %s\n", ((DInfo*)ptr->Data)->filename);
                    name = Tokenizer(((DInfo*)ptr->Data)->filename);
printf("name : %s\n", name);
                    fputs(name, file);
                    fputs("</a>", file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(((DInfo*)ptr->Data)->permission, file);
                    fputs("</td>\n", file);
                    char link[256] = {0};
                    sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(link, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(((DInfo*)ptr->Data)->userid, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(((DInfo*)ptr->Data)->groupid, file);
                    fputs("</td>\n", file);
                    char size[256] = {0};
                    sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(size, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:blue\">", file);
                    char date[256] = {0};
                    sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                    fputs(date, file);
                    fputs("</td></tr>", file);
                  }
                  else if(!strncmp(((DInfo*)ptr->Data)->permission, "l", 1)){
                    fputs("<tr><td style=\"color:green\">", file);
                    fputs("<a href=\"", file);
                    fputs(((DInfo*)ptr->Data)->filename, file);
                    fputs("\">", file);
                    name = Tokenizer(((DInfo*)ptr->Data)->filename);
                    fputs(name, file);
                    fputs("</a>", file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:green\">", file);
                    fputs(((DInfo*)ptr->Data)->permission, file);
                    fputs("</td>\n", file);
                    char link[256] = {0};
                    sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                    fputs("<td style=\"color:green\">", file);
                    fputs(link, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:green\">", file);
                    fputs(((DInfo*)ptr->Data)->userid, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:green\">", file);
                    fputs(((DInfo*)ptr->Data)->groupid, file);
                    fputs("</td>\n", file);
                    char size[256] = {0};
                    sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                    fputs("<td style=\"color:green\">", file);
                    fputs(size, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:green\">", file);
                    char date[256] = {0};
                    sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                    fputs(date, file);
                    fputs("</td></tr>", file);
                  }
                  else{
                    fputs("<tr><td style=\"color:red\">", file);
                    fputs("<a href=\"", file);
                    fputs(((DInfo*)ptr->Data)->filename, file);
                    fputs("\">", file);
                    name = Tokenizer(((DInfo*)ptr->Data)->filename);
                    fputs(name, file);
                    fputs("</a>", file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:red\">", file);
                    fputs(((DInfo*)ptr->Data)->permission, file);
                    fputs("</td>\n", file);
                    char link[256] = {0};
                    sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                    fputs("<td style=\"color:red\">", file);
                    fputs(link, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:red\">", file);
                    fputs(((DInfo*)ptr->Data)->userid, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:red\">", file);
                    fputs(((DInfo*)ptr->Data)->groupid, file);
                    fputs("</td>\n", file);
                    char size[256] = {0};
                    sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                    fputs("<td style=\"color:red\">", file);
                    fputs(size, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:red\">", file);
                    char date[256] = {0};
                    sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                    fputs(date, file);
                    fputs("</td></tr>", file);
                  }
                  ptr = ptr->NextNode;
                }
                break;
////////////////////////////////// non option //////////////////////////////////
    case 'n':
                ptr = list->Head->NextNode;
                while(ptr != list->Tail){
                  if(!strncmp(((DInfo*)ptr->Data)->permission, "d", 1)){
                    fputs("<tr><td style=\"color:blue\">", file);
                    fputs("<a href=\"", file);
                    fputs(((DInfo*)ptr->Data)->filename, file);
                    fputs("\">", file);
                    name = Tokenizer(((DInfo*)ptr->Data)->filename);
                    fputs(name, file);
                    fputs("</a>", file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(((DInfo*)ptr->Data)->permission, file);
                    fputs("</td>\n", file);
                    char link[256] = {0};
                    sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(link, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(((DInfo*)ptr->Data)->userid, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(((DInfo*)ptr->Data)->groupid, file);
                    fputs("</td>\n", file);
                    char size[256] = {0};
                    sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                    fputs("<td style=\"color:blue\">", file);
                    fputs(size, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:blue\">", file);
                    char date[256] = {0};
                    sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                    fputs(date, file);
                    fputs("</td></tr>", file);
                  }
                  else if(!strncmp(((DInfo*)ptr->Data)->permission, "l", 1)){
                    if(!strcmp(((DInfo*)ptr->Data)->filename,"html_ls.html")){
                      ptr = ptr->NextNode;
                      continue;
                    }
                    fputs("<tr><td style=\"color:green\">", file);
                    fputs("<a href=\"", file);
                    fputs(((DInfo*)ptr->Data)->filename, file);
                    fputs("\">", file);
                    name = Tokenizer(((DInfo*)ptr->Data)->filename);
                    fputs(name, file);
                    fputs("</a>", file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:green\">", file);
                    fputs(((DInfo*)ptr->Data)->permission, file);
                    fputs("</td>\n", file);
                    char link[256] = {0};
                    sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                    fputs("<td style=\"color:green\">", file);
                    fputs(link, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:green\">", file);
                    fputs(((DInfo*)ptr->Data)->userid, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:green\">", file);
                    fputs(((DInfo*)ptr->Data)->groupid, file);
                    fputs("</td>\n", file);
                    char size[256] = {0};
                    sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                    fputs("<td style=\"color:green\">", file);
                    fputs(size, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:green\">", file);
                    char date[256] = {0};
                    sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                    fputs(date, file);
                    fputs("</td></tr>", file);
                  }
                  else{
                    if(!strcmp(((DInfo*)ptr->Data)->filename,"html_ls.html")){
                      ptr = ptr->NextNode;
                      continue;
                    }
                    fputs("<tr><td style=\"color:red\">", file);
                    fputs("<a href=\"", file);
                    fputs(((DInfo*)ptr->Data)->filename, file);
                    fputs("\">", file);
                    name = Tokenizer(((DInfo*)ptr->Data)->filename);
                    fputs(name, file);
                    fputs("</a>", file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:red\">", file);
                    fputs(((DInfo*)ptr->Data)->permission, file);
                    fputs("</td>\n", file);
                    char link[256] = {0};
                    sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                    fputs("<td style=\"color:red\">", file);
                    fputs(link, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:red\">", file);
                    fputs(((DInfo*)ptr->Data)->userid, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:red\">", file);
                    fputs(((DInfo*)ptr->Data)->groupid, file);
                    fputs("</td>\n", file);
                    char size[256] = {0};
                    sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                    fputs("<td style=\"color:red\">", file);
                    fputs(size, file);
                    fputs("</td>\n", file);
                    fputs("<td style=\"color:red\">", file);
                    char date[256] = {0};
                    sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                    fputs(date, file);
                    fputs("</td></tr>", file);
                  }
                  ptr = ptr->NextNode;
                }
              break;
/////////////////////////////////// -h option //////////////////////////////////
    case 'h':
                  ptr = list->Head->NextNode;
                    while(ptr != list->Tail){
                      if(!strncmp(((DInfo*)ptr->Data)->permission, "d", 1)){
                        fputs("<tr><td style=\"color:blue\">", file);
                        fputs("<a href=\"", file);
                        fputs(((DInfo*)ptr->Data)->filename, file);
                        fputs("\">", file);
                        name = Tokenizer(((DInfo*)ptr->Data)->filename);
                        fputs(name, file);
                        fputs("</a>", file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:blue\">", file);
                        fputs(((DInfo*)ptr->Data)->permission, file);
                        fputs("</td>\n", file);
                        char link[256] = {0};
                        sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                        fputs("<td style=\"color:blue\">", file);
                        fputs(link, file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:blue\">", file);
                        fputs(((DInfo*)ptr->Data)->userid, file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:blue\">", file);
                        fputs(((DInfo*)ptr->Data)->groupid, file);
                        fputs("</td>\n", file);
                        char size[256] = {0};
                        sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                        fputs("<td style=\"color:blue\">", file);
                        fputs(size, file);
                        fputs("K", file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:blue\">", file);
                        char date[256] = {0};
                        sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                        fputs(date, file);
                        fputs("</td></tr>", file);
                      }
                      else if(!strncmp(((DInfo*)ptr->Data)->permission, "l", 1)){
                        if(!strcmp(((DInfo*)ptr->Data)->filename,"html_ls.html")){
                          ptr = ptr->NextNode;
                          continue;
                        }
                        fputs("<tr><td style=\"color:green\">", file);
                        fputs("<a href=\"", file);
                        fputs(((DInfo*)ptr->Data)->filename, file);
                        fputs("\">", file);
                        name = Tokenizer(((DInfo*)ptr->Data)->filename);
                        fputs(name, file);
                        fputs("</a>", file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:green\">", file);
                        fputs(((DInfo*)ptr->Data)->permission, file);
                        fputs("</td>\n", file);
                        char link[256] = {0};
                        sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                        fputs("<td style=\"color:green\">", file);
                        fputs(link, file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:green\">", file);
                        fputs(((DInfo*)ptr->Data)->userid, file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:green\">", file);
                        fputs(((DInfo*)ptr->Data)->groupid, file);
                        fputs("</td>\n", file);
                        char size[256] = {0};
                        sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                        fputs("<td style=\"color:green\">", file);
                        fputs(size, file);
                        fputs("K", file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:green\">", file);
                        char date[256] = {0};
                        sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                        fputs(date, file);
                        fputs("</td></tr>", file);
                      }
                      else{
                        if(!strcmp(((DInfo*)ptr->Data)->filename,"html_ls.html")){
                          ptr = ptr->NextNode;
                          continue;
                        }
                        fputs("<tr><td style=\"color:red\">", file);
                        fputs("<a href=\"", file);
                        fputs(((DInfo*)ptr->Data)->filename, file);
                        fputs("\">", file);
                        name = Tokenizer(((DInfo*)ptr->Data)->filename);
                        fputs(name, file);
                        fputs("</a>", file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:red\">", file);
                        fputs(((DInfo*)ptr->Data)->permission, file);
                        fputs("</td>\n", file);
                        char link[256] = {0};
                        sprintf(link, "%d", ((DInfo*)ptr->Data)->numoflink);
                        fputs("<td style=\"color:red\">", file);
                        fputs(link, file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:red\">", file);
                        fputs(((DInfo*)ptr->Data)->userid, file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:red\">", file);
                        fputs(((DInfo*)ptr->Data)->groupid, file);
                        fputs("</td>\n", file);
                        char size[256] = {0};
                        sprintf(size, "%d", ((DInfo*)ptr->Data)->filesize);
                        fputs("<td style=\"color:red\">", file);
                        fputs(size, file);
                        fputs("K", file);
                        fputs("</td>\n", file);
                        fputs("<td style=\"color:red\">", file);
                        char date[256] = {0};
                        sprintf(date, "%s %d %d %d", ((DInfo*)ptr->Data)->date_str, ((DInfo*)ptr->Data)->date_int[0], ((DInfo*)ptr->Data)->time[0], ((DInfo*)ptr->Data)->time[1]);
                        fputs(date, file);
                        fputs("</td></tr>", file);
                      }
                      ptr = ptr->NextNode;
                    }
                    break;
  }
}
//End PrintInfo


////////////////////////////////////////////////////////////
// IsDir                                                  //
// ====================================================== //
// Input: char *Dirname    -> file or directory name      //
// Output: int  1    -> input is '*'                      //
//         int count -> input is /home/~~                 //
//         int 0     -> default                           //
// Purpose: Check input Dirname is file or Directory also //
//          change and return argv in this function       //
////////////////////////////////////////////////////////////
int IsDir(char *Dirname){
  const char          *check = "*";     // check * input variable
  char                *tok;             // tokenizer variable
  int                 count = 0;        // number of /
  char                eli = '*';        // get rid of * in /home/~~~
  char                Temp[255] = "";   // save Dirname
////////////////////////// Making temp /////////////////////////////////////////
  strcpy(Temp, Dirname);
////////////////////////// Check * input ///////////////////////////////////////
  if(strcmp(Dirname, check) == 0){
    strcpy(Dirname, ".");
    return 4;
  }
//////////////////// Dir is current Dir ////////////////////////////////////////
  else if(strcmp(Dirname, ".") == 0) return 1;
//////////////////////// Dir is home ///////////////////////////////////////////
  else if(!fnmatch("/home/*",Dirname,0)){
////////////////////////////// tokenizing //////////////////////////////////////
    tok = strtok(Temp, "/");
    while(tok != NULL){
      tok =strtok(NULL, "/");
      count++;
    }
printf("before DIr : %s\n", Dirname);
//////////////////////// tokenize * in /home/~~ dir ////////////////////////////
    for (; *Dirname != '\0'; Dirname++){
      if (*Dirname == eli){
        strcpy(Dirname, Dirname + 1);
        Dirname--;
      }
    }
  printf("after DIr : %s\n", Dirname);
////////////////////////////// return count ////////////////////////////////////
    return count; // 3
  }
  else if(!chdir(Dirname)){
    return 2;
  }
  else return 0; // default return
}
//End IsDir



char* Tokenizer(char *path){
  char                filename[255] = "";   // save Filename
  char                tk = '/';             // tokenizer
  int                 count = 0;
////////////////////////// Making temp /////////////////////////////////////////

if(!strncmp(path, "/", 1)){
  for (; *path != '\0'; path++){
    if (*path == tk){
      strcpy(filename, path + 1);
      strcpy(path, path + 1);
      path--;
      count++;
    }
  }
}
else if(!strncmp(path, "./", 2)){
  for (; *path != '\0'; path++){
    if (*path == tk){
      strcpy(filename, path + 1);
      strcpy(path, path + 1);
      path--;
      count++;
    }
  }
}
//  memcpy(path, filename, sizeof(filename));
  if(count){
    strcpy(path, filename);
      return path;
  }
  else  return path;

}
//End Tokenizer


//////////////////////////////////////////////////////////////////////////////
// ls_function                                                              //
// ======================================================================== //
// Input : int Check                 -> in paramter, path is exist or not   //
//       : int aflag                 -> a option flag                       //
//       : int lflag                 -> l option flag                       //
//       : int hflag                 -> h option flag                       //
//       : int sflag                 -> S option flag                       //
//       : int rflag                 -> r option flag                       //
//       : int argc                  -> number of argument                  //
//       : char **argv               -> argument array                      //
// Output: int  0 default                                                   //
//       : int -1 ERROR                                                     //
// Purpose: Get flags and argv to check is dir or file or error input.      //
//          process all functions here                                      //
//////////////////////////////////////////////////////////////////////////////
int ls_function(int Check, int aflag, int lflag, int hflag, int sflag, int rflag, int argc, char** argv, FILE *file){
  DIR             *dirp;          // DIR struct's variable for opendir
  struct dirent   *dir;           // dirent struct's variable for readdir
  char            Dirname[255];   // temporary variable for use pathname
  List            *list;          // List struct's variable
  char            Abspath[255];   // Get absolute path
  char            path[255];      // Temporary variable for use pathname
  struct stat     buf;            // stat variable
  DIR             *dirp2;         // DIR struct's variable for use in dirp
  struct dirent   *dir2;          // dirent struct's variable for us in dir

/////////////////////////////// Initialize list ////////////////////////////////
  list = (List*)malloc(sizeof(List));
  List_Init(list);
///////////////////////////// -alhSr option ////////////////////////////////////
  if(lflag && aflag && hflag && sflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{

        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
               // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'h', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0;  // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'q')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'h', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0;  // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -alhS option //////////////////////////////////
  else if(lflag && aflag && hflag && sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'h', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 's')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
        // html title
                  fputs("<h4>Directory path : ", file);
                  fputs(Dirname, file);
                  fputs("</h4>", file);
                  fputs("<h4>total ", file);
                  char total[5] = {0};
                  sprintf(total, "%d", Totalsize);
                  fputs(total, file);
                  fputs("</h4>", file);
        // html title

        // chart
                  fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
                  PrintInfo(list, 'h', file); // Print
                  fputs("</table>\n", file);
        // chart
                  Totalsize = 0; // Set blocksize as 0
                  List_Delete(list); // List clear
                  closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -alhr option //////////////////////////////////
  else if(lflag && aflag && hflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'h', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'r')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'h', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -alSr option //////////////////////////////////
  else if(lflag && aflag && sflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'l', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'q')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'h', file); // Print
        fputs("</table>\n", file);
// chart
        printf("Directory path : %s\n", Abspath); // Print abspath
        printf("total : %d\n", Totalsize/2); // Print blocksize
        Totalsize = 0; // Set blocksize as 0
        PrintInfo(list, 'l', file); // Print
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -lhSr option //////////////////////////////////
  else if(lflag && hflag && sflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'h', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'q')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'q')) == -1){ // List inser
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'h', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -ahSr option //////////////////////////////////
  else if(aflag && hflag && sflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; //chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'q')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -alh option ///////////////////////////////////
  else if(lflag && aflag && hflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'h', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'n')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'h', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -alS option ///////////////////////////////////
  else if(lflag && aflag && sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'l', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 's')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'l', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -alr option ///////////////////////////////////
  else if(lflag && aflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'l', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'r')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; //chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'l', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -ahS option ///////////////////////////////////
  else if(aflag && hflag && sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);

// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path

        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 's')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -ahr option ///////////////////////////////////
  else if(aflag && hflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path

        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'r')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -aSr option ///////////////////////////////////
  else if(aflag && sflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path

        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'q')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -lhS option ///////////////////////////////////
  else if(lflag && hflag && sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // getcwd ERROR
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'h', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0;  // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 's')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . file
            if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'h', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -lhr option ///////////////////////////////////
  else if(lflag && hflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'h', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path

        if((dirp = opendir(Dirname)) == NULL){ // No dir
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'r')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'h', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -lSr option ///////////////////////////////////
  else if(lflag && sflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'l', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0;  // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'q')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'l', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -hrS option ///////////////////////////////////
  else if(hflag && sflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
              // List insert ERROR
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'q')) == -1){ // List insert ERROR

            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
            if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
                // List insert ERROR
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -la option ////////////////////////////////////
  else if(aflag && lflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'l', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname)) printf("cannot access '%s' : No such file or directory\n", Dirname);
          else if((List_Insert(list, Dirname, 'n')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            strcat(Abspath, "/");
            strcat(Abspath, dir->d_name);
            if((List_Insert(list, Abspath, 'n')) == -1){ // List insert
              return 0;
            }
            strcpy(Abspath, Dirname);
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'l', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -lh option ////////////////////////////////////
  else if(lflag && hflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; //chdir RROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'h', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'n')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'h', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -lS option ////////////////////////////////////
  else if(lflag && sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; //chdir RROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'l', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 's')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'l', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -lr option ////////////////////////////////////
  else if(lflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'l', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'r')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'l', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -ar option ////////////////////////////////////
  else if(aflag && rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Saving file datas
          if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'r')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -ah option ////////////////////////////////////
  else if(aflag && hflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Saving file datas
          if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'n')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -aS option ////////////////////////////////////

  else if(aflag && sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 's')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -rh option ////////////////////////////////////
  else if(rflag && hflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'r')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -rS option ////////////////////////////////////
  else if(rflag && sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
          if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'q')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'q')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -hS option ////////////////////////////////////
  else if(hflag && sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
          if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 's')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -l option /////////////////////////////////////
  else if(lflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'l', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'n')) == -1){ // List insert
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'l', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -a option /////////////////////////////////////
  else if(aflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'n')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -S option /////////////////////////////////////
  else if(sflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
          if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 's')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 's')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -h option /////////////////////////////////////
  else if(hflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        if(chdir(Dirname) == -1) return -1; // chdir ERROR
        if(getcwd(Abspath, 255) == NULL) return -1; // getcwd ERROR
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

          if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'n')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          if(chdir(Dirname) == -1) return -1; // chdir ERROR
          if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// -r option /////////////////////////////////////
  else if(rflag){
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
          if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
      fputs("<h4>Directory path : ", file);
      fputs(Dirname, file);
      fputs("</h4>", file);
      fputs("<h4>total ", file);
      char total[5] = {0};
      sprintf(total, "%d", Totalsize);
      fputs(total, file);
      fputs("</h4>", file);
// html title

// chart
      fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
      PrintInfo(list, 'n', file); // Print
      fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        strcpy(Dirname, argv[optind]); // Set path
/////////////////////////// Input is not dir ///////////////////////////////////
        if((dirp = opendir(Dirname)) == NULL){
          if(IsDir(Dirname))  printf("cannot access '%s' : No such file or directory\n", Dirname);// Wrong file check
          else if((List_Insert(list, Dirname, 'r')) == -1){ // List insert ERROR
            return 0;
          }
        }
///////////////////////////// Input is dir /////////////////////////////////////
        else{
          while((dir = readdir(dirp)) != NULL){
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

            if((List_Insert(list, dir->d_name, 'r')) == -1){ // List insert
              return 0;
            }
          }
        }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
        Totalsize = 0; // Set blocksize as 0
        List_Delete(list); // List clear
        closedir(dirp); // Close dir
      }
    }
    return 0;
  }
//////////////////////////////// No option /////////////////////////////////////
  else{
///////////////////////////// NO path input ////////////////////////////////////
    if(Check){
      strcpy(Dirname, "."); // Set path
      if((dirp = opendir(Dirname)) == NULL) return 0;
      else{
        while((dir = readdir(dirp)) != NULL){ // Read directory
          if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
          if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
            return 0;
          }
        }
      }
// html title
        fputs("<h4>Directory path : ", file);
        fputs(Dirname, file);
        fputs("</h4>", file);
        fputs("<h4>total ", file);
        char total[5] = {0};
        sprintf(total, "%d", Totalsize);
        fputs(total, file);
        fputs("</h4>", file);
// html title

// chart
        fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
        PrintInfo(list, 'n', file); // Print
        fputs("</table>\n", file);
// chart
      Totalsize = 0; // Set blocksize as 0
      List_Delete(list); // List clear
      closedir(dirp); // Close dir
    }
////////////////////////// YES path input //////////////////////////////////////
    else if(!Check){
      int rt = 0;
      for(optind; optind < argc; optind++){ // Iterate the number of paths
        if(rt = IsDir(argv[optind])){ // Input is Dir
          if(rt == 1){
            strcpy(Dirname, argv[optind]); // Set path
    /////////////////////////// Input is not dir ///////////////////////////////////
            if((dirp = opendir(Dirname)) == NULL){
              if(IsDir(Dirname)) printf("cannot access '%s' : No such file or directory\n", Dirname);
              else if((List_Insert(list, Dirname, 'n')) == -1){ // List insert ERROR

                return 0;
              }
            }
    ///////////////////////////// Input is dir /////////////////////////////////////
            else{
              if(chdir(Dirname) == -1) return -1; // chdir ERROR
              if(getcwd(Abspath,255) == NULL) return -1; // getcwd ERROR
              while((dir = readdir(dirp)) != NULL){ // Read directory
                if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
                  return 0;
                }
              }
            }
    // html title
            fputs("<h4>Directory path : ", file);
            fputs(Dirname, file);
            fputs("</h4>", file);
            fputs("<h4>total ", file);
            char total[5] = {0};
            sprintf(total, "%d", Totalsize);
            fputs(total, file);
            fputs("</h4>", file);
    // html title

    // chart
            fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
            PrintInfo(list, 'l', file); // Print
            fputs("</table>\n", file);
    // chart
            Totalsize = 0; // Set blocksize as 0
            List_Delete(list); // List clear
            closedir(dirp); // Close dir
          }
          else if(rt == 2){ // ~ input
            if((dirp = opendir(argv[optind])) == NULL) return 0;
            while((dir = readdir(dirp)) != NULL){ // Read directory
              if(!strncmp(dir->d_name,".",1))   continue; // Remove . files

              if(!lstat(dir->d_name, &buf)){ // Get file buf
                if(S_ISDIR(buf.st_mode)) continue; // Check file is Dir
              }
              if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
                return 0;
              }
            }
// html title
            fputs("<h4>Directory path : ", file);
            fputs(argv[optind], file);
            fputs("</h4>", file);
            fputs("<h4>total ", file);
            char total[5] = {0};
            sprintf(total, "%d", Totalsize);
            fputs(total, file);
            fputs("</h4>", file);
// html title

// chart
            fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
            PrintInfo(list, 'n', file); // Print
            fputs("</table>\n", file);
// chart
            Totalsize = 0; // Set blocksize as 0
            List_Delete(list); // List clear
            closedir(dirp); // Close Dir
            continue; // Skip
          }
          else if(rt == 3){ // /home/~ input
////////////////////////// File print part /////////////////////////////////////
            if((dirp = opendir(argv[optind])) == NULL) return 0;
            while((dir = readdir(dirp)) != NULL){ // Read directory
              if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
              if(!lstat(dir->d_name, &buf)){ // Get file buf
                if(!S_ISDIR(buf.st_mode)){ // Input is not Dir
                  if((List_Insert(list, dir->d_name, 'n')) == -1){ // List insert
                    return 0;
                  }
                }
              }
            }
// html title
            fputs("<h4>Directory path : ", file);
            fputs(argv[optind], file);
            fputs("</h4>", file);
            fputs("<h4>total ", file);
            char total[5] = {0};
            sprintf(total, "%d", Totalsize);
            fputs(total, file);
            fputs("</h4>", file);
// html title

// chart
            fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
            PrintInfo(list, 'n', file); // Print
            fputs("</table>\n", file);
// chart
            Totalsize = 0; // Set blocksize as 0
            List_Delete(list); // List clear
////////////////////////// Dir print part //////////////////////////////////////
            rewinddir(dirp); // Reread
            while((dir = readdir(dirp)) != NULL){ // Saving file datas
              if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
              if(!lstat(dir->d_name, &buf)){ // Get file buf
                if(S_ISDIR(buf.st_mode)){ // Input is Dir
                  strcpy(path, argv[optind]); // Copy argv
                  strcat(path, dir->d_name); // Copy dir name

                  if((dirp2 = opendir(path)) == NULL) return 0;
                  while((dir2 = readdir(dirp2)) != NULL){ // Read directory
                    if(!strncmp(dir2->d_name,".",1))   continue; // Remove . files
                    char pathtemp[255];
                    getcwd(pathtemp, 255);
                    chdir(path);
                    if((List_Insert(list, dir2->d_name, 'n')) == -1) // List insert
                      return 0;
                    chdir(pathtemp);
                  }
// html title
                  fputs("<h4>Directory path : ", file);
                  fputs(path, file);
                  fputs("</h4>", file);
                  fputs("<h4>total ", file);
                  char total[5] = {0};
                  sprintf(total, "%d", Totalsize);
                  fputs(total, file);
                  fputs("</h4>", file);
// html title
// chart
                  fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
                  PrintInfo(list, 'n', file); // Print
                  fputs("</table>\n", file);
                  List_Delete(list);
// chart
                  closedir(dirp2); // Close Dir
                }
              }
            }
          }
          else if(rt == 4){ // * input
            if((dirp = opendir(argv[optind])) == NULL) return 0;
            while((dir = readdir(dirp)) != NULL){ // Read directory
              if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
              if(!lstat(dir->d_name, &buf)){ // Get file buf
                if(!S_ISDIR(buf.st_mode)) continue; // If not dir, skip
              }
              printf("Directory path : %s\n", dir->d_name); // Print dir path
              strcpy(path,argv[optind]); // set path as argv
              strcat(path, "/"); // Copy / to path
              strcat(path, dir->d_name); // Copy filename to path
              if((dirp2 = opendir(path)) == NULL){
                printf("opendir error\n");
                return 0;
              }
              while((dir2 = readdir(dirp2)) != NULL){ // Read directory
                if(!strncmp(dir2->d_name,".",1))   continue; // Remove . files
                printf("%s\n", dir2->d_name); // Print filenames
              }
              closedir(dirp2); // CLose Dir
              printf("\n"); // New line
            }
            closedir(dirp); // Close dir
          }
          else if(rt == 2){ // Not wildcard input
            printf("dd\n");
          }
        }
        else{ // Not Dir
          if((dirp = opendir(".")) == NULL) return 0;
          while((dir = readdir(dirp)) != NULL){ // Read directory
            if(!strncmp(dir->d_name,".",1))   continue; // Remove . files
            if(!fnmatch(argv[optind], dir->d_name, 0)){ // Wildcard searching
              List_Insert(list, dir->d_name, 'n'); // List insert
            }
            else continue; // Not wildcard
          }
          if(list->size == 0){ // List size error
              printf("cannot access '%s' : No such file or directory\n", argv[optind]);// Wrong file check
              return 0;
          }
          else{
// html title
            fputs("<h4>Directory path : ", file);
            fputs(Dirname, file);
            fputs("</h4>", file);
            fputs("<h4>total ", file);
            char total[5] = {0};
            sprintf(total, "%d", Totalsize);
            fputs(total, file);
            fputs("</h4>", file);
// html title

// chart
            fputs("<table border=\"1\">\n<tr><th>Name</th><th>Permission</th><th>Link</th><th>Owner</th><th>Group</th><th>Size</th><th>Last Modified</th></tr>\n", file);
            PrintInfo(list, 'n', file); // Print
            fputs("</table>\n", file);
// chart
            Totalsize = 0; // Set blocksize as 0
            List_Delete(list); // List Clear
            closedir(dirp); // Close dir
          }
        }
      }
    }
  }
}
//End ls_function


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
// End CheckFilePermission
