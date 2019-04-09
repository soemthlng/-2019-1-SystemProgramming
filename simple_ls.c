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

int i=0;

int ls_function(const char *name, char **path);
int Sorting(char **path);

////////////////////////////////////////////////////////////////////
// main  							  //
// ===============================================================//
// Input: int argc 	-> number of parameter			  //
// 	: char** argv -> name of directory  			  //
// Output: int 0 default					  //
// Purpose: Check direcory errors if no errors, run ls function	  //
////////////////////////////////////////////////////////////////////
int main(int argc, char **argv){
	char **path = NULL;
	char pathname[128];

	/////////////////Copy directory name and check errors///////////////
	if(argc == 1){
		strcpy(pathname,".");

	///////////////////////Call ls function/////////////////////////////
	if(ls_function(pathname, path) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
	/////////////////////End of call function///////////////////////////
	}
	else if(argc >= 3){
		printf("simple_ls: only one directory path can be processed\n");
	}
	else{
		strcpy(pathname, argv[1]);

	///////////////////////Call ls function/////////////////////////////
	if(ls_function(pathname, path) == -1) printf("simple_ls: cannot access '%s' : No such directory\n", pathname);
	/////////////////////End of call function///////////////////////////
	}
//////////////End copy directory name and check errors///////////////

	return 0;
}

////////////////////////////////////////////////////////////////////
// Sorting 							  //
// ===============================================================//
// Input: char** path -> name of files				  //
// Output: int 0 default					  //
// Purpose: Sorting filenames in ascending order		  //
////////////////////////////////////////////////////////////////////
int Sorting(char **path){
/////////////Declare temporary variable for sorting/////////////////
	char* temp = NULL;
////////////////End of declaring temporary variable/////////////////

	for(int a = 0; a < i-1; a++){
		for(int k = 0; k < i-1-a; k++){
			if(strcasecmp(path[k], path[k+1]) > 0){
				temp = path[k];
				path[k] = path[k+1];
				path[k+1] = temp;
			}
		}
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////
// ls_function							     		 //
// ==============================================================================//
// Input: const char** name -> name of directory				 //
//	  char** path -> Array for saving file names 				 //
// Output: int 0 default						         //
// Purpose: Read directory names and send filenames to Sorting function		 //
//	    and print lists							 //
///////////////////////////////////////////////////////////////////////////////////
int ls_function(const char *name, char **path){
	DIR *drip;
  struct dirent *dir;
	int c = 0;

///////////////////////Check end of directory///////////////////////////
	if((drip = opendir(name)) == NULL) return (-1);
///////////////////////Directory checking end///////////////////////////


////////////////////Count number of directories/////////////////////////
	while((dir = readdir(drip)) != NULL){
		if(!strcmp(dir->d_name,".") | !strcmp(dir->d_name,"..")){
			continue;
		}
		c++;
	}
////////////////////////////End of count///////////////////////////////


///////////////////////Malloc 'path' variable//////////////////////////
	path = (char **) malloc(sizeof(char *)*c);
///////////////////////////End of malloc///////////////////////////////


///////////////Read directory names and store in 'path'////////////////
	rewinddir(drip);

	while((dir = readdir(drip))!=NULL){
		if(!strcmp(dir->d_name,".") | !(strcmp(dir->d_name,".."))){
			continue;
		}
		path[i] = dir->d_name;
		i++;
		if(i == c) break;
	}
////////////////////End of reading directory names/////////////////////


//////////////////////Call Sorting function////////////////////////////
	Sorting(path);
//////////////////////End Sorting function/////////////////////////////


////////////////////////Print directories//////////////////////////////
	for (int j = 0; j < i; j++)	printf("%s\n", path[j]);
//////////////////////End print directories////////////////////////////

	closedir(drip);

	return 0;
}

///////////////////////////////////////////////////////////////////////
//Makefile																													 //
///////////////////////////////////////////////////////////////////////
//OBJS = simple_ls.c																							   //
//CC = gcc 																													 //
//EXEC  = simple_ls																						       //
//																																   //
//all: $(OBJS)																										   //
//	$(CC) -o $(EXEC) $^																							 //
//clean:																														 //
//	rm -rf $(EXEC)																									 //
///////////////////////////////////////////////////////////////////////
