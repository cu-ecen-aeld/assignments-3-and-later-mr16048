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
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    thread_data* thread_func_args = (thread_data *) thread_param;

    pthread_mutex_t *mutex = thread_func_args->mutex;
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    printf("child start to try locking mutex\n");
    pthread_mutex_lock(mutex);
    printf("child locked mutex\n");

    usleep(thread_func_args->wait_to_release_ms * 1000);
   
    thread_func_args->thread_complete_success = true;
   
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
    // void *ret_val;

     thread_data *data = (thread_data *)malloc(sizeof(thread_data));
     if(data == NULL){
        return false;
     }

     data->wait_to_obtain_ms = wait_to_obtain_ms;
     data->wait_to_release_ms = wait_to_release_ms;
     data->mutex = mutex;
     data->thread_complete_success = false;

     if(pthread_create(thread, NULL, threadfunc, data) != 0){
        return false;
     }

    //  free(data);
    return true;
}

