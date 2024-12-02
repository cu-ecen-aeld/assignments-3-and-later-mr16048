#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>
#include <syslog.h>

int main(int argc, char *argv[]) {

  char *writefile;
  char *writestr;
  int fd;
  ssize_t writen;

  openlog("writer", LOG_PID|LOG_CONS, LOG_USER);

  if (argc != 3){
    perror("Insufficient arguments");
    syslog(LOG_ERR, "Insufficient arguments");
    exit(1); 
  }

  writefile = argv[1];
  writestr = argv[2];

  fd = open(writefile,  O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd == -1){
    perror("Error opening file");
    syslog(LOG_ERR, "Error opening file");
    exit(1);
  }

  writen = write(fd, writestr, strlen(writestr));
  if(writen == -1){
    perror("Error writing to file");
    syslog(LOG_ERR, "Error writing to file");
    exit(1);
  }

  syslog(LOG_USER, "Writing %s to %s", writestr, writefile);
  closelog();

  return 0;
}