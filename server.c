/* CSCI4061 Fall 2018 Project 3
Ganzorig Battur
battu010*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
#include "util.h"
#include <stdbool.h>

#define MAX_THREADS 100
#define MAX_queue_len 100
#define MAX_CE 100
#define INVALID -1
#define BUFF_SIZE 1024
char* givenPath;
int countThread;
int cacheSize;
int usedSpace =0;
int qLen;
pthread_mutex_t mutexThread = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexCache = PTHREAD_MUTEX_INITIALIZER;

/*
  THE CODE STRUCTURE GIVEN BELOW IS JUST A SUGESSTION. FEEL FREE TO MODIFY AS NEEDED
*/
// structs:
pthread_mutex_t mutexQueue = PTHREAD_MUTEX_INITIALIZER;
typedef struct request_queue {
   int fd;
   void *request;
} request_t;

typedef struct cache_entry {
    int len;
    char *request;
    char *content;
    int freqCalled;
} cache_entry_t;

cache_entry_t * cacheArray;


request_t reqQueue[MAX_queue_len];
int firstRQ=0;
int lastRQ=MAX_queue_len-1;
int sizeRQ =0;
int capacityRQ = MAX_queue_len;

void enqueue(int myfd, char* myrequest){
  if(isFull()){
    return;
  }
  lastRQ = (lastRQ +1)%capacityRQ;
  reqQueue[lastRQ].fd = myfd;
  reqQueue[lastRQ].request = (void *) myrequest;
  sizeRQ++;
}
int isFull(){
   return qLen == sizeRQ;
}

int dequeue( request_t * result){
  if (sizeRQ > 0){
    request_t holder = reqQueue[firstRQ];
    firstRQ = (firstRQ+1)%capacityRQ;
    sizeRQ--;
    result->fd = holder.fd;
    result->request = holder.request;
    return 1;
  }
  return 0;
}
/* ************************ Dynamic Pool Code ***********************************/
// Extra Credit: This function implements the policy to change the worker thread pool dynamically
// depending on the number of requests
void * dynamic_pool_size_update(void *arg) {
  while(1) {
    // Run at regular intervals
    // Increase / decrease dynamically based on your policy
  }
}
/**********************************************************************************/

/* ************************************ Cache Code ********************************/

// Function to check whether the given request is present in cache
int getCacheIndex(char *request){
  // return the index if the request is present in the cache
  for(int i =0; i < usedSpace; i++){
    if(strcmp(request, cacheArray[i].request) == 0){
      cacheArray[i].freqCalled += 1;
      return i;
    }

  }
  return -1;
}


// Function to add the request and its file content into the cache
void addIntoCache(char *mybuf, char *memory , int memory_size){
  // It should add the request at an index according to the cache replacement policy
  // Make sure to allocate/free memeory when adding or replacing cache entries
        if(usedSpace >= cacheSize){
          //need to remove one and add
          int min = cacheArray[0].freqCalled;
          int index = 0;
          for(int i = 1; i < cacheSize; i++){
            if(cacheArray[i].freqCalled < min){
              index = i;
              min = cacheArray[i].freqCalled;
            }
          }

          free(cacheArray[index].content);

          cacheArray[index].len = memory_size;
          strcpy(cacheArray[index].request, mybuf);
          cacheArray[index].content = memory;
          cacheArray[index].freqCalled = 1;

        }else{
          // meed to add;
          cacheArray[usedSpace].len = memory_size;
          cacheArray[usedSpace].request = malloc(strlen(mybuf));
          strcpy(cacheArray[usedSpace].request, mybuf);

          cacheArray[usedSpace].content = memory;
          cacheArray[usedSpace].freqCalled = 1;

          usedSpace++;
        }
}

// clear the memory allocated to the cache
void deleteCache(){
  // De-allocate/free the cache memory
  for(int i = 0; i < cacheSize; i++){
    free(&(cacheArray[i].request));
  }
  free(cacheArray);
}

// Function to initialize the cache
void initCache(){
  // Allocating memory and initializing the cache array
  cacheArray = malloc(sizeof(cache_entry_t) * cacheSize);

}


// Function to open and read the file from the disk into the memory
// Add necessary arguments as needed
char * readFromDisk(int * retSize,  char* path) {
  // Open and read the contents of file given the request
  // open file with fd by fopen()
  char * str3 = (char *) malloc(2 + strlen(givenPath)+ strlen(path) );
  strcpy(str3, givenPath);
  strcat(str3, path);

  FILE* openedFile= fopen( str3, "r");
  if (openedFile == NULL){
    printf("opened file failed \n" );
    return NULL;
  }
  fseek(openedFile, 0, SEEK_END);
  long fileSize = ftell(openedFile);
  fseek(openedFile, 0, SEEK_SET);
  //store in buf by fread()

  char* readStorage = malloc (fileSize+1);
  if(readStorage == NULL){
    printf("Malloc failed\n" );
  }

   int result = fread(readStorage, fileSize, 1, openedFile);
   *retSize = fileSize;
   fclose(openedFile);

  return readStorage;

}

/**********************************************************************************/

/* ************************************ Utilities ********************************/
// Function to get the content type from the request
void getContentType(char * mybuf, char * ans) {
  char *  holder;
  int i = 0;
  while(mybuf[i] != '\0'){
    if(mybuf[i] == '.'){
      holder = mybuf+i;
    }
    i++;
  }
   if(strcmp(holder, ".gif") == 0){
     strcpy(ans,"image/gif");
   }else if(strcmp(holder, ".jpg") == 0){
     strcpy(ans,"image/jpeg");
   }else if(strcmp(holder, ".html") ==0 || strcmp(holder, ".htm") == 0){
     strcpy(ans,"text/html");
   }else {
     strcpy(ans,"text/plain");
   }
}

// This function returns the current time in milliseconds
long getCurrentTimeInMicro() {
  struct timeval curr_time;
  gettimeofday(&curr_time, NULL);
  return curr_time.tv_sec * 1000000 + curr_time.tv_usec;
}

/**********************************************************************************/


// Function to receive the request from the client and add to the queue
void * dispatch(void *arg) {
  //printf("%d th thread dispatcher\n", count);
  while (1) {
    int fd;
    // Accept client connection
    if((fd = accept_connection())<0){
      fprintf(stderr, "Negative file description\n" );
      return NULL;
    }
    // Get request from the client
    char buf[BUFF_SIZE];

    if(get_request(fd,&buf) != 0){
      fprintf(stderr, "Get Request Fail\n");
      return NULL;
    }
    // Add the request into the queue
    //lock
    pthread_mutex_lock(&mutexQueue);
    enqueue(fd, &buf );
    pthread_mutex_unlock(&mutexQueue);
      //unlock

   }
   return NULL;
}

/**********************************************************************************/

// Function to retrieve the request from the queue, process it and then return a result to the client
void * worker(void *arg) {
  // giving each worker thread assinged number.
  pthread_mutex_lock(&mutexThread);
  countThread++;
  int worker = countThread;
  pthread_mutex_unlock(&mutexThread);
  int countRequest = 0;
  while (1) {

    // Start recording time
    long start = getCurrentTimeInMicro();
    // Get the request from the queue
    // lock
    pthread_mutex_lock(&mutexQueue);
    request_t dequeued;
    int result = dequeue(&dequeued);
    pthread_mutex_unlock(&mutexQueue);
    if (result != 0){
      countRequest++;
      char buf[BUFF_SIZE];
      getContentType((char *)dequeued.request, &buf);
      // Get the data from the disk or the cache
      // Check in cache
      char * bufRead;
      int isError = 0;
      int readedSize =0;
      char * errorText;
      char* cacheReasult = "MISS";
      pthread_mutex_lock(&mutexCache);
      int indexC = getCacheIndex((char *)dequeued.request);
      if(indexC != -1){
        bufRead = cacheArray[indexC].content;
        readedSize = cacheArray[indexC].len;
        cacheReasult = "HIT";
      }else{
        //if not in cache then read form disk
        bufRead = readFromDisk( &readedSize,(char *)dequeued.request);
        if(bufRead == NULL) {
          errorText = "Failed to open file";
          return_error(dequeued.fd, errorText );
          isError = 1;
        }else{
        //save into disk read to cache
          addIntoCache(dequeued.request, bufRead, readedSize);
        }
      }
      pthread_mutex_unlock(&mutexCache);

      // Stop recording the time
      long end = getCurrentTimeInMicro();
      // Log the request into the file and terminal
      FILE *f = fopen("webserver_log.txt", "a");
      if (isError == 1){
        fprintf(f, "[%d][%d][%d][%s][%s][%ldus][%s]\n", worker, countRequest, dequeued.fd, dequeued.request, errorText, (end-start), cacheReasult);
        fprintf(stderr, "[%d][%d][%d][%s][%s][%ldus][%s]\n", worker, countRequest, dequeued.fd, dequeued.request, errorText, (end-start),cacheReasult);
      }else{
        fprintf(f, "[%d][%d][%d][%s][%d][%ldus][%s]\n", worker, countRequest, dequeued.fd, dequeued.request, readedSize, (end-start), cacheReasult);
        fprintf(stderr, "[%d][%d][%d][%s][%d][%ldus][%s]\n", worker, countRequest, dequeued.fd, dequeued.request, readedSize, (end-start),cacheReasult);

      }
      fclose(f);

      // return the result
      if(readedSize > 0){
        return_result(dequeued.fd, buf, bufRead, readedSize);
      }
    }
  }
  return NULL;
}


/**********************************************************************************/

int main(int argc, char **argv) {

  // Error check on number of arguments
  if(argc != 8){
    printf("usage: %s port path num_dispatcher num_workers dynamic_flag queue_length cache_size\n", argv[0]);
    return -1;
  }

  // Get the input args
  int portN = atoi(argv[1]);
  givenPath = argv[2];
  int num_disp =atoi( argv[3]);
  int num_worker = atoi(argv[4]);
  int dynamicFlag = 0;
  qLen = atoi(argv[6]);
  cacheSize = atoi(argv[7]);

  // Perform error checks on the input arguments
  if(portN > 65535 || portN < 1025){
    printf("Enter port between 1025 to 65535\n");
    return -1;
  }
  if(num_disp > 100 || num_disp <= 0){
    printf("number of dispatch must be between 1 to 100:  %d\n", num_disp );
    return -1;
  }

  if(num_worker > 100 || num_disp <= 0){
    printf("number of worker must be between 1 to 100\n" );
    return -1;
  }
  if(qLen > MAX_queue_len){
    printf("queue length must be less than %d\n", MAX_queue_len);
    return -1;
  }
  // Change the current working directory to server root directory
  int dirChange = chdir(givenPath);
  if(dirChange == -1){
    printf("Unable to connect to root directory\n" );
  }
  char cwd[1024];

  // Start the server and initialize cache
  init(portN);
  // Create dispatcher and worker threads
  initCache();
  for(int i = 0; i < num_disp; i ++){
    pthread_t th1;
    pthread_create(&th1, NULL, dispatch, NULL);
  }
  for(int i = 0; i < num_worker; i ++){
    pthread_t th1;
    pthread_create(&th1, NULL, worker, NULL);
  }
  while (1);
  // Clean up
  // since we do not have option to exit as server. It uncertain to decide when we should call deleteCache() function.
  deleteCache();
  return 0;
}
