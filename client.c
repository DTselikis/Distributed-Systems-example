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

unsigned int menu(char **numArray, unsigned int *numOfElements, unsigned int *length, float *floatNum);
char *numArrayToCharArray(void *array, unsigned int numOfElements, char delimeter, unsigned int *length, unsigned short int mode);
char *compileStrArray(char **strArray, unsigned int length, unsigned int numOfElements, char delimeter);
float *strToFloatArray(char *floatBuffer, unsigned int numOfElements);

FILE *initFile(char *fName);
unsigned int readFromFile(char **numArray, unsigned int *length, float *floatNum, FILE *fPointer);

int main (int argc, char *argv[]) {

  client(argc, argv);

  return 0;
}

void client(int argc, char **argv) {
  int client_discr; // Client's socket
  char *buffer;
  unsigned int choice;

  char *numArray;
  unsigned int numOfElements;
  unsigned int length;
  float floatNum;

  float average;
  float *minMax;
  float *muledArray;
  unsigned int i;

  client_discr = clientInitialize(argv[1], atoi(argv[2]));

  fprintf(stdout, COLOR_GREEN "Connection with server established!\n" COLOR_RESET);

  if (argc < 4) {
    choice = menu(&numArray, &numOfElements, &length, &floatNum);
  }
  else {
    numOfElements = readFromFile(&numArray, &length, &floatNum, initFile(argv[4]));
    fprintf(stdout, "Option:1\n2\n3\n");
    scanf("%d", &choice);
  }

  // Program will terminate if user's input is number zero
  while (choice) {
    fprintf(stdout, COLOR_YELLOW "Sending message to server...\n" COLOR_RESET);

    send(client_discr, &choice, sizeof(unsigned int), 0);

    switch(choice) {
      case 3: {
        send(client_discr, &floatNum, sizeof(float), 0);
      }
      case 2:
      case 1: {
        int num = htonl(numOfElements);
        int len = htonl(length);
        send(client_discr, &num, sizeof(num), 0);
        send(client_discr, &len, sizeof(len), 0);
        send(client_discr, numArray, length, 0);
      }
    }

    switch(choice) {
      case 1: {
        recv(client_discr, &average, sizeof(float), 0);

        fprintf(stdout, "Average: %f", average);

        break;
      }
      case 2: {
        recv(client_discr, &length, sizeof(unsigned int), 0);

        buffer = (char *)malloc(length * sizeof(char));

        recv(client_discr, buffer, length, 0);

        minMax = strToFloatArray(buffer, 2);

        // TODO fix type
        fprintf(stdout, "Min: %f, Max: %f", minMax[0], minMax[1]);

        free(buffer);

        break;
      }
      case 3: {
        recv(client_discr, &length, sizeof(unsigned int), 0);

        buffer = (char *)malloc(length * sizeof(char));

        recv(client_discr, buffer, length, 0);

        muledArray = strToFloatArray(buffer, numOfElements);

        for (i = 0; i < numOfElements; i++) {
          fprintf(stdout, "Element %d: %f\n", i + 1, muledArray[i]);
        }
        free(buffer);
        free(muledArray);
      }
    }

    fprintf(stdout, COLOR_YELLOW "Response from server received\n" COLOR_RESET);

    // Adding string termination character because is missing
    // (strlen from server-side doesn't count the '\0' so the messages
    // that arrive don't include it
    //buffer[recievedMsgSize] = '\0';

    choice = menu(&numArray, &numOfElements, &length, &floatNum);
  }

  fprintf(stdout, COLOR_YELLOW "Sending close to server...\n" COLOR_RESET);
  // Covers the case that user enter '\n' from the beginning.
  send(client_discr, &choice, sizeof(unsigned int), 0);
  close(client_discr); // Closes the client's socket
  fprintf(stdout, COLOR_GREEN "Connection to server closed successfully!\n" COLOR_RESET);

  free(numArray);
}

/**
 * Client's socket initialization procedure
 * Returns the clients's socket up and running
*/
int clientInitialize(char *host, unsigned int port) {
  int client_discr;
  struct sockaddr_in serverSockAddr;
  struct hostent *server;

  // Creates a socket. This will be the client's socket
  if ((client_discr = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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
  if (connect(client_discr, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr)) == -1) {
    perror("Error at connecting to the server");
    exit(2);
  }

  return client_discr;
}

unsigned int menu(char **numArray, unsigned int *numOfElements, unsigned int *length, float *floatNum) {
  unsigned int choice;
  unsigned int i;
  int *array;
  static unsigned char flag = 0;
  unsigned int newInputFlag;

  fprintf(stdout, "Select operation:\n1.\n2.\n3\n.");
  scanf("%d", &choice);

  if (flag) {
    fprintf(stdout, "Keep the same numbers?");
    scanf("%d", &newInputFlag);
  }

  if (newInputFlag == 'y' || flag == 0) {
    flag = 1;
    switch(choice) {
      case 3: {
        fprintf(stdout, "\nEnter floating number: ");
        scanf("%f", floatNum);
      }
      case 2:
      case 1: {
        fprintf(stdout, "\nEnter number of elements: ");
        scanf("%d", numOfElements);

        array = (int *)malloc(*numOfElements * sizeof(int));

        for (i = 0; i < *numOfElements; i++) {
          fprintf(stdout, "\nEnter element %d: ", i + 1);
          scanf("%d", &array[i]);
        }

        *numArray = numArrayToCharArray((void *) array, *numOfElements, ' ', length, 1);
      }
    }
  }

  free(array);

  return choice;
}

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

char *compileStrArray(char **strArray, unsigned int length, unsigned int numOfElements, char delimeter) {
	char *buffer = (char *)malloc(length * sizeof(char));
	unsigned int i;
	unsigned int pos;

	pos = 0;
	for (i = 0; i < numOfElements; i++) {
		strncpy(buffer + pos, strArray[i], strlen(strArray[i]));
		pos += strlen(strArray[i]);
		(i == numOfElements - 1) ? (buffer[pos] = '\0') : (buffer[pos] = delimeter);
		pos++;
	}

	return buffer;
}

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

// Open file for reading operation
// Returns a pointer to the opened file
FILE *initFile(char *fName) {
	FILE *fPointer;

	fPointer = fopen(fName, "r");

	if (fPointer == NULL) {
		fprintf(stderr, "\nFile \"%s\" not found!\n", fName);
		exit(2);
	}

	return fPointer;
}

// File input
// Returns number of elements read and an 1-D array of these elements
unsigned int readFromFile(char **numArray, unsigned int *length, float *floatNum, FILE *fPointer) {
	unsigned int numOfElements;
	unsigned int i;
  int *array;

  fscanf(fPointer, "%f", floatNum);
	fscanf(fPointer, "%d", &numOfElements);

  array = (int *)malloc(numOfElements * sizeof(int));
  for (i = 0; i < numOfElements; i++) {
    fprintf(stdout, "\nEnter element: %d", i + 1);
    fscanf(fPointer, "%d", &array[i]);
  }

  *numArray = numArrayToCharArray((void *) array, numOfElements, ' ', length, 1);

	fclose(fPointer);
  free(array);

	return numOfElements;
}
