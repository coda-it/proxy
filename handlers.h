#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define CHUNK_SIZE 256
#define HEADERS_BODY_SEPARATOR "\r\n\r\n"
#define REQUEST_TERMINATION "\r\n\r\n"
#define CHUNKED_REQUEST_TERMINATION "0\r\n\r\n"
#define MATCH_HEADER "Host"

void readRequest(int clientFd, char **request) {
  char *marker = NULL;
  int n;
  int isBody = 0;
  char requestChunk[CHUNK_SIZE];
  char *domain;
  int hasDomain = 0;
  int requestSize = 0;

  while ((n = read(clientFd, requestChunk, sizeof(requestChunk) - 1)) > 0) {
    if (*request != NULL) {
      requestSize = strlen(*request);
    }

    *request = realloc(*request, (requestSize + n) * sizeof(char));
    strncpy(*request + requestSize, requestChunk, n);

    marker = strstr(requestChunk, HEADERS_BODY_SEPARATOR);

    if (isBody == 0 && marker != NULL) {
      isBody = 1;
    }

    if (isBody) {
      marker = strstr(requestChunk, REQUEST_TERMINATION);

      if (marker != NULL) {
        break;
      }
    }

    memset(requestChunk, '\0', sizeof requestChunk);
  }

  requestSize = strlen(*request);
  *request = realloc(*request, (requestSize + 1) * sizeof(char));
  strcpy(*request + requestSize, "\0");
}

int parseHeaders(char *sourceDomain, char *request) {
  char *domain;
  int hasDomain = 0;

  domain = getHeaderVal(request, MATCH_HEADER);

  if (domain != NULL && strcmp(domain, sourceDomain) == 0) {
    hasDomain = 1;
  }

  return hasDomain;
}

void connectToTarget(char *targetIP, char *targetPort, int *targetFd) {
  struct sockaddr_in targetAddress;
  targetAddress.sin_family = AF_INET;
  targetAddress.sin_addr.s_addr = inet_addr(targetIP);
  targetAddress.sin_port = htons(atoi(targetPort));
  memset(targetAddress.sin_zero, '\0', sizeof targetAddress.sin_zero);

  if ((*targetFd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("target socket");
    exit(EXIT_FAILURE);
  }

  if (connect(*targetFd, (struct sockaddr *)&targetAddress,
              sizeof(targetAddress)) < 0) {
    perror("target connection");
    exit(EXIT_FAILURE);
  }
}

void forwardRequest(int clientFd, int targetFd, char *request) {
  char *marker = NULL;
  int n;

  if (write(targetFd, request, strlen(request) + 1) < 0) {
    perror("writing content from client to target failed - probably "
           "SIGPIPE\n");
  }
}

void handleResponse(int clientFd, int targetFd) {
  char *marker = NULL;
  int n;
  int bodyLen = 0;
  int isBody = 0;
  int contentLen = 0;
  char *contentLenStr;

  char response[CHUNK_SIZE];
  memset(response, '\0', sizeof response);

  while ((n = read(targetFd, response, sizeof(response) - 1)) > 0) {
    marker = strstr(response, HEADERS_BODY_SEPARATOR);

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

    marker = strstr(response, CHUNKED_REQUEST_TERMINATION);

    if (marker != NULL) {
      break;
    }

    memset(response, '\0', sizeof response);
  }
}