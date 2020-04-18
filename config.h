#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IP_PORT_SEPARATOR ":"

void readConfig(char *filePath, char **sourceDomain, char **targetIP,
                char **targetPort) {
  printf("reading proxy config file\n");
  FILE *cnfPtr;
  cnfPtr = fopen(filePath, "r");

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

  i = 0;

  str = strtok(cnf, IP_PORT_SEPARATOR);
  while (i < 3) {
    if (i == 0) {
      *sourceDomain = malloc(sizeof **sourceDomain * strlen(str));
      *sourceDomain = str;
    } else if (i == 1) {
      *targetIP = malloc(sizeof **targetIP * strlen(str));
      *targetIP = str;
    } else {
      *targetPort = malloc(sizeof **targetPort * strlen(str));
      *targetPort = str;
    }

    str = strtok(NULL, IP_PORT_SEPARATOR);
    i++;
  }
}