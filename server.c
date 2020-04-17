#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX_CONN 128
// According to manual of listen(2), the default number of maximum
// socket connections is 128, unless we provide something bigger. We
// make sure that at least this amount of connection can be handled

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

void *clientThread(void *arg);
int serverInitialize(unsigned int port);
short int checkAvailability(unsigned short int *threadsState, unsigned int size);
void server(unsigned int port);

int *strToIntArray(char *floatBuffer, unsigned int numOfElements);
float findAverage(int *numArray, int numOfElements);
int *findMinMax(int *numArray, unsigned int numOfElements);
float *mulMatrixWithFloat(int *numArray, unsigned int numOfElements, float multiplier);
char *numArrayToCharArray(void *voidArray, unsigned int numOfElements, char delimeter, unsigned int *length, unsigned short int mode);
char *compileStrArray(char **strArray, unsigned int length, unsigned int numOfElements, char delimeter);

// Information that client's thread needs
struct threadArgs {
  int client_discr;
  unsigned short int *finished;
};


int main (int argc, char *argv[]) {

  server(atoi(argv[1]));

  return 0;
}

/**
 * Instance of a server.
 * After calling "serverInitialize()" to create it's socket, server constantly
 * listening to that socket for incoming connections.
 * If a connection successfully establish, it checks if there are available
 * resources (a non-busy thread slot) and if there is, the program create a new
 * process, dedicated to this client
 * The server also creates a thread that handles logging operations.
 * All threads are set as detachables.
 * Server check for new connections every 2 seconds
*/
void server(unsigned int port) {
  int server_discr, client_discr; // File descriptors for sockets
  unsigned int clientSockSize;
  unsigned short int threadsState[MAX_CONN] = {0}; // Number of threads slots
  short int freeResource; // Helping variable
  struct sockaddr_in clientSockAddr;
  struct threadArgs passingArgs[MAX_CONN]; // Structs for passing important information to client's thread
  pthread_t thread[MAX_CONN]; // Number of threads
  pthread_attr_t attr; // Threads attributes

  pthread_attr_init(&attr); // Initialize attributes
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

  server_discr = serverInitialize(port);

  // Server runs infintly
  while (1) {
    fprintf(stdout, "Waiting for a connection...\n");
    clientSockSize = sizeof(clientSockAddr);
    // Extracts the first connection request on the queue of pending connections
    // for the listening socket, creates a new connected socket, and returns
    // a new file descriptor referring to that socket (client).
    // It has been set as non blocking, so the rest of the program can run uninterrupted
    if ((client_discr = accept(server_discr, (struct sockaddr *)&clientSockAddr, &clientSockSize)) != -1) {
      // Checks if threre is an available thread slot to serve the new client
      fprintf(stdout, COLOR_GREEN "Connection with client [%d] established.\n"
              COLOR_CYAN "Checking for available resources...\n" COLOR_RESET, client_discr);
      if ((freeResource = checkAvailability(threadsState, MAX_CONN)) != -1) {
        passingArgs[freeResource].client_discr = client_discr; // Client's socket. It's just a number so there's no need to pass it b reference
        passingArgs[freeResource].finished = &threadsState[freeResource]; // Pointer to the address of thread's slot that will server this client
        pthread_create(&(thread[freeResource]), &attr, clientThread, (void *) &passingArgs[freeResource]);
        fprintf(stdout, COLOR_CYAN "Resources found. Thread for client [%d] created successfully!\n" COLOR_RESET, client_discr);
      }
      else { // No free slots available
        fprintf(stdout, COLOR_RED "No available resources for client [%d]\n" COLOR_RESET, client_discr);
      }
    }
    sleep(2); // Server check for new connections every 2 seconds
  }

}

/**
 * Each call to this function represents a new thread that
 * serves independently one (and only one) client.
 * The function is responsible for the communication with the
 * client (receive and send messages) and also for updating
 * the two global variables "totalCalculations" and "totalPassedCalculations"
 * and signaling the logging thread.
*/
void *clientThread(void *arg) {
  // Typecasting the void argument to something we can handle
  struct threadArgs *passedArgs = (struct threadArgs *) arg;

  unsigned int numOfElementsNet;
  unsigned int numOfElements;
  float multiplierNet;
  float multiplier;
  int *numArrayNet;
  int *numArray;
  unsigned int choiceNet;
  unsigned int choice;

  char* averageStr;
  unsigned int length;
  float average;
  int *minMaxNet;
  int *minMax;
  float *muledMatrixNet;
  float *muledMatrix;
  unsigned int i;


  // Receive messages from a socket.
  // If no messages are available at the socket, it blocks the execution
  // of the thread until a message is available (in non-blocking state)
  fprintf(stdout, COLOR_YELLOW "Waiting for message from client [%d]...\n" COLOR_RESET, passedArgs->client_discr);
	recv(passedArgs->client_discr, &choiceNet, sizeof(unsigned int), 0);
  choice = ntohl(choiceNet);
  printf("choice%d\n", choice);
  fprintf(stdout, COLOR_BLUE "Message from client [%d] recieved\n" COLOR_RESET, passedArgs->client_discr);
  // 0 indicates that client don't want to continue the communitcation
  while (choice) {
	   // Receive data from client
  	switch(choice) {
  		case 3: {
  			recv(passedArgs->client_discr, &multiplierNet, sizeof(float), 0);

        multiplier = ntohl(multiplierNet);
  		}
  		case 2:
  		case 1: {
        int res;
  			recv(passedArgs->client_discr, &numOfElementsNet, sizeof(unsigned int), 0);
        numOfElements = ntohl(numOfElementsNet);

  			numArrayNet = (int *)malloc(numOfElements * sizeof(int));
  			res = recv(passedArgs->client_discr, numArrayNet, numOfElements * sizeof(int), 0);

  			numArray = (int *)malloc(numOfElements * sizeof(int));

        for (i = 0; i < numOfElements; i++) {
          numArray[i] = ntohl(numArrayNet[i]);
          printf("%d\n", numArray[i]);
        }

        free(numArrayNet);
  		}
  	}

  	switch(choice) {
  		case 1: {
  			average = findAverage(numArray, numOfElements);
        averageStr = (char *)malloc(100 * sizeof(char*));
        sprintf(averageStr, "%f", average);
        length = strlen(averageStr) + 1;
        printf("str%s\n", averageStr);

  			fprintf(stdout, COLOR_YELLOW "Sending message to client [%d]...\n" COLOR_RESET, passedArgs->client_discr);
        send(passedArgs->client_discr, &length, sizeof(unsigned int), 0);
        send(passedArgs->client_discr, averageStr, length, 0);

        free(averageStr);
        break;
  		}
  		case 2: {
  			minMax = findMinMax(numArray, numOfElements);
        minMaxNet = (int *)malloc(2 * sizeof(int));
        minMaxNet[0] = htonl(minMax[0]);
        minMaxNet[1] = htonl(minMax[1]);

  			fprintf(stdout, COLOR_YELLOW "Sending message to client [%d]...\n" COLOR_RESET, passedArgs->client_discr);
        send(passedArgs->client_discr, minMaxNet, 2 * sizeof(int), 0);

  			free(minMax);
  			free(minMaxNet);

        break;
  		}
  		case 3: {
  			muledMatrix = mulMatrixWithFloat(numArray, numOfElements, multiplier);
        muledMatrixNet = (float *)malloc(numOfElements * sizeof(float));
        for (i = 0; i < numOfElements; i++) {
          muledMatrixNet[i] = htonl(muledMatrix[i]);
        }

  			fprintf(stdout, COLOR_YELLOW "Sending message to client [%d]...\n" COLOR_RESET, passedArgs->client_discr);
        send(passedArgs->client_discr, muledMatrixNet, numOfElements * sizeof(float), 0);

  			free(muledMatrix);
  			free(muledMatrixNet);
  		}
  	}

  	free(numArray);

    fprintf(stdout, COLOR_YELLOW "Waiting for message from client [%d]...\n" COLOR_RESET, passedArgs->client_discr);
    // Waiting for another message
    recv(passedArgs->client_discr, &choiceNet, sizeof(unsigned int), 0);
    choice = ntohl(choiceNet);
  }

  *passedArgs->finished = 0; // Set this thread's slot as free
  fprintf(stdout, COLOR_GREEN "Connection with client [%d] closed successfully!\n" COLOR_RESET, passedArgs->client_discr);
  close(passedArgs->client_discr); // Closes the connection with the client's socket
  pthread_exit(NULL);  // Terminates the thread
}

/**
 * Server's socket initialization procedure
 * Returns the server's socket up and running
*/
int serverInitialize(unsigned int port) {
  int server_discr;
  struct sockaddr_in servAddr;

  // Creates a socket. This will be the server's socket
  if ((server_discr = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error at creating socket");
    exit(1);
  }
  // fcntl used for flaging socket (file descriptor) as non-Blocking
  // so the other threads can run independently
  fcntl(server_discr, F_SETFL, O_NONBLOCK);

	servAddr.sin_family = AF_INET; // Specify IPv4 Internet protocols
	servAddr.sin_port = htons(port); // Convert bytes to network order
	servAddr.sin_addr.s_addr = INADDR_ANY; // Any address for binding

  // Assigns the address specified by localSockAddr to the server's socket
  if (bind(server_discr, (struct sockaddr *)&servAddr, sizeof(servAddr)) == -1) {
    perror("Error at binding");
    exit(2);
  }

  // Marks server's socket as a socket that will be used to
  // accept incoming connection requests using accept() (passive socket)
  // The second argument specifies the maximum connections that this server
  // will allow. If the number is less than the number specified at
  // "/proc/sys/net/core/somaxconn" then listen takes it's default value (128)
  if (listen(server_discr, 10) == -1) {
    perror("Error at listening");
    exit(3);
  }

  return server_discr;
}


int *strToIntArray(char *floatBuffer, unsigned int numOfElements) {
	int *intArray = (int *)malloc(numOfElements * sizeof(int));
	char *tmpStr = (char *)malloc(100 * sizeof(char));
	unsigned int i;

	i = 0;
	tmpStr = strtok(floatBuffer, " ");
	while (tmpStr != NULL) {
		intArray[i++] = atof(tmpStr);
		tmpStr = strtok(NULL, " ");
	}

	free(tmpStr);

	return intArray;
}

/**
 * Find the average value of the numbers that client has sent
*/
float findAverage(int *numArray, int numOfElements) {
	unsigned int sum = 0;
	unsigned int i;

	for (i = 0; i < numOfElements; sum += numArray[i++])
  ;

  // Considering the chance of dividing by 0
  // Typecast to float to get float number
  return (numOfElements > 0) ? sum / (float) numOfElements : 0;
}

int *findMinMax(int *numArray, unsigned int numOfElements) {
	int *minMax = (int *)malloc(2 * sizeof(int));
	unsigned int i;
	int min;
	int max;

	min = numArray[0];
	max = numArray[0];
	for (i = 1; i < numOfElements; i++) {
		if (min > numArray[i]) {
			min = numArray[i];
		}
		if (max < numArray[i]) {
			max = numArray[i];
		}
	}

	minMax[0] = min;
	minMax[1] = max;

	return minMax;
}

float *mulMatrixWithFloat(int *numArray, unsigned int numOfElements, float multiplier) {
	float *muledMatrix = (float *)malloc(numOfElements * sizeof(float));
	unsigned int i;

	for (i = 0; i < numOfElements; i++) {
		muledMatrix[i] = numArray[i] * multiplier;
	}

	return muledMatrix;
}

char *numArrayToCharArray(void *voidArray, unsigned int numOfElements, char delimeter, unsigned int *length, unsigned short int mode) {
	char **strArray;
	strArray = (char **)malloc(numOfElements * sizeof(char *));
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

/**
 * Scans the array which contains the thread slots
 * to find if there is an available slot for a new thread.
 * If there is, marks this slot as "in use" and returns it.
 * Otherwise returns -1
*/
short int checkAvailability(unsigned short int *threadsState, unsigned int size) {
  unsigned int i;
  i = 0;
  while (i < size) {
    if (threadsState[i] == 0) {
      threadsState[i] = 1; // Mark this slot as unavailable
      return i;
    }
    i++;
  }

  return -1;
}
