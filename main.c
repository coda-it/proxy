#include "version.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define CONFIG_FILE "./proxy.db"
#define IP_PORT_SEPARATOR ":"
#define SERVER_PORT 80
#define RESPONSE_SIZE 2000
#define MAX_CLIENTS 30

int main(int argc, char const *argv[]) {
  signal(SIGPIPE, SIG_IGN);

  printf("starting proxy %s\n", VERSION);

  printf("reading proxy config file\n");
  FILE *cnfPtr;
  cnfPtr = fopen(CONFIG_FILE, "r");

  if (cnfPtr == NULL) {
    perror("proxy.db is not defined");
    exit(EXIT_FAILURE);
  }

  int i = 0;
  char c;
  char *str;

  fseek(cnfPtr, 0L, SEEK_END);
  long cnfSize = ftell(cnfPtr);
  rewind(cnfPtr);
  char *cnf;
  cnf = malloc(sizeof *cnf * cnfSize);
  memset(cnf, '\0', sizeof *cnf * cnfSize);

  while (c != EOF) {
    c = fgetc(cnfPtr);
    cnf[i] = c;

    if (c == EOF) {
      cnf[i] = '\0';
    }

    i++;
  }
  fclose(cnfPtr);

  char *targetIP;
  char *targetPort;

  i = 0;

  str = strtok(cnf, IP_PORT_SEPARATOR);
  while (i < 2) {
    if (i == 0) {
      targetIP = malloc(sizeof *targetIP * strlen(str));
      targetIP = str;
    } else {
      targetPort = malloc(sizeof *targetPort * strlen(str));
      targetPort = str;
    }

    str = strtok(NULL, IP_PORT_SEPARATOR);
    i++;
  }

  i = 0;
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

  if (listen(serverFd, MAX_CLIENTS) < 0) {
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
    targetAddress.sin_addr.s_addr = inet_addr(targetIP);
    targetAddress.sin_port = htons(atoi(targetPort));
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
        printf("writing content to client failed - probably SIGPIPE\n");
      }
    }

    close(clientFd);
    close(targetFd);
  }

  printf("closing proxy\n");

  free(targetIP);
  free(targetPort);

  return 0;
}
