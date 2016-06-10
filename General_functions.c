/* Author: Niketh Boreddy
   email: nikethcse@gmail.com
   
  Date: 10th June 2016
  The follwing file is a part of the LZO algortihm implementation for GPU's
*/

#include "General_functions.h"

void print_usage(char* executable){
	printf("Expected input format: %s -tag [optional-value] input-file/dirname output-file/dirname\n", executable);
	printf("Available tags:\n");
	printf("-D -> Decompression (Compression is default), Values: 0(CPU Version, Default) || 1(CPU+GPU version)\n");
	printf("-R -> If input is a Directory (A file is assumed to be given by default)\n");
	printf("-B -> Block size(Bytes), 1024 <= Values <= 49152 \n");
	printf("-p -> Display information regarding the openCl platform\n");
	printf("-g -> Display information regarding the GPU Device being used\n");
	printf("-P -> Represents number of work-groups launced per SMP, Values >= 1(DefaultValue)\n");
	printf("-T -> No of work-items in a work-group, 8 <= Values <= 1024 (Default value = 128)\n");
	printf("-t -> Display timing information of the program\n");
	exit(1);
}

int RecursiveCompressFiles(int max_files, int max_filename_sz, char *MainDir, char *NewDir, char **files, char **out_files){
	struct dirent* de;
	
	char **Dirs = (char **)malloc(max_files*sizeof(char*));
	if(Dirs == NULL){
		printf("Malloc Failed: RecursiveCompressFiles: Dirs\n");
		return 1;
	}

	char *cwd = (char *)malloc(max_filename_sz*sizeof(char));		//path of directory specified for compression
	strcat(cwd, "./");
	strcat(cwd, MainDir);
	strcat(cwd, "/");

	int Dirslen = 1;
	Dirs[0] = cwd;
	int filelen = 0;
	int dirname_len = strlen(MainDir);
	struct stat st = {0};

	if(stat(NewDir, &st) == -1){
		mkdir(NewDir, 0700);
	}
	else{
		printf("Directory %s already exists, Please give a differnet name for the directory\n", NewDir);
		exit(1);
	}

	while(Dirslen != 0){
		DIR *dr = opendir(Dirs[Dirslen-1]);
		char *path = (char *)malloc(max_filename_sz*sizeof(char));	//Path is updated for each sub-directory present
		strcpy(path, Dirs[Dirslen-1]);
		if (dr == NULL){
			printf("Unable to open dir '%s'\n", Dirs[Dirslen-1]);
			return 1;
		}
		free(Dirs[Dirslen-1]);
		Dirslen--;
		while((de = readdir(dr)) != NULL){
		    char *name = de->d_name;
		    if(name[0] == '.') continue;
		    else if(de->d_type == DT_DIR){				//If entry is a directory
		    	Dirs[Dirslen] = (char *)malloc(sizeof(char)*max_filename_sz);
		    	memset(Dirs[Dirslen], 0, max_filename_sz);
		    	strcat(Dirs[Dirslen], NewDir);				//Creation of subdirectory in the compressed folder
		    	strcat(Dirs[Dirslen], "/");
		    	strcat(Dirs[Dirslen], path + 3 + dirname_len);
		    	strcat(Dirs[Dirslen], name);
		    	mkdir(Dirs[Dirslen], 0700);
		    	memset(Dirs[Dirslen], 0, max_filename_sz);		//Add path of sub-directory
		    	strcat(Dirs[Dirslen], path);
		    	strcat(Dirs[Dirslen], name);
		    	strcat(Dirs[Dirslen], "/");
		    	Dirslen++;
		    	continue;
		    }								//If entry is a file
		    files[filelen] = (char *)malloc(sizeof(char)*max_filename_sz);
		    out_files[filelen] = (char *)malloc(sizeof(char)*max_filename_sz);
		    memset(files[filelen], 0, max_filename_sz);
		    strcat(files[filelen], path);
		    strcat(files[filelen], name);
		    memset(out_files[filelen], 0, max_filename_sz);
		    strcat(out_files[filelen], NewDir);
		    strcat(out_files[filelen], "/");
		    strcat(out_files[filelen], files[filelen] + dirname_len + 3);
		    filelen++;
		}
		closedir(dr);
	}
	return filelen;
}
