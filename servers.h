#include <arpa/inet.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_CLIENTS 30

void startTCPSrv(struct sockaddr_in *srvAddr, int *srvFd, unsigned int port) {
  if ((*srvFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("server socket");
    exit(EXIT_FAILURE);
  }

  srvAddr->sin_family = AF_INET;
  srvAddr->sin_addr.s_addr = INADDR_ANY;
  srvAddr->sin_port = htons(port);
  memset(srvAddr->sin_zero, '\0', sizeof srvAddr->sin_zero);

  if (bind(*srvFd, (struct sockaddr *)srvAddr, sizeof(*srvAddr)) < 0) {
    perror("server binding");
    exit(EXIT_FAILURE);
  }

  if (listen(*srvFd, MAX_CLIENTS) < 0) {
    perror("server listening");
    exit(EXIT_FAILURE);
  }
}