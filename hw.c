// CSCI 4210 Homework 3
// Shantanu Patil (patils2)
// Section 01

#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>

void* childTask(void* args);

// Struct to hold each word read and file from which it came 
struct word
{
	char * this_word;
	char * file;
}; 

// Global variables 
int currentSize = 8;
int numWords = 0;
int currIndex = 0;
struct word* allWords;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[])
{
	if(argc != 3) // Make sure correct number of arguments provided 
	{
		fprintf(stderr, "Error: incorrect arguments provided\n");
	}
	DIR * dir = opendir(argv[1]);   /* open the specified working directory */
	if ( dir == NULL )
	{
		perror( "opendir() failed" );
		return 1;
	}

	struct dirent * file;
	char** fNames = (char**)malloc(0);
	int numFiles = 0;
	char* substr = argv[2];

	// Go through directories; get and save all the file names 
	while (( file = readdir( dir )) != NULL )
  	{
  		if(strcmp(".", file->d_name) && strcmp("..", file->d_name))
  		{
  			numFiles++;
  			fNames = (char**)realloc(fNames, numFiles * sizeof(char*));
  			fNames[numFiles-1] = (char*)malloc((strlen(file->d_name)+1) * sizeof(char));
  			strcpy(fNames[numFiles-1], file->d_name);
  		}
    }
    closedir(dir);
    allWords = (struct word*)calloc(8, sizeof(struct word));
    printf("MAIN THREAD: Allocated initial array of 8 pointers.\n");
    pthread_t childThreads[numFiles];
    int i;
    int rc;
    char* path;
    for(i = 0; i < numFiles; i++)
    {
    	path = (char*)calloc((strlen(argv[1]) + strlen(fNames[i]) + 1), sizeof(char));
    	strcat(path, argv[1]);
    	strcat(path, "/");
    	strcat(path, fNames[i]);
    	rc = pthread_create( &childThreads[i], NULL, childTask, path);
    	if ( rc != 0 )
	    {
	      fprintf( stderr, "MAIN: Could not create child thread (%d)\n", rc );
	    }
    	printf("MAIN THREAD: Assigned \"%s\" to child thread %zu.\n", fNames[i], (unsigned long)childThreads[i]);
    }

    // Child threads created, have each thread read it's file contents 
    for(i = 0; i < numFiles; i++)
    {
    	pthread_join( childThreads[i], NULL);
    }
    //numWords--; // function counts 1 more word than there actually is, correctional measure 
    printf( "MAIN THREAD: All done (successfully read %d words from %d files).\n", numWords, numFiles);
    printf("MAIN THREAD: Words containing substring \"%s\" are:\n", argv[2]);

    // Go through all the words containing the substring and print them
    for(i = 0; i < numWords; i++)
    {
    	if(strstr((allWords[i].this_word), substr))
    	{
    		int temp = strlen(allWords[i].file);
    		int k;
    		for(k = 0; k < temp; k++)
    		{
    			if(allWords[i].file[k] == '/')
    			{
    				break;
    			}
    		}
    		char* betterFile = (char*)malloc(temp * sizeof(char));
    		allWords[i].file += (k+1);
    		strcpy(betterFile, allWords[i].file);
    		printf("MAIN THREAD: %s (from \"%s\")\n", allWords[i].this_word, betterFile);
    	}
    }
	return 0; 
}

void* childTask(void* arg)
{
	//pthread_mutex_lock(&lock);
	char * file_to_open = (char*) arg;
	char buffered_word[50]; // buffered array to hold word
	char* temp;
	// Opens file for reading, throws error if file could not be opened
	FILE *instr = fopen(file_to_open, "r");
	if(instr == NULL)
	{
		fprintf(stderr, "Could not open file for reading, exiting\n");
		exit(1);
	}
	// Parses input file until the end
	while(!feof(instr))
	{
		fscanf(instr, "%s", buffered_word);
		pthread_mutex_lock(&lock);
		if(currIndex == currentSize-1) //Array full, re-allocate array
		{
			currentSize = currentSize*2;
			allWords = (struct word*)realloc(allWords, currentSize * sizeof(struct word));
			printf("THREAD %zu: Re-allocated array of %d pointers.\n", (unsigned long)pthread_self(),currentSize);
		}
		pthread_mutex_unlock(&lock);
		int t = 0;
		int length = 0;
		while(isalnum(buffered_word[t]))
		{
			++t;
			++length;
		}
		++length; // add 1 more for \0
		temp = (char*)malloc(length * sizeof(char)); //dynamically create string for word 
		pthread_mutex_lock(&lock);
		strcpy(temp, buffered_word);
		allWords[currIndex].this_word = (char *)malloc(length * sizeof(char));
		strcpy((allWords[currIndex].this_word), temp);
		allWords[currIndex].file = (char *)malloc((strlen(file_to_open)) * sizeof(char));
		strcpy((allWords[currIndex].file), file_to_open);
		currIndex++;
		numWords++;
		pthread_mutex_unlock(&lock);
		printf("THREAD %zu: Added \"%s\" at index %d.\n", (unsigned long)pthread_self(), temp, currIndex);
	}
	//pthread_mutex_unlock(&lock);
	return NULL;
}