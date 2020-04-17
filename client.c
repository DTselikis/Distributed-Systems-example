#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

int clientInitialize(char *host, unsigned int port);
void client(int argc, char **argv);

int *menu(unsigned int *choice, unsigned int *numOfElements, float *floatNum, int *previousArray, unsigned short *flag, unsigned short *floatFlag);
char *numArrayToCharArray(void *array, unsigned int numOfElements, char delimeter, unsigned int *length, unsigned short int mode);
char *compileStrArray(char **strArray, unsigned int length, unsigned int numOfElements, char delimeter);
float *strToFloatArray(char *floatBuffer, unsigned int numOfElements);

FILE *initFile(char *fName);
int *readFromFile(unsigned int *numOfElements, float *floatNum, FILE *fPointer);

int main (int argc, char *argv[]) {

  if (argc < 3) {
    fprintf(stderr, "Usage: ./client host port <file>\n");
    exit(1);
  }

  client(argc, argv);

  return 0;
}

void client(int argc, char **argv) {
  int server_discr; // Client's socket

  // Socket variables
  unsigned int choiceNet;
  unsigned int choice;
  int *numArrayNet;
  int *numArray;
  unsigned int numOfElementsNet;
  unsigned int numOfElements;
  char *floatNumStr;
  float floatNum;
  char *averageStr;
  float average;
  unsigned int length;
  int *minMaxNet;
  int *minMax;
  char *muledArrayStr;
  float *muledArray;

  // Helping variables
  unsigned int i;
  unsigned short flag = 0;
  unsigned short floatFlag = 0;

  server_discr = clientInitialize(argv[1], atoi(argv[2]));

  fprintf(stdout, COLOR_GREEN "Connection with server established!\n" COLOR_RESET);

  if (argc < 4) {
    numArray = menu(&choice, &numOfElements, &floatNum, NULL, &flag, &floatFlag);
  }
  else {
    numArray = readFromFile(&numOfElements, &floatNum, initFile(argv[3]));
    flag = 1;
    floatFlag = 1;
    numArray = menu(&choice, &numOfElements, &floatNum, numArray, &flag, &floatFlag);
  }

  // Program will terminate if user's input is number zero
  while (choice) {
    fprintf(stdout, COLOR_YELLOW "Sending message to server...\n" COLOR_RESET);

    choiceNet = htonl(choice);
    send(server_discr, &choiceNet, sizeof(unsigned int), 0);

    // Send data to socket server
    switch(choice) {
      case 3: {
        // Floating point numbers are send as char* array
        // to ensure compatibility
        floatNumStr = (char *)malloc(100 * sizeof(char));
        sprintf(floatNumStr, "%f", floatNum);
        length = strlen(floatNumStr) + 1;

        send(server_discr, &length, sizeof(unsigned int), 0);
        send(server_discr, floatNumStr, length, 0);

        free(floatNumStr);
      }
      case 2:
      case 1: {
        // integers are converted to network byte order
        // to ensure compatibility
        int res;
        numOfElementsNet = htonl(numOfElements);
        numArrayNet = (int *)malloc(numOfElements * sizeof(int));
        for (i = 0; i < numOfElements; i++) {
          numArrayNet[i] = htonl(numArray[i]);
        }

        send(server_discr, &numOfElementsNet, sizeof(unsigned int), 0);
        res = send(server_discr, numArrayNet, numOfElements * sizeof(int), 0);

        free(numArrayNet);
      }
    }

    // Receive data from socket server
    switch(choice) {
      case 1: {
        recv(server_discr, &length, sizeof(unsigned int), 0);
        averageStr = (char *)malloc(length * sizeof(char));
        recv(server_discr, averageStr, length, 0);
        average = atof(averageStr);

        fprintf(stdout, "Average: %5.2f\n", average);

        break;
      }
      case 2: {
        minMaxNet = (int *)malloc(2 * sizeof(int));
        recv(server_discr, minMaxNet, 2 * sizeof(int), 0);

        minMax = (int *)malloc(2 * sizeof(int));
        minMax[0] = ntohl(minMaxNet[0]);
        minMax[1] = ntohl(minMaxNet[1]);

        fprintf(stdout, "Min: %d, Max: %d\n", minMax[0], minMax[1]);

        free(minMax);
        free(minMaxNet);

        break;
      }
      case 3: {
        recv(server_discr, &length, sizeof(unsigned int), 0);
        muledArrayStr = (char *)malloc(length * sizeof(char));

        recv(server_discr, muledArrayStr, length, 0);

        muledArray = strToFloatArray(muledArrayStr, numOfElements);

        for (i = 0; i < numOfElements; i++) {
          fprintf(stdout, "Element %d: %f\n", i + 1, muledArray[i]);
        }

        free(muledArray);
        free(muledArrayStr);
      }
    }

    fprintf(stdout, COLOR_YELLOW "Response from server received\n" COLOR_RESET);

    numArray = menu(&choice, &numOfElements, &floatNum, numArray, &flag, &floatFlag);
  }

  fprintf(stdout, COLOR_YELLOW "Sending closing message to server...\n" COLOR_RESET);
  send(server_discr, &choice, sizeof(unsigned int), 0);
  close(server_discr); // Closes the server's socket
  fprintf(stdout, COLOR_GREEN "Connection to server closed successfully!\n" COLOR_RESET);

  free(numArray);
}

/**
 * Client's socket initialization procedure
 * Returns the clients's socket up and running
*/
int clientInitialize(char *host, unsigned int port) {
  int server_discr;
  struct sockaddr_in serverSockAddr;
  struct hostent *server;

  // Creates a socket. This will be the client's socket
  if ((server_discr = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error at creating socket");
    exit(1);
  }

  server = gethostbyname(host);
  if (server == NULL) {
      fprintf(stderr,"ERROR, no such host\n");
      exit(0);
  }

  serverSockAddr.sin_family = AF_INET;
  bcopy((char *)server->h_addr,
       (char *)&serverSockAddr.sin_addr.s_addr,
       server->h_length);
  serverSockAddr.sin_port = htons(port);

  // Makes the connection with server's socket
  if (connect(server_discr, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr)) == -1) {
    perror("Error at connecting to the server");
    exit(2);
  }

  return server_discr;
}

int *menu(unsigned int *choice, unsigned int *numOfElements, float *floatNum, int *previousArray, unsigned short *flag, unsigned short *floatFlag) {
  int *numArray = previousArray;
  unsigned int i;
  unsigned int newInputFlag;

  fprintf(stdout, "Select operation:\n1: Find average\n"
  "2: Find minimum and maximum number\n"
  "3: Matrix multiplication\n"
  "0: Exit\nChoice: ");
  scanf("%d", choice);

  // Ask to change input only in the case that:
  // 1. no previous input was provided
  // 2. option 3 is not the first option of the user so
  // and so value for floatNum is needed
  if (*flag && *choice != 0) {
    if (*choice == 3 && *floatFlag == 0) {
      newInputFlag = 0;
    }
    else {
      fprintf(stdout, "Keep the same numbers? (1 = yes 0 = no)\nChoice: ");
      scanf("%d", &newInputFlag);
    }
  }
  else if (*choice == 0) {
    newInputFlag = 1;
  }

  if (newInputFlag == 0 || *flag == 0) {
    if (previousArray != NULL) free(numArray);
    *flag = 1;
    switch(*choice) {
      case 3: {
        fprintf(stdout, "\nEnter floating number: ");
        scanf("%f", floatNum);
        *floatFlag = 1;
      }
      case 2:
      case 1: {
        fprintf(stdout, "\nEnter number of elements: ");
        scanf("%d", numOfElements);

        numArray = (int *)malloc(*numOfElements * sizeof(int));
        for (i = 0; i < *numOfElements; i++) {
          fprintf(stdout, "\nEnter element %d: ", i + 1);
          scanf("%d", &numArray[i]);
        }
      }
    }
  }

  return numArray;
}

/**
  * Converts an int or float array to a char* array
  * that contains the same numbers.
  * This function seperates each number and converts it to char*.
  * compileStrArray compile all parts to one char*
*/
char *numArrayToCharArray(void *voidArray, unsigned int numOfElements, char delimeter, unsigned int *length, unsigned short int mode) {
	char **strArray = (char **)malloc(numOfElements * sizeof(char *));
	unsigned int i;
	char *buffer;
  float *floatArray;
  int *intArray;

  if (mode) {
    intArray = (int *) voidArray;
  }
  else {
    floatArray = (float *) voidArray;
  }

	*length = 0;
	for (i = 0; i < numOfElements; i++) {
    strArray[i] = (char *)malloc(100 * sizeof(char));
    if(mode) {
      sprintf(strArray[i], "%d", intArray[i]);
    }
    else {
      sprintf(strArray[i], "%f", floatArray[i]);
    }
    *length += strlen(strArray[i]);
	}

	// numOfElements '\0' + numOfElements spaces that will separate
	// each number (last space will be used as '\0')
	*length += numOfElements;

	buffer = compileStrArray(strArray, *length, numOfElements, delimeter);

  for (i = 0; i < numOfElements; i++) {
    free(strArray[i]);
  }
	free(strArray);

	return buffer;
}

/**
  * Combine all char* numbers to a single char* array.
*/
char *compileStrArray(char **strArray, unsigned int length, unsigned int numOfElements, char delimeter) {
	char *buffer = (char *)malloc(length * sizeof(char));
	unsigned int i;
	unsigned int pos;

	pos = 0;
	for (i = 0; i < numOfElements; i++) {
		strncpy(buffer + pos, strArray[i], strlen(strArray[i]));
		pos += strlen(strArray[i]);
    // If there is no other element, terminate the string
		(i == numOfElements - 1) ? (buffer[pos] = '\0') : (buffer[pos] = delimeter);
		pos++;
	}

	return buffer;
}

/**
  * Converts a char* that contains one or more numbers
  * to a float array.
*/
float *strToFloatArray(char *floatBuffer, unsigned int numOfElements) {
	float *floatArray = (float *)malloc(numOfElements * sizeof(float));
	char *tmpStr = (char *)malloc(100 * sizeof(char));
	unsigned int i;

	i = 0;
	tmpStr = strtok(floatBuffer, " ");
	while (tmpStr != NULL) {
		floatArray[i++] = atof(tmpStr);
		tmpStr = strtok(NULL, " ");
	}

	free(tmpStr);

	return floatArray;
}

/**
  * Open file for reading.
  * Returns a pointer to the opened file
*/
FILE *initFile(char *fName) {
	FILE *fPointer;

	fPointer = fopen(fName, "r");

	if (fPointer == NULL) {
		fprintf(stderr, "\nFile \"%s\" not found!\n", fName);
		exit(2);
	}

	return fPointer;
}

/**
  * Input from file
*/
int *readFromFile(unsigned int *numOfElements, float *floatNum, FILE *fPointer) {
	unsigned int i;
  int *numArray;

  fscanf(fPointer, "%f", floatNum);
	fscanf(fPointer, "%d", numOfElements);

  numArray = (int *)malloc(*numOfElements * sizeof(int));
  for (i = 0; i < *numOfElements; i++) {
    fprintf(stdout, "\nEnter element: %d", i + 1);
    fscanf(fPointer, "%d", &numArray[i]);
  }

  fprintf(stdout, "\n");
	fclose(fPointer);

	return numArray;
}
