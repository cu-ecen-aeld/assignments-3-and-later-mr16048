#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include "../aesd-char-driver/aesd_ioctl.h"
#define BUF_SIZE 1024
#define OUT_FILE "/dev/aesdchar"
#define SPECIAL_STR_PREFIX "AESDCHAR_IOCSEEKTO:"

static int write_all(int, void *, size_t);
void sig_handler(int);
// static void print_cur_time(void);
static void* proc_new_connection(void *);
static int create_thread_and_run(int);
static int write_to_out_file(int *, void *, size_t);
static void write_time_to_file(union sigval);
static int write_prdc_to_file(void);
static inline void trim_crlf(char *);
static int parse_seek_command(char *, unsigned *, unsigned *);
// static int aesd_check_special_str(char *, unsigned int, unsigned int *, unsigned int *);

volatile sig_atomic_t quit_sig = 0;
pthread_mutex_t mutex;
volatile sig_atomic_t timer_error_flag = 0;

void sig_handler(int sig){
  quit_sig = 1;
}

int main(){
  int sock, new_fd;
  int err = 0;
  struct addrinfo hints, *res;

  struct sockaddr_storage client_addr;
  socklen_t addr_size;
  char host[NI_MAXHOST];

  printf("start aesdsocket main\n");
  openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);
  syslog(LOG_INFO, "start aesdsocket\n");

  pthread_mutex_init(&mutex, NULL);

  // write timestamp every 10s
  //write_prdc_to_file();

  //set signal handler
  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if(sock == -1){
    perror("socket");
    return -1;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;        // IPv4
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_flags = AI_PASSIVE;      // For binding with NULL node (listen on all interfaces)

  if(getaddrinfo(NULL, "9000", &hints, &res) != 0){
    perror("getaddrinfo ");
    goto CLOSE;
  }
  
  int yes = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
    perror("setsockopt");
    goto CLOSE;
  }
  if(bind(sock, res->ai_addr, res->ai_addrlen) == -1){
    goto CLOSE;
    return -1;
  }
  freeaddrinfo(res);

  if(listen(sock, 3) == -1){
    perror("listen");
    close(sock);
    return -1;
  }

  addr_size = sizeof(client_addr);
  
  while(quit_sig == 0){
     
    //accept new connection
    new_fd = accept(sock, (struct sockaddr *)&client_addr, &addr_size);
    if (new_fd == -1) {
        perror("accept");
        goto CLOSE;
    }

    getnameinfo((struct sockaddr *)&client_addr, addr_size,
                  host, sizeof(host), NULL, 0, NI_NUMERICHOST);
    syslog(LOG_INFO, "Accepted connection from %s\n", host);
    printf("Accepted connection from %s\n", host);

    create_thread_and_run(new_fd); 

    if(timer_error_flag != 0){
      goto CLOSE;
    }
  }

CLOSE:
  if(quit_sig > 0){
    syslog(LOG_INFO, "Caught signal, exiting");
    printf("Caught signal, exiting\n");
  }
  if(err != 0){
    close(new_fd);
  }
  if(close(sock) != 0){
    perror("close sock");
  }
  // if(unlink(OUT_FILE)){
  //   err = 1;
  // }
  syslog(LOG_INFO, "closed sock");
  closelog();
  if(err != 0){    
    return -1;
  }
}

static int write_all(int fd, void *buffer, size_t write_size){
  
  int write_byte, total_write_byte;

  total_write_byte = 0;
  while(total_write_byte < write_size){
    write_byte = write(fd, buffer + total_write_byte, write_size - total_write_byte);
    if(write_byte < 0){
      return 1;
    }
    total_write_byte += write_byte;
  }

  return 0;
}

// static void print_cur_time(void){
//   struct timespec t1;
//   clock_gettime(CLOCK_MONOTONIC, &t1);
//   printf("%ds %dns\n", t1.tv_sec, t1.tv_nsec);
// }

static void* proc_new_connection(void *arg){
  int read_byte, out_fd, read_from_file, tmp_fd;
  char buffer[BUF_SIZE];
  char file_buffer[BUF_SIZE];
  int *err;
  #define TMP_FILE_NAME_SIZE 64
  char tmp_file_name[TMP_FILE_NAME_SIZE];
  
  syslog(LOG_INFO, "start thread");

  int new_fd = *(int*)(arg);
  free(arg);

  err = malloc(sizeof(int));
  if(err == NULL){
    pthread_exit(NULL);
  }
  *err = 0;

  snprintf(tmp_file_name, TMP_FILE_NAME_SIZE, "/var/tmp/aesdsocket%02d", new_fd);
  tmp_fd = open(tmp_file_name, O_RDWR | O_CREAT | O_APPEND, 0644);
  if(tmp_fd == -1){
    *err = 1;
    return err;
  }

 // read from client
  while(1){      
    read_byte = read(new_fd, buffer, BUF_SIZE);  
    if(read_byte <= 0){
      if(read_byte < 0){
        *err = -1;
      }
      break;
    }
    // write to tmp file
    *err = write_all(tmp_fd, buffer, read_byte);
    if(err != 0){
      break;
    }

    if(memchr(buffer, '\n', read_byte)){
      break;
    }
  }
  close(tmp_fd);
  unlink(tmp_file_name);
  if(*err != 0){
    return err;
  }

  // write to file
  out_fd = open(OUT_FILE, O_RDONLY, 0644);

  *err = write_to_out_file(&out_fd, buffer, read_byte);
  if(*err != 0){
    return err;
  }

  while(1){
    // read from file
    read_from_file = read(out_fd, file_buffer, BUF_SIZE);
    if(read_from_file <= 0){
      if(read_from_file < 0){
        *err = 1;
      }
      break;
    }
    // printf("Read %d bytes from file\n", read_from_file);
    //send to client
    *err = write_all(new_fd, file_buffer, read_from_file);
    if(*err != 0){
      perror("send to client\n");
      break;
    }
  }
  // syslog(LOG_INFO, "Closed connection from XXX %s\n", host);
  close(out_fd);
  close(new_fd); 

  return err;
}

static int create_thread_and_run(int new_fd){

  pthread_t thread;
  int err = 0;
  void *ret;

  int *client_fd = malloc(sizeof(int));
  if(!client_fd){
    perror("alloc for client_fd\n");
    err = 1;
    goto TH_END;
  }
  *client_fd = new_fd;

  if (pthread_create(&thread, NULL, proc_new_connection, client_fd)!= 0) {
      perror("Failed to create thread");
      err = 1;
      goto TH_END;
    }

  pthread_join(thread, &ret);
  err = *(int*)ret;
  if(err != 0){
    perror("Failed to join thread");
    err = 1;
    goto TH_END;
  }
  free(ret);
TH_END:
  return err;
}

static int write_to_out_file(int *out_fd, void *buffer, size_t write_size){

  // int out_fd;
  int err = 0;
  pthread_mutex_lock(&mutex);
  if(out_fd == NULL){
    *out_fd = open(OUT_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
  }
  if(*out_fd == -1){
    err = -1;
    return err;
  }

  syslog(LOG_INFO, "write_to_out_file() buffer=%s\n", buffer);
  struct aesd_seekto seekto;
  if(parse_seek_command(buffer, &seekto.write_cmd, &seekto.write_cmd_offset) != 0){
    syslog(LOG_INFO, "write_to_out_file() ioctl\n");  
    ioctl(*out_fd, AESDCHAR_IOCSEEKTO, (unsigned long)&seekto);
  }
  else{
    syslog(LOG_INFO, "write_to_out_file() ordinal write");  
    err = write_all(*out_fd, buffer, write_size);
  }
  // close(out_fd);
  pthread_mutex_unlock(&mutex);

  return err;
}

static void write_time_to_file(union sigval sv){

  // get current time
  time_t now = time(NULL);  
  if(now == (time_t)(-1)){
    perror("get time");
    timer_error_flag = 1;
    return;
  }

  // convert to local time
  struct tm *local_time = localtime(&now);
  if(local_time == NULL){
    perror("localtime");
    timer_error_flag = 1;
    return;
  }

  // format the time string
  char time_str[64];
  if (strftime(time_str, sizeof(time_str), "timestamp: %Y%m%d %H:%M:%S\n", local_time) == 0){
    perror("strftime");
    timer_error_flag = 1;
    return;
  }

  if(write_to_out_file(NULL, time_str, strlen(time_str)) != 0){
    timer_error_flag = 1;
    perror("write timestamp to file");
    return;
  }

  return;
}

static int write_prdc_to_file(){

  struct sigevent sev;
  struct itimerspec its;
  timer_t timerid;

  memset(&sev, 0, sizeof(sev));
  sev.sigev_notify = SIGEV_THREAD;
  sev.sigev_notify_function = write_time_to_file;
  sev.sigev_notify_attributes = NULL;

  if(timer_create(CLOCK_REALTIME, &sev, &timerid) == -1){
    perror("timer_create");
    return 1;
  }

  its.it_interval.tv_sec = 10;
  its.it_interval.tv_nsec = 0;
  its.it_value.tv_sec = 10;
  its.it_value.tv_nsec = 0;

  if(timer_settime(timerid, 0, &its, NULL) == -1){
    perror("settimer");
    return 1;
  }

  return 0;
}

static inline void trim_crlf(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n-1] == '\n' || s[n-1] == '\r')) s[--n] = '\0';
}

/* Returns 1 on success and sets *x,*y. Returns 0 if not a match. */
static int parse_seek_command(char *line, unsigned *x, unsigned *y)
{
    if (!line || !x || !y) return 0;

    trim_crlf(line);

    int consumed = 0;
    int matched = sscanf(line, "AESDCHAR_IOCSEEKTO:%u,%u%n", x, y, &consumed);
    syslog(LOG_INFO, "parse_seek_command() consumed = %d, matched=%d, x = %d, y = %d\n", consumed, matched, x, y); 

    return (matched == 2) && (line[consumed] == '\0');
}
#if 0
static int aesd_check_special_str(char *buf, unsigned int len, unsigned int *x, unsigned int *y){

    // PDEBUG("check_special_str() buf=%s, len=%d", buf, len);

    char *pbuf = NULL;
    size_t n = len;

    // Null terminate string to use it for C str function
    // Copy it to temp buffer not to modify the original
    pbuf = malloc(n + 1);
    if (!pbuf) { return -1; }
    memcpy(pbuf, buf, n);
    pbuf[n] = '\0';

    while (n && (pbuf[n-1] == '\n' || pbuf[n-1] == '\r'))
        pbuf[--n] = '\0';

    const size_t prefix_len = sizeof(SPECIAL_STR_PREFIX) - 1;
    if(len < prefix_len + 3){
        // PDEBUG("check_special_str() : 1");
        return -1;
    }

    if(strncmp(pbuf, SPECIAL_STR_PREFIX, prefix_len) != 0){
        // PDEBUG("check_special_str() : 2");
        return -1;
    }
    
    char *p = pbuf + prefix_len;
    char *comma = strchr(p, ',');
    char *endptr;

    if (!comma) { 
        // PDEBUG("check_special_str() : 3");
        return -1;
    }

    /* enforce NO spaces: fail if any space appears */
    if (strchr(p, ' ') || strchr(p, '\t')) {
        // PDEBUG("check_special_str() : 4");
        return -1;
    }

    /* split X and Y */
    *comma = '\0';
    /* parse X */
    if (kstrtouint(p, 10, x)) { 
        // PDEBUG("check_special_str() : 5");
        return -1;
    }
    /* parse Y (ensure no trailing junk after Y) */
    if (kstrtouint(comma + 1, 10, y)) {
        // PDEBUG("check_special_str() : 6, comma+1=%s", comma + 1); 
        return -1;
     }
    /* also ensure Y is the last token in the inspected chunk */
    #if 0
    endptr = comma + 1;
    while (*endptr && *endptr >= '0' && *endptr <= '9') endptr++;
    if (*endptr != '\0') { 
        PDEBUG("check_special_str() : 7 endptr=%s", endptr);
        return -1; 
    }
    #endif
    // PDEBUG("check_special_str() : 8 x=%d, y=%d", *x, *y);

    return 0;
}
#endif
