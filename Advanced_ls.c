//////////////////////////////////////////////////////////////////
// File Name	: simple_ls.c					//
// Date		: 2019/04/04					//
// Os		: Ubuntu 16.04 64bits				//
// Author	: Park Jeong Yong				//
// Student ID	: 2016722098					//
// -------------------------------------------------------------//
// Title : System Programming Assignment #2-1 (ls function)	//
// Description : Making 'ls' function which automatically sort	//
// and do not print hidden files				//
//////////////////////////////////////////////////////////////////

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

//Dir information
typedef struct Dir_Info{
	char 					filename[255]; // File name
	char 					permission[11]; // File permission
	char					userid[20]; // User id
	char					groupid[20]; // Group id
	int 					numoflink; // Number of links
	int			 			filesize; // File size
	int			 			time[2]; // Time
	int 					date[2]; // Date
	char					symlink[4096]; // Symbolic link path
}DInfo;
//Dir information

typedef struct Node{
	void 					*Data;
	struct Node		*NextNode;
}Node;

typedef struct List{
	int 					size;
	Node					*Head;
	Node					*Tail;
}List;

void List_start(List *list);

int List_Insert(List *list, Node* node, const void *data);

void Sorting(List* list);

void CheckFilePermission(struct stat* buf,DInfo *dinfo);

int FileSize(const char Dirname[]);

void PrintInfo(List* list);

int ls_function(const char Dirname[], int Param);
////////////////////////////////////////////////////////////////////
// main  							  //
// ===============================================================//
// Input: int argc 	-> number of parameter			  //
// 	: char** argv -> name of directory  			  //
// Output: int 0 default					  //
// Purpose: Check direcory errors if no errors, run ls function	  //
////////////////////////////////////////////////////////////////////
int main(int argc, char **argv){
	char	pathname[255];
	int		  flag = 0;
	int 		c = 0;

	if(argc == 1){ // Just ls
		printf("Access to argc 1\n");
		flag = 0;
		if(ls_function(".", flag) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
	}
	else if(argc == 2){ // Only pathname
		while((c = getopt(argc, argv, "alc")) != -1)
		{
			switch(c)
			{
				case 'a' :
									flag = 1;
									strcpy(pathname,".");
									printf("Access to option a, pathname : %s\n", pathname);
									if(ls_function(pathname, flag) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
									break;
				case 'l' :
									flag = 2;
									strcpy(pathname,".");
									if(ls_function(pathname, flag) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
									break;
				case '?' :
									printf("Error\n");
									break;
				default  :
									printf("default\n");
									if(ls_function(".", flag) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
			}
		}
	}
	else if(argc >= 3){ // Options
		while((c = getopt(argc, argv, "a:l:c")) != -1)
		{
			switch(c)
			{
				case 'a' :
									flag = 1;
									strcpy(pathname,optarg);
									printf("Access to option a, pathname : %s\n", pathname);
									if(ls_function(pathname, flag) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
									break;
				case 'l' :
									flag = 2;
									strcpy(pathname,optarg);
									if(ls_function(pathname, flag) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
									break;
				case '?' :
										printf("Error\n");
										break;
				default  :
									printf("default\n");
									if(ls_function(".", flag) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
			}
		}
	}
	return 0;
}

// List start
void List_start(List *list){
	list->size = 0;
	list->Head = NULL;
	list->Tail = NULL;

	printf("List created\n");
	return ;
}
// List start


// List insert
int List_Insert(List *list, Node* node, const void *data){
		Node *NewNode;

		if((NewNode = (Node*)malloc(sizeof(Node))) == NULL)	return -1;

		NewNode->Data = (void*)data;

		if(node == NULL){
			if(list->size == 0)	list->Tail = NewNode;

			NewNode->NextNode = list->Head;
			list->Head = NewNode;
		}
		else{
			if(node->NextNode == NULL)	list->Tail = NewNode;

			NewNode->NextNode = node->NextNode;
			node->NextNode = NewNode;
		}

		list->size++;

		return 0;

}
// List insert


// Sorting
void Sorting(List* list){
	int i, j, strlen_1, strlen_2, min, cmp;
	int index;
	Node* node;
	void *tmp;

	for(i=list->size-1; i>0; i--){
		for(j=0, node = list->Head; j<1; j++, node= node->NextNode){
			strlen_1 = strlen(((DInfo*)node->Data)->filename);
			strlen_2 = strlen(((DInfo*)node->NextNode->Data)->filename);

			if(strlen_1 < strlen_2)	min = strlen_2;
			else	min = strlen_1;

			for(cmp = min, index = 0; cmp != 0; cmp--, index++){
				if(((DInfo*)node->Data)->filename[index] > ((DInfo*)node->NextNode->Data)->filename[index]){
					tmp = node->Data;
					node->Data = node->NextNode->Data;
					node->NextNode->Data = tmp;
					break;
				}
				else if(((DInfo*)node->Data)->filename[index] == ((DInfo*)node->NextNode->Data)->filename[index]){
					continue;
				}
				break;
			}
		}
	}
}
// Sorting



// File permission
void CheckFilePermission(struct stat* buf,DInfo* dinfo){
	// Check if it is Dir
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
	/*사용자 권한 검사*/
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
	else if(buf->st_mode & S_ISUID)
		dinfo->permission[3]='s';
	else
		dinfo->permission[3]='-';

	/*그룹 권한 검사*/
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
	else if(buf->st_mode & S_ISGID)
		dinfo->permission[6]='s';
	else
		dinfo->permission[6]='-';
	/*일반사용자 권한 검사*/
	if(buf->st_mode & S_IROTH)
		dinfo->permission[7]='r';
	else
		dinfo->permission[7]='-';
	if(buf->st_mode & S_IWOTH)
		dinfo->permission[8]='w';
	else
		dinfo->permission[8]='-';

	if(buf->st_mode & S_IXOTH) //stiky bit 설정
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

	dinfo->permission[10]='\0';
}


/*	Path를 입력한 그 디렉토리의 크기를 구하는 함수.		*/
int FileSize(const char Dirname[])
{
         struct dirent * dir;
         DIR * dirp;
         int sum=0;
         struct stat buf;
         char filename[255];

         dirp=opendir(Dirname);

         while((dir=readdir(dirp))!= NULL)
         {
		    strcpy(filename,Dirname);
		    strcat(filename,"/");
		    strcat(filename,dir->d_name);
		 if((lstat(filename,&buf))==0)  //lstat 성공시만 출력 for check
			 sum=sum+buf.st_blocks;
	 }
	 closedir(dirp);

	 return sum/2;
}


//Print list
void PrintInfo(List* list){
	int i;
	Node *ptr;

	if(list->size == 0){
		printf("Empty\n");
		return ;
	}

	for(i=0, ptr=list->Head; i<list->size; i++, ptr=ptr->NextNode){
		printf("%s", ((DInfo*)ptr->Data)->permission);
		printf(" %d", ((DInfo*)ptr->Data)->numoflink);
		printf(" %s", ((DInfo*)ptr->Data)->userid);
		printf(" %s", ((DInfo*)ptr->Data)->groupid);
		printf(" %d", ((DInfo*)ptr->Data)->filesize);
		printf(" %5d %2d",((DInfo*)ptr->Data)->date[0]
				 ,((DInfo*)ptr->Data)->date[1]);
		printf(" %02d:%02d",((DInfo*)ptr->Data)->time[0]
				,((DInfo*)ptr->Data)->time[1]);
		printf(" %s",((DInfo*)ptr->Data)->filename);
		printf("\n");
	}
}
//Print list


// File size
///////////////////////////////////////////////////////////////////////////////////
// ls_function							     		 //
// ==============================================================================//
// Input: const char** name -> name of directory				 //
//	  char** path -> Array for saving file names 				 //
// Output: int 0 default						         //
// Purpose: Read directory names and send filenames to Sorting function		 //
//	    and print lists							 //
///////////////////////////////////////////////////////////////////////////////////
int ls_function(const char Dirname[], int Param){
	DIR 							*dirp;
  struct dirent 		*dir;
	struct stat 			buf;
	struct group 			*grp;
	struct passwd 		*pwd;
	struct tm 				*time;
	Node 							*node=NULL;
	List 							list;
	DInfo 						*dinfo;
	char 							Filename[255];
	int 							Check = 0;
	int 							total = 0;

	List_start(&list);

	if((dirp = opendir(Dirname)) == NULL) return -1;

	printf("%s:\n", Dirname);

	total = FileSize(Dirname);
	printf("Sum %d\n", total); // Complete

	if(Param == 1){ // -a option
		while((dir = readdir(dirp))!=NULL){
			strcpy(Filename,Dirname);
			strcat(Filename, "/");
			strcat(Filename, dir->d_name);

			if((lstat(Filename, &buf) ==0)){
				dinfo = (DInfo*)malloc(sizeof(DInfo));
				pwd = getpwuid(buf.st_uid);
				grp = getgrgid(buf.st_gid);
				strcpy(dinfo->userid, pwd->pw_name);
				strcpy(dinfo->groupid, grp->gr_name);

				dinfo->numoflink = buf.st_nlink;
				dinfo->filesize = buf.st_size;

				time = localtime(&buf.st_mtime);

				dinfo->date[0]=(time->tm_mon)+1;
				dinfo->date[1]=time->tm_mday;
				dinfo->time[0]=time->tm_hour;
				dinfo->time[1]=time->tm_min;

				CheckFilePermission(&buf, dinfo);
				strcpy(dinfo->filename, dir->d_name);

				if((List_Insert(&list,node,dinfo))==-1)
				{
					printf("List insert Access\n");
					puts("list삽입실패");
					return 0;
				}
			}
		}
		Sorting(&list);
		PrintInfo(&list);
//		Free
//		Close
	}
	else if(Param == 2){ // -l option
		while((dir = readdir(dirp))!=NULL){
			if(!strcmp(dir->d_name,".") | !(strcmp(dir->d_name,"..")))	continue;
			strcpy(Filename,Dirname);
			strcat(Filename, "/");
			strcat(Filename, dir->d_name);

			if((lstat(Filename, &buf) ==0)){
				dinfo = (DInfo*)malloc(sizeof(DInfo));
				pwd = getpwuid(buf.st_uid);
				grp = getgrgid(buf.st_gid);
				strcpy(dinfo->userid, pwd->pw_name);
				strcpy(dinfo->groupid, grp->gr_name);

				dinfo->numoflink = buf.st_nlink;
				dinfo->filesize = buf.st_size;

				time = localtime(&buf.st_mtime);

				dinfo->date[0]=(time->tm_mon)+1;
				dinfo->date[1]=time->tm_mday;
				dinfo->time[0]=time->tm_hour;
				dinfo->time[1]=time->tm_min;

				CheckFilePermission(&buf, dinfo);
				strcpy(dinfo->filename, dir->d_name);
			}
		}
	}
	else if(Param == 0){
		while((dir = readdir(dirp))!=NULL){
			if(!strcmp(dir->d_name,".") | !(strcmp(dir->d_name,"..")))	continue;
				strcpy(Filename,Dirname);
				strcat(Filename, "/");
				strcat(Filename, dir->d_name);

				if((lstat(Filename, &buf) ==0)){
					dinfo = (DInfo*)malloc(sizeof(DInfo));
					strcpy(dinfo->filename, dir->d_name);
			}
		}
		Sorting(&list);
		PrintInfo(&list);
	}

	closedir(dirp);

	return 0;
}
