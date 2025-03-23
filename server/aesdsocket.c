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

#define BUF_SIZE 1024

int main(){
  int sock, new_fd, read_byte, write_byte, out_fd;
  int err = 0;
  struct addrinfo hints, *res;
  char buffer[BUF_SIZE];

  openlog("aesdsocket", LOG_PID | LOG_CONS, LOG_USER);

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if(sock == -1){
    return -1;
  }

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;        // IPv4
  hints.ai_socktype = SOCK_STREAM;  // TCP
  hints.ai_flags = AI_PASSIVE;      // For binding with NULL node (listen on all interfaces)

  if(getaddrinfo(NULL, "9000", &hints, &res) != 0){
    close(sock);
    return -1;
  }

  if(bind(sock, res->ai_addr, res->ai_addrlen) == -1){
    close(sock);
    freeaddrinfo(res);
    return -1;
  }
  freeaddrinfo(res);

  if(listen(sock, 3) == -1){
    close(sock);
    return -1;
  }

  struct sockaddr_storage client_addr;
  socklen_t addr_size = sizeof(client_addr);
  new_fd = accept(sock, (struct sockaddr *)&client_addr, &addr_size);
  if (new_fd == -1) {
      close(sock);
      return -1;
  }

  char host[NI_MAXHOST];
  getnameinfo((struct sockaddr *)&client_addr, addr_size,
                host, sizeof(host), NULL, 0, NI_NUMERICHOST);
  syslog(LOG_INFO, "Accepted connection from %s\n", host);

  out_fd = open("/var/tmp/aesdsocketdata", O_RDWR | O_CREAT | O_TRUNC, 0644);
  if(out_fd == -1){
    err = 1;
    goto CLOSE;
  }

  while(1){

      // read from client
      read_byte = read(new_fd, buffer, BUF_SIZE);
      if(read_byte < 0){
        err = 1;
        break;
      }

      // write to file
      write_byte = write(out_fd, buffer, read_byte);
      if(write_byte != read_byte){
        err = 1;
        break;
      }
  }

CLOSE:
  close(new_fd);
  close(sock);
  closelog();
  if(err > 0){
    return -1;
  }
}