#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)
typedef struct thread_data thread_data;
pthread_mutex_t mutex1;

void* threadfunc(void* thread_param)
{
   struct timespec point1, point2, point3;
   printf("start child\n");
   clock_gettime(CLOCK_MONOTONIC, &point1);
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    thread_data* thread_func_args = (thread_data *) thread_param;

    pthread_mutex_t *mutex = thread_func_args->mutex;

    printf("wait_to_obtain_ms: %d\n", thread_func_args->wait_to_obtain_ms);
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    clock_gettime(CLOCK_MONOTONIC, &point2);
    printf("waited for %ld ms\n", (point2.tv_nsec - point1.tv_nsec)/1000000);
   
    printf("child start to try locking mutex\n");
    pthread_mutex_lock(mutex);
    printf("child locked mutex\n");

    usleep(thread_func_args->wait_to_release_ms * 1000);
   
    clock_gettime(CLOCK_MONOTONIC, &point3);
    printf("waited for %ld ms\n", (point3.tv_nsec - point2.tv_nsec)/1000000);
   
    thread_func_args->thread_complete_success = true;
   //  pthread_cond_signal(thread_func_args->cond);
   //  printf("child send cond sig\n");
    pthread_mutex_unlock(mutex);
    printf("child released mutex\n");

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
    void *ret_val;
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);
   
   //  pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
   pthread_cond_t *cond = (pthread_cond_t *)malloc(sizeof(pthread_cond_t));

     thread_data *data = (thread_data *)malloc(sizeof(thread_data));
     if(data == NULL){
        return false;
     }

     data->wait_to_obtain_ms = wait_to_obtain_ms;
     data->wait_to_release_ms = wait_to_release_ms;
     data->mutex = mutex;
     data->cond = cond;
     data->thread_complete_success = false;

     if(pthread_create(thread, NULL, threadfunc, data) != 0){
        return false;
     }

     printf("parent start waiting\n");
   //   while(!data->thread_complete_success){
   //    printf("parent waiting...\n");  
   //    if(pthread_cond_wait(cond, mutex) != 0){
   //       printf("cond_wait_failed\n");
   //       exit(-1);
   //    }
   //   }

     if(pthread_join(*thread, &ret_val) != 0){
        return false;
     }
     printf("parent detected child finish\n");
     free(data);
     free(cond);

     clock_gettime(CLOCK_MONOTONIC, &end);
     printf("completed in %ld ms\n", (end.tv_nsec - start.tv_nsec)/1000000);
    return true;
}

