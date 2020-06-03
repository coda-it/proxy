#include "config.h"
#include "handlers.h"
#include "servers.h"
#include "version.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CONFIG_FILE "./proxy.db"
#define SERVER_PORT 80

int main(int argc, char const *argv[]) {
  signal(SIGPIPE, SIG_IGN);

  printf("starting proxy %s\n", VERSION);

  char *sourceDomain;
  char *targetIP;
  char *targetPort;
  readConfig(CONFIG_FILE, &sourceDomain, &targetIP, &targetPort);

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
          printf("forwarding request with the following parameters:\n");
          printf("- domain: %s\n", sourceDomain);
          printf("- targetIP: %s\n", targetIP);
          printf("- targetPort: %s\n", targetPort);

          int targetFd;
          char *request = NULL;

          printf("determining target for request\n");
          readRequest(clientFd, &request);
          int hasTarget = parseHeaders(sourceDomain, request);

          if (hasTarget) {
            printf("connecting to target\n");
            connectToTarget(targetIP, targetPort, &targetFd);

            printf("sending request from client to target\n");
            forwardRequest(clientFd, targetFd, request);

            printf("sending response from target to client\n");
            handleResponse(clientFd, targetFd);

            printf("request completed\n");
            close(targetFd);
          }

          free(request);
          FD_CLR(clientFd, &clientFds);
          close(clientFd);
        }
      }
    }
  }

  printf("closing proxy\n");

  free(targetIP);
  free(targetPort);

  return 0;
}
