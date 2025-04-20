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

#define BUF_SIZE 1024
#define OUT_FILE "/var/tmp/aesdsocketdata"

static int write_all(int, void *, size_t);
void sig_handler(int);
// static void print_cur_time(void);

volatile sig_atomic_t quit_sig = 0;

void sig_handler(int sig){
  quit_sig = 1;
}

int main(){
  int sock, new_fd, read_byte, out_fd, read_from_file;
  int err = 0;
  struct addrinfo hints, *res;
  char buffer[BUF_SIZE];
  char file_buffer[BUF_SIZE];
  struct sockaddr_storage client_addr;
  socklen_t addr_size;
  char host[NI_MAXHOST];

  echo("start aesdsocket main\n");
  openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

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
    close(sock);
    perror("getaddrinfo");
    return -1;
  }

  if(bind(sock, res->ai_addr, res->ai_addrlen) == -1){
    close(sock);
    freeaddrinfo(res);
    perror("bind");
    return -1;
  }
  freeaddrinfo(res);

  if(listen(sock, 3) == -1){
    close(sock);
    perror("listen");
    return -1;
  }

  addr_size = sizeof(client_addr);
  
  while(quit_sig == 0){
     
    //accept new connection
    new_fd = accept(sock, (struct sockaddr *)&client_addr, &addr_size);
    if (new_fd == -1) {
        goto CLOSE;
    }

    getnameinfo((struct sockaddr *)&client_addr, addr_size,
                  host, sizeof(host), NULL, 0, NI_NUMERICHOST);
    syslog(LOG_INFO, "Accepted connection from %s\n", host);
    printf("Accepted connection from %s\n", host);
    echo("accepted");

    out_fd = open(OUT_FILE, O_RDWR | O_CREAT | O_APPEND, 0644);
    if(out_fd == -1){
      err = 1;
      goto CLOSE;
    }
    
    // read from client
    while(1){      
      read_byte = read(new_fd, buffer, BUF_SIZE);  
      if(read_byte <= 0){
        if(read_byte < 0){
          err = 1;
        }
        break;
      }
    // write to file
      err = write_all(out_fd, buffer, read_byte);
      if(err != 0){
        break;
      }
      if(memchr(buffer, '\n', read_byte)){
        break;
      }
    }
    close(out_fd);

    out_fd = open(OUT_FILE, O_RDONLY, 0644);
    while(1){
      // read from file
      read_from_file = read(out_fd, file_buffer, BUF_SIZE);
      if(read_from_file <= 0){
        if(read_from_file < 0){
          err = 1;
        }
        break;
      }
      printf("Read %d bytes from file\n", read_from_file);
      echo("read");
      //send to client
      err = write_all(new_fd, file_buffer, read_from_file);
      if(err != 0){
        perror("send to client\n");
        break;
      }
    }
    echo("write");
    syslog(LOG_INFO, "Closed connection from XXX %s\n", host);
    close(out_fd);
    close(new_fd);
  }

CLOSE:
  if(quit_sig > 0){
    syslog(LOG_INFO, "Caught signal, exiting");
    printf("Caught signal, exiting\n");
  }
  if(err != 0){
    close(new_fd);
    close(out_fd);
  }
  close(sock);
  closelog();
  if(unlink(OUT_FILE)){
    err = 1;
  }
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
