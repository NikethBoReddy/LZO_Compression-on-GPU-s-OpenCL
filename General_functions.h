/* Author: Niketh Boreddy
   email: nikethcse@gmail.com
   
  Date: 10th June 2016
  The follwing file is a part of the LZO algortihm implementation for GPU's
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/time.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

void print_usage(char* executable);
int RecursiveCompressFiles(int max_files, int max_filename_sz, char *MainDir, char *NewDir, char **files, char **out_files);
