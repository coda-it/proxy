#include "utils.h"
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
#define MAX_REQUEST_SIZE 256
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
  int n;
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
  startTCPSrv(&serverAddress, &serverFd, SERVER_PORT);

  fd_set clientFds;
  int clientFd;
  int newClientFd;
  int maxClientFd = serverFd;
  FD_ZERO(&clientFds);
  FD_SET(serverFd, &clientFds);

  while (1) {
    printf("listening for incomming connections\n");

    if (select(maxClientFd + 1, &clientFds, NULL, NULL, NULL) == -1) {
      perror("error checking file descriptors");
      exit(EXIT_FAILURE);
    }

    for (clientFd = 0; clientFd <= maxClientFd; clientFd++) {
      if (FD_ISSET(clientFd, &clientFds)) {
        if (clientFd == serverFd) {
          if ((newClientFd = accept(serverFd, (struct sockaddr *)&serverAddress,
                                    (socklen_t *)&serverAddrLen)) < 0) {
            perror("client connection accept");
            exit(EXIT_FAILURE);
          }

          FD_SET(newClientFd, &clientFds);
          if (newClientFd > maxClientFd) {
            maxClientFd = newClientFd;
          }
          printf("client-server connection established\n");

        } else {
          struct sockaddr_in targetAddress;
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

          printf("sending request from client to target\n");
          char request[MAX_REQUEST_SIZE];
          memset(request, '\0', sizeof request);

          while ((n = read(clientFd, request, sizeof(request) - 1)) > 0) {
            if (write(targetFd, request, n) < 0) {
              perror("writing content from client to target failed - probably "
                     "SIGPIPE\n");
            }

            char *requestEnd = strstr(request, "\r\n\r\n");
            memset(request, '\0', sizeof request);
            if (requestEnd != NULL) {
              break;
            }
          }

          printf("sending response from target to client\n");
          char response[MAX_REQUEST_SIZE];
          memset(response, '\0', sizeof response);

          int bodyLen = 0;
          int isBody = 0;
          int contentLen = 0;
          char *contentLenStr;
          char *marker;

          while ((n = read(targetFd, response, sizeof(response) - 1)) > 0) {
            marker = strstr(response, "\r\n\r\n");

            if (isBody == 0) {
              contentLenStr = getHeaderVal(response, "Content-Length");
            } else if (isBody == 1) {
              bodyLen += n;
            }

            if (isBody == 0 && marker != NULL) {
              isBody = 1;
              bodyLen += strlen(marker) - 4;
            }

            if (write(clientFd, response, n) < 0) {
              perror("writing content from target to client failed - probably "
                     "SIGPIPE\n");
            }

            if (contentLenStr != NULL) {
              contentLen = atoi(contentLenStr);
              contentLenStr = NULL;
            }

            if (contentLen != 0) {
              if (bodyLen == contentLen) {
                break;
              }
            }

            marker = strstr(response, "0\r\n\r\n");

            if (marker != NULL) {
              break;
            }

            memset(response, '\0', sizeof response);
          }

          printf("request completed\n");
          FD_CLR(clientFd, &clientFds);
          close(clientFd);
          close(targetFd);
        }
      }
    }
  }

  printf("closing proxy\n");

  free(targetIP);
  free(targetPort);

  return 0;
}
