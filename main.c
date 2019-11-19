#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "version.h"

#define CONNECTIONS_BACKLOG 1
#define SERVER_PORT 80
#define TARGET_PORT 3223
#define RESPONSE_SIZE 2000

int main(int argc, char const *argv[]) {
  printf("starting proxy %s\n", VERSION);

  struct sockaddr_in serverAddress;
  int serverAddrLen = sizeof(serverAddress);
  int serverFd;

  if ((serverFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("server socker");
    exit(EXIT_FAILURE);
  }

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_addr.s_addr = INADDR_ANY;
  serverAddress.sin_port = htons(SERVER_PORT);
  memset(serverAddress.sin_zero, '\0', sizeof serverAddress.sin_zero);

  if (bind(serverFd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) <
      0) {
    perror("server binding");
    exit(EXIT_FAILURE);
  }

  if (listen(serverFd, CONNECTIONS_BACKLOG) < 0) {
    perror("server listening");
    exit(EXIT_FAILURE);
  }

  while (1) {
    int clientFd;

    printf("waiting for client connection\n");
    if ((clientFd = accept(serverFd, (struct sockaddr *)&serverAddress,
                           (socklen_t *)&serverAddrLen)) < 0) {
      perror("client connection accept");
      exit(EXIT_FAILURE);
    }

    printf("client-server connection established\n");

    struct sockaddr_in targetAddress;
    int targetAddLen = sizeof(targetAddress);
    int targetFd;

    targetAddress.sin_family = AF_INET;
    targetAddress.sin_addr.s_addr = inet_addr("173.194.217.94");
    targetAddress.sin_port = htons(80);
    memset(targetAddress.sin_zero, '\0', sizeof targetAddress.sin_zero);

    if ((targetFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
      perror("target socket");
      exit(EXIT_FAILURE);
    }

    if (connect(targetFd, (struct sockaddr *)&targetAddress,
                sizeof(targetAddress)) < 0) {
      perror("target connection");
      exit(EXIT_FAILURE);
    }

    char request[] = "GET /\n\r";

    if (write(targetFd, request, strlen(request)) < 0) {
      perror("sending data to target");
    }

    char response[RESPONSE_SIZE];
    int n;

    while ((n = read(targetFd, response, RESPONSE_SIZE)) > 0) {
      if (write(clientFd, response, n) < 0) {
        perror("writing data to client");
        exit(EXIT_FAILURE);
      }
    }

    close(clientFd);
    close(targetFd);
  }

  printf("closing proxy\n");
  return 0;
}
