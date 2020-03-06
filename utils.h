#include <arpa/inet.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HEADER_SEPARATOR " :\n\r"

char *getHeaderVal(char *request, char *header) {
  char *body;
  char *params;

  char *requestCpy = calloc(strlen(request) + 1, sizeof(char));
  strcpy(requestCpy, request);

  body = strtok(requestCpy, HEADER_SEPARATOR);

  while (1) {
    if (body == NULL) {
      break;
    }

    if (strcmp(header, body) == 0) {
      return strtok(NULL, HEADER_SEPARATOR);
    }
    body = strtok(NULL, HEADER_SEPARATOR);
  }

  free(requestCpy);

  return NULL;
}
