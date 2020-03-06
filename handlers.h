#include <arpa/inet.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHUNK_SIZE 256
#define HEADERS_BODY_SEPARATOR "\r\n\r\n"
#define CHUNKED_REQUEST_TERMINATION "0\r\n\r\n"

void handleRequest(int clientFd, int targetFd) {
  char *marker = NULL;
  int n;
  char request[CHUNK_SIZE];
  memset(request, '\0', sizeof request);

  while ((n = read(clientFd, request, sizeof(request) - 1)) > 0) {
    if (write(targetFd, request, n) < 0) {
      perror("writing content from client to target failed - probably "
             "SIGPIPE\n");
    }

    marker = strstr(request, HEADERS_BODY_SEPARATOR);
    memset(request, '\0', sizeof request);
    if (marker != NULL) {
      break;
    }
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