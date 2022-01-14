#include "../fs/operations.h"
#include "../fs/state.h"
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define COUNT 200
#define SIZE 4

//creates file and writes Js to it
void* thread_func_1(void* unused) 
{
    (void)(unused);
    struct timespec time, time2;
    time.tv_sec = 0;
    time.tv_nsec = 300000000L;

    char *path = "/f1";
    char input[SIZE]; 
    memset(input, 'J', SIZE);
    
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
    for (int i = 0; i < COUNT; i++) {
        nanosleep(&time, &time2);
        fd = tfs_open(path, TFS_O_APPEND);
        assert(fd != -1);
        assert(tfs_write(fd, input, SIZE) == SIZE);
        assert(tfs_close(fd) != -1);
    }
    printf("CONCLUDED THREAD 1\n");
    return 0;
}

//creates file and writes Ps to to it
void* thread_func_2(void* unused) 
{
    (void)(unused);
    struct timespec time, time2;
    time.tv_sec = 0;
    time.tv_nsec = 400000000L;

    char *path = "/f2";
    char input[SIZE]; 
    memset(input, 'P', SIZE);
    
    int fd = tfs_open(path, TFS_O_CREAT);
    assert(fd != -1);
    assert(tfs_close(fd) != -1);
    for (int i = 0; i < COUNT; i++) {
        nanosleep(&time, &time2);
        fd = tfs_open(path, TFS_O_APPEND);
        assert(fd != -1);
        assert(tfs_write(fd, input, SIZE) == SIZE);
        assert(tfs_close(fd) != -1);
    }
    printf("CONCLUDED THREAD 2\n");
    return 0;
}

//reads from f1
void* thread_func_3(void* unused) 
{
    (void)(unused);
    struct timespec time, time2;
    time.tv_sec = 1;
    time.tv_nsec = 0;

    char *path_1 = "/f1";
    char *path_2 = "/f2";
    char output_1 [SIZE*COUNT+1];
    char output_2 [SIZE*COUNT+1];
    output_1[SIZE*COUNT] = '\0';
    output_2[SIZE*COUNT] = '\0';
    for(int i = 0; i < 51; i++) 
    {
        int fd_1 = tfs_open(path_1, 0);
        int fd_2 = tfs_open(path_2, 0);
        if(fd_1 != -1) 
        {
            ssize_t size_1 = tfs_read(fd_1, output_1, SIZE*COUNT);
            printf("%ld: %.100s\n", size_1, output_1);
            assert(tfs_close(fd_1) != -1);
        } else { printf("COULDNT READ fd_1\n"); }
        if(fd_2 != -1) 
        {
            ssize_t size_2 = tfs_read(fd_2, output_2, SIZE*COUNT);
            printf("%ld: %.100s\n", size_2, output_2);
            assert(tfs_close(fd_2) != -1);
        } else { printf("COULDNT READ fd_2\n"); }
        nanosleep(&time, &time2);
    }    
    printf("CONCLUDED THREAD 3\n");
    return 0;
}

int main() 
{
    assert(tfs_init() != -1);
    pthread_t thread_1, thread_2, thread_3;
    pthread_create(&thread_2, NULL, thread_func_2, NULL);
    pthread_create(&thread_3, NULL, thread_func_3, NULL);
    pthread_create(&thread_1, NULL, thread_func_1, NULL);
    pthread_join(thread_2, NULL);
    pthread_join(thread_3, NULL);
    pthread_join(thread_1, NULL);
    assert(tfs_destroy() != -1);
}