//header files
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>

#define iterations 1000000
#define iterations1 10000

//bunch of variable and array declarations to avoid memory errors
long usertime1, systime1, usertime2, systime2; //to store the user and system times of the task and empty loop
double usertime3, systime3, usertimeavg, systimeavg; //to store the diff between usertime and systime of empty loop from that of the task.... and then to save the avg of the usertime and systime
int i = 0; //iterator
int page;
struct rusage start1, end1; //for task
struct rusage start2, end2; //for empty loop
struct timeval start3, end3;
struct timeval start4, end4;
double time1, time2, time3, timeavg;
void *ptrarray[iterations]; 
pthread_mutex_t mutex[iterations]; //used to initialize the mutex struct
pthread_t thread;

int main(){

    //1st task - allocate one page of memory with mmap()
    page = getpagesize();

    //get the usage statistics before the task loop starts
    getrusage(RUSAGE_SELF, &start1);
    for(i=0; i < iterations; i++) {
        ptrarray[i] = mmap(NULL, page, PROT_READ,MAP_ANONYMOUS|MAP_PRIVATE,-1,0); 
    }
    getrusage(RUSAGE_SELF, &end1);
    //get the usage statistics after the task loop ends

    //checking the errors of the function calls and simultaneously deallocating the memory
    for(i=0; i < iterations; i++) {
        if(ptrarray[i]==MAP_FAILED){
            perror("mmap error");
            exit(EXIT_FAILURE);
        }
        if(munmap(ptrarray[i], page)==-1){
            perror("munmap error");
            exit(EXIT_FAILURE);
        }
    }
    
    //calculating the user time of task loop
    usertime1 = (double)(end1.ru_utime.tv_sec - start1.ru_utime.tv_sec) * 1000000 + (double)(end1.ru_utime.tv_usec - start1.ru_utime.tv_usec);
    //calculating the system time of task loop
    systime1 = (double)(end1.ru_stime.tv_sec - start1.ru_stime.tv_sec) * 1000000 + (double)(end1.ru_stime.tv_usec - start1.ru_stime.tv_usec);

    //get the usage statistics before the empty loop starts
    getrusage(RUSAGE_SELF, &start2);
    for(i=0; i < iterations; i++) {
    }
    getrusage(RUSAGE_SELF, &end2);
    //get the usage statistics after the empty loop ends

    //calculating the user time of empty loop
    usertime2 = (double)(end2.ru_utime.tv_sec - start2.ru_utime.tv_sec) * 1000000 + (double)(end2.ru_utime.tv_usec - start2.ru_utime.tv_usec);
    //calculating the system time of empty loop
    systime2 = (double)(end2.ru_stime.tv_sec - start2.ru_stime.tv_sec) * 1000000 + (double)(end2.ru_stime.tv_usec - start2.ru_stime.tv_usec);

    //subtracting the time taken to run an empty loop from the time taken to run the task loop 
    usertime3 = usertime1 - usertime2;
    systime3 = systime1 - systime2;

    //getting the average user time and average system time
    usertimeavg = usertime3/iterations;
    systimeavg = systime3/iterations;

    printf("1a. Average user time to allocate one page of memory with mmap(): %f microseconds\n", usertimeavg);
    printf("1b. Average system time to allocate one page of memory with mmap(): %f microseconds\n", systimeavg);

    //------------------------------------------------------------------------------------

    //2nd task - lock a mutex with pthread_mutex_lock()

    //doing the initialization in a seperate for loop
    for(i=0; i < iterations; i++) {
        if (pthread_mutex_init(&mutex[i], NULL) !=0) perror("mutex initialization error"); 
    }

    //Locking the array of mutexes in a for loop
    getrusage(RUSAGE_SELF, &start1);
    for(i=0; i < iterations; i++) {
        pthread_mutex_lock(&mutex[i]); 
    }
    getrusage(RUSAGE_SELF, &end1);

    //unlocking and destroying the mutexes 
    for(i=0;i<iterations;i++){
        pthread_mutex_unlock(&mutex[i]);
        pthread_mutex_destroy(&mutex[i]);
    }

    //calculating the user and system times to perform the task loop
    usertime1 = (double)(end1.ru_utime.tv_sec - start1.ru_utime.tv_sec) * 1000000 + (double)(end1.ru_utime.tv_usec - start1.ru_utime.tv_usec);
    systime1 = (double)(end1.ru_stime.tv_sec - start1.ru_stime.tv_sec) * 1000000 + (double)(end1.ru_stime.tv_usec - start1.ru_stime.tv_usec);

    //running and calculating the time taken to run an empty for loop
    getrusage(RUSAGE_SELF, &start2);
    for(i=0; i < iterations; i++) {
    }
    getrusage(RUSAGE_SELF, &end2);

    //calculating the user and system times to perform the empty loop
    usertime2 = (double)(end2.ru_utime.tv_sec - start2.ru_utime.tv_sec) * 1000000 + (double)(end2.ru_utime.tv_usec - start2.ru_utime.tv_usec);
    systime2 = (double)(end2.ru_stime.tv_sec - start2.ru_stime.tv_sec) * 1000000 + (double)(end2.ru_stime.tv_usec - start2.ru_stime.tv_usec);

    //subtracting the time taken to run an empty loop from the task loop
    usertime3 = usertime1 - usertime2;
    systime3 = systime1 - systime2;

    //calculating the average time
    usertimeavg = usertime3/iterations;
    systimeavg = systime3/iterations;

    printf("2a. Average user time to lock a mutex with pthread_mutex_lock(): %f microseconds\n", usertimeavg);
    printf("2b. Average system time to lock a mutex with pthread_mutex_lock(): %f microseconds\n", systimeavg);

    //------------------------------------------------------------------------------------
    
    //3rd - 6th task
    int fd;
    int *check;
    char *buf;
    
    //3rd task - write 4096 Bytes directly to /tmp

    //allocating memory
    check = (int*)malloc(iterations1 * sizeof(int));
    if(check == NULL){
        perror("memory allocation error");
        exit(EXIT_FAILURE);
    }
    //since we are using O_DIRECT, the memory buffer should be aligned in memory
    if(posix_memalign((void**) &buf, 4096, 4096)!=0){
        perror("posix align error");
        free(buf);
        exit(EXIT_FAILURE);
    }
    memset(buf, 'a', 4096);
    //opening the file
    fd = open("/tmp/vrames01.txt", O_DIRECT | O_SYNC | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(fd==-1){
        perror("fd open");
        free(buf);
        exit(EXIT_FAILURE);
    }

    //Start measuring time for the write loop
    gettimeofday(&start3, NULL);
    for (i = 0; i < iterations1; i++) {
        check[i] = write(fd, buf, 4096);
    }
    gettimeofday(&end3, NULL);
    //End measuring time for the write loop

    //checking the errors of the function calls
    for(i=0;i<iterations1;i++){
        if(check[i]==-1){
            perror("write ");
            free(buf);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    //free the memory buffer which was used to store and check for errors
    free(check);

    //calculate the wall-clock time to perform the write loop for n times
    time1 = ((end3.tv_sec - start3.tv_sec) * 1000000) + (end3.tv_usec - start3.tv_usec);

    //Start measuring time for the empty loop
    gettimeofday(&start4, NULL);
    for (i = 0; i < iterations1; i++) {
    }
    gettimeofday(&end4, NULL);
    //End measuring time for the empty loop

    //calculate the wall-clock time to perform the empty loop for n times
    time2 = ((end4.tv_sec - start4.tv_sec) * 1000000) + (end4.tv_usec - start4.tv_usec);

    //subtracting the time taken to perform an empty for-loop from the time taken to perform the write() for-loop
    time3 = time1 - time2;
    //calculating the average wall-clock time 
    timeavg = time3 / iterations1;

    printf("3. Average wall-clock time to write 4096 bytes directly to /tmp: %f microseconds\n", timeavg);
    //closing the file descriptor
    if(close(fd) == -1){
        perror("close");
        free(buf);
        exit(EXIT_FAILURE);
    }
    //free the buffer
    free(buf);
    //removing the file at the end after the final task

    //------------------------------------------------------------------------------------
    
    //4th task - read 4096 Bytes directly from /tmp

    //allocating memory
    check = (int*)malloc(iterations1 * sizeof(int));
    if(check == NULL){
        perror("memory allocation error");
        exit(EXIT_FAILURE);
    }
    //since we are using O_DIRECT, the memory buffer should be aligned in memory
    if(posix_memalign((void**) &buf, 4096, 4096)!=0){
        perror("posix align error");
        free(buf);
        exit(EXIT_FAILURE);
    }
    memset(buf, 'a', 4096);
    //opening the file
    fd = open("/tmp/vrames01.txt", O_DIRECT | O_SYNC | O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if(fd==-1){
        perror("fd open");
        free(buf);
        exit(EXIT_FAILURE);
    }

    //Start measuring time for the read loop
    gettimeofday(&start3, NULL);
    for (i = 0; i < iterations1; i++) {
        check[i] = read(fd, buf, 4096);
    }
    gettimeofday(&end3, NULL);
    //End measuring time for the read loop

    //checking the errors of the function calls
    for(i=0;i<iterations1;i++){
        if(check[i]==-1){
            perror("read ");
            free(buf);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    //free the memory buffer which was used to store and check for errors
    free(check);

    //calculate the wall-clock time to perform the write loop for n times
    time1 = ((end3.tv_sec - start3.tv_sec) * 1000000) + (end3.tv_usec - start3.tv_usec);

    //Start measuring time for the empty loop
    gettimeofday(&start4, NULL);
    for (i = 0; i < iterations1; i++) {
    }
    gettimeofday(&end4, NULL);
    //End measuring time for the empty loop

    //calculate the wall-clock time to perform the empty loop for n times
    time2 = ((end4.tv_sec - start4.tv_sec) * 1000000) + (end4.tv_usec - start4.tv_usec);

    //subtracting the time taken to perform an empty for-loop from the time taken to perform the write() for-loop
    time3 = time1 - time2;
    //calculating the average wall-clock time 
    timeavg = time3 / iterations1;

    printf("4. Average wall-clock time to read 4096 bytes directly from /tmp: %f microseconds\n", timeavg);
    //closing the file descriptor
    if(close(fd) == -1){
        perror("close");
        free(buf);
        exit(EXIT_FAILURE);
    }
    //free the buffer
    free(buf);
    //removing the file at the end ie after the last task

    //------------------------------------------------------------------------------------
    
    //5th task - write 4096 Bytes to the disk page cache

    //we have to allocate memory as we are not using memalign
    //and memalign is not used here as we are not using the O_DIRECT flag

    if((buf = malloc(4096))==NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    //allocating memory
    check = (int*)malloc(iterations1 * sizeof(int));
    if(check == NULL){
        perror("memory allocation error");
        exit(EXIT_FAILURE);
    }
    
    //we are not using the O_DIRECT and O_SYNC flags here
    fd = open("/tmp/vrames01.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fd==-1){
        perror("fd open");
        free(buf);
        exit(EXIT_FAILURE);
    }
    memset(buf, 'a', 4096);

    //warming up the disk page cache
    //we fill the cache, so when the real write happens, it has cache access
    off_t offset = 0;
    for(i=0;i<iterations1;i++){
        if(lseek(fd, offset, SEEK_SET)==-1){
            perror("lseek");
            exit(EXIT_FAILURE);
        }
        if(write(fd, buf, 4096)==-1){
            perror("write");
            exit(EXIT_FAILURE);
        }
        offset+=4096;
    }
    if(lseek(fd, 0, SEEK_SET)==-1){
        perror("lseek");
        exit(EXIT_FAILURE);       
    }

    //Start measuring time for the write loop
    gettimeofday(&start3, NULL);
    for (i = 0; i < iterations1; i++) {
        check[i] = write(fd, buf, 4096);
    }
    gettimeofday(&end3, NULL);
    //End measuring time for the write loop

    //checking the errors of the function calls
    for(i=0;i<iterations1;i++){
        if(check[i]==-1){
            perror("write ");
            free(buf);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    //free the memory buffer which was used to store and check for errors
    free(check);

    //calculate the wall-clock time to perform the write loop for n times
    time1 = ((end3.tv_sec - start3.tv_sec) * 1000000) + (end3.tv_usec - start3.tv_usec);

    //Start measuring time for the empty loop
    gettimeofday(&start4, NULL);
    for (i = 0; i < iterations1; i++) {
    }
    gettimeofday(&end4, NULL);
    //End measuring time for the empty loop

    //calculate the wall-clock time to perform the empty loop for n times
    time2 = ((end4.tv_sec - start4.tv_sec) * 1000000) + (end4.tv_usec - start4.tv_usec);

    //subtracting the time taken to perform an empty for-loop from the time taken to perform the write() for-loop
    time3 = time1 - time2;
    //calculating the average wall-clock time 
    timeavg = time3 / iterations1;

    printf("5. Average wall-clock time to write 4096 bytes to the disk page cache: %f microseconds\n", timeavg);
    //closing the file descriptor
    if(close(fd) == -1){
        perror("close");
        free(buf);
        exit(EXIT_FAILURE);
    }
    //free the buffer
    free(buf);
    
    //removing the file at the end, ie after the last task

    //------------------------------------------------------------------------------------
    
    //6th task - read 4096 Bytes from the disk page cache

    //we have to allocate memory as we are not using memalign
    //and memalign is not used here as we are not using the O_DIRECT flag

    if((buf = malloc(4096))==NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    //allocating memory
    check = (int*)malloc(iterations1 * sizeof(int));
    if(check == NULL){
        perror("memory allocation error");
        exit(EXIT_FAILURE);
    }
    
    //we are not using the O_DIRECT and O_SYNC flags here
    fd = open("/tmp/vrames01.txt", O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    if(fd==-1){
        perror("fd open");
        free(buf);
        exit(EXIT_FAILURE);
    }
    memset(buf, 'a', 4096);

    //warming up the disk page cache
    //we read from the cache, so when the real read happens, it has cache access
    offset = 0;
    for(i=0;i<iterations1;i++){
        if(lseek(fd, offset, SEEK_SET)==-1){
            perror("lseek");
            exit(EXIT_FAILURE);
        }
        if(read(fd, buf, 4096)==-1){
            perror("read");
            exit(EXIT_FAILURE);
        }
        offset+=4096;
    }
    if(lseek(fd, 0, SEEK_SET)==-1){
        perror("lseek");
        exit(EXIT_FAILURE);       
    }

    //Start measuring time for the read loop
    gettimeofday(&start3, NULL);
    for (i = 0; i < iterations1; i++) {
        check[i] = read(fd, buf, 4096);
    }
    gettimeofday(&end3, NULL);
    //End measuring time for the read loop

    //checking the errors of the function calls
    for(i=0;i<iterations1;i++){
        if(check[i]==-1){
            perror("read ");
            free(buf);
            close(fd);
            exit(EXIT_FAILURE);
        }
    }
    //free the memory buffer which was used to store and check for errors
    free(check);

    //calculate the wall-clock time to perform the write loop for n times
    time1 = ((end3.tv_sec - start3.tv_sec) * 1000000) + (end3.tv_usec - start3.tv_usec);

    //Start measuring time for the empty loop
    gettimeofday(&start4, NULL);
    for (i = 0; i < iterations1; i++) {
        //empty loop
    }
    gettimeofday(&end4, NULL);
    //End measuring time for the empty loop

    //calculate the wall-clock time to perform the empty loop for n times
    time2 = ((end4.tv_sec - start4.tv_sec) * 1000000) + (end4.tv_usec - start4.tv_usec);

    //subtracting the time taken to perform an empty for-loop from the time taken to perform the write() for-loop
    time3 = time1 - time2;
    //calculating the average wall-clock time 
    timeavg = time3 / iterations1;

    printf("6. Average wall-clock time to read 4096 bytes from the disk page cache: %f microseconds\n", timeavg);
    //closing the file descriptor
    if(close(fd) == -1){
        perror("close");
        free(buf);
        exit(EXIT_FAILURE);
    }
    //free the buffer
    free(buf);
    //removing the file
    remove("/tmp/vrames01.txt");
    return 0;
}
