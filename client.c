#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdbool.h>

#define TRUE 1
#define FALSE 0

#define IDENTICAL 0

/* Global constants. */
int PARAMETERS_REQUIRED = 5;
int MAX_SIZE = 256;
char EOL = 10;

int MEMORY_RANGE_MIN = 1;
int MEMORY_RANGE_MAX = 32;

/* Global variables. */
int running = 1;

char *main_server_ip;
char *backup_server_ip;
int main_server_port;
int backup_server_port;

/* 1 if connects to main 
2 if connects to backup */ 
int server_connected = 0;

int socket_fd;                      //socket file descriptor
struct sockaddr_in server_address;
struct hostent *server;

/* User commands. */
int SHORTEST_COMMAND = 5; //fe. get 1

char COMMAND_DISCONNECT[3] = "dis";
char COMMAND_GET_LOWER[3] = "get";
char COMMAND_GET_UPPER[3] = "GET";
char COMMAND_SET_LOWER[3] = "set";
char COMMAND_SET_UPPER[3] = "SET";
char COMMAND_HELP_LOWER[4] = "help";
char COMMAND_HELP_UPPER[4] = "HELP";
char COMMAND_EXIT_LOWER[4] = "exit";
char COMMAND_EXIT_UPPER[4] = "EXIT";

void openCommandLine();

/* Prints error message and if required, exits the program. */
void reportError(char *errorMsg, int exitOnEnd) {
    fprintf(stderr, "%s", errorMsg);

    if (exitOnEnd == TRUE) {
        fprintf(stderr, "Shutting down...\n");
        exit(EXIT_FAILURE);
    } 
}

/* Checks input parameters. */
void checkParameters(int argc, char *argv[]) {

    if (argc != PARAMETERS_REQUIRED) {
        reportError("Invalid amount of parameters. Expected:\nclient <server-port> <server-ip-address> <backup-server-port> <backup-server-ip-address>\n", TRUE);
    }

    main_server_port = atoi(argv[1]);
    main_server_ip = argv[2];
    backup_server_port = atoi(argv[3]);
    backup_server_ip = argv[4];

    /* Converts input ports to integer and checks for errors. */
    if (main_server_port == 0) {
        reportError("Invalid main server port.\n", TRUE);
    }

    if (backup_server_port == 0) {
        reportError("Invalid backup server port.\n", TRUE);
    }

    printf("Main server address: %s:%d \n", main_server_ip, main_server_port);
    printf("Backup server address: %s:%d \n", backup_server_ip, backup_server_port);
}

/* Connects the client to given server. */
int connectToServer(char* server_ip, int server_port) {
    
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    /* If socket system call fails, it returns -1. */
    if (socket_fd < 0) {
        reportError("There was error opening a socket.\n", FALSE);  
        return 1;     
    }

    printf("Socket created.\n");
    
    /* Finds server by name. */
    server = gethostbyname(server_ip);

    /* If server of such name was not found. */
    if (server == NULL) {
        reportError("Server was not found.\n", FALSE);
        return 1; 
    }

    printf("Server found.\n");
    /* Sets the fields in serv_addr. */
    bzero((char *) &server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&server_address.sin_addr.s_addr, server->h_length);
    server_address.sin_port = htons(server_port);

    /* Establishes connection with server. 

        arg1 - socket file descriptor
        arg2 - address of host we are connecting to
        arg3 - size of the address

        @returns - 0 on success, -1 on fail
    */

    printf("Connecting to server.\n", server_port);

    if (connect(socket_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        reportError("Failed to connect.\n", FALSE);
        return 1; 
    }

    printf("Connection established.\n");
    return 0;
}

/* Prints help dialog. */
void printHelp() {
    printf("Use following commands: \n");
    printf("get 'id' - recieve data from slot 'id' from the server \n");
    printf("set 'id' 'data' - sends new 'data' to server which will be stored in slot 'id' \n");
    printf("\n");
    printf("If you need more help, either panic or consult with local magician. \n");
}

/* Prints hint dialog. */
void printHint() {
    printf("Try command 'help'. \n");
}

/* Tries to connect to backup server. If already on
backup server, shuts down. */
void connectNext() {

    if (server_connected == 2) {
        reportError("Second server disconnected.", TRUE);
    } else {
        reportError("First server disconnected.\nConnecting to backup server.\n", FALSE);
        int response = connectToServer(backup_server_ip, backup_server_port);
        if (response == 0) {
            server_connected = 2;
            printf("Try your command again.\n");
            openCommandLine();  
        } else {
            reportError("Could not connect to second server.\n", TRUE);
        }
    }    
}

// sends message to the connected server
void sendMessage(char* command) {
    int n = 0, checkConnection = 0, response = 0;
    char buffer[256];
    bzero(buffer, 256);

    checkConnection = send(socket_fd, buffer, n, MSG_NOSIGNAL);

    if (checkConnection == -1) {
        connectNext();

    } else {

        printf("Sending: ");
        printf("%s \n", command);
        n = write(socket_fd, command, strlen(command));

        if (n < 0) {
            reportError("Unable to reach server with your message", FALSE);
        }
    }
}

/* Terminates connection. */
void closeConnection() {
    printf("Closing connection.\n");
    sendMessage("dis");
    shutdown(socket_fd, 2);
}

/* Prints exit dialog. */
void exitProgram() {
    closeConnection();
    printf("Shutting down.\n");
    exit(0);
}

/* Checks if id is within the range 1-32. */
bool isInMemoryRange(char *command) {
    char id[2];
    id[0] = command[4];
    id[1] = command[5];
    int memid = atoi(id);

    if (memid >= MEMORY_RANGE_MIN && memid <= MEMORY_RANGE_MAX) {
        return true;
    }
    reportError("Error: Incorrect ID, only values 1-32 are accepted.\n", FALSE);
    return false;
}

/*Checks if the message sent to server has correct format. */
bool isSetCorrectFormat(char *command) {

    if (command[3] == ' ') {
        /* First parameter is two digits long or first parameter is one digit */
        if (isdigit(command[4]) && isdigit(command[5]) && command[6] == ' ') {
            if (command[7] != ' ') {
                if (isInMemoryRange(command)) {
                    return true;
                }
            }            
        } else if(isdigit(command[4]) && command[5] == ' ') {
            if (isInMemoryRange(command)) {
                return true;
            }
        }
    }
    return false;
}

/*Checks if the message sent to server has correct format. */
bool isGetCorrectFormat(char *command) {

    if (command[3] == ' ') {
        /* First parameter is two digits long or first parameter is one digit */
        if (isdigit(command[4]) && isdigit(command[5]) && command[6] == EOL) {
            if (isInMemoryRange(command)) {
                return true;
            }       
        } else if (isdigit(command[4]) && command[5] == 10) {
            if (isInMemoryRange(command)) {
                return true;
            }   
        }
    }
    return false;
}

/* Checks if user used specific keyword like help or exit. */
int checkKeywords(char* command) {
    int m = 0;

    /* Looks for 'help' command. */
    if(strncmp(command, COMMAND_HELP_LOWER, 4) == IDENTICAL || strncmp(command, COMMAND_HELP_UPPER, 4) == IDENTICAL) {
        printHelp();
        return 1;
    }

    /* Looks for 'exit' command. */
    if(strncmp(command, COMMAND_EXIT_LOWER, 4) == IDENTICAL || strncmp(command, COMMAND_EXIT_UPPER, 4) == IDENTICAL) {
        exitProgram();
        return 1;
    }

    /* Looks for 'set' command. */
    if(strncmp(command, COMMAND_SET_UPPER, 3) == IDENTICAL || strncmp(command, COMMAND_SET_LOWER, 3) == IDENTICAL) {
        /* Checks if the format is correct. */
        if(isSetCorrectFormat(command)) {   
            /* Cuts the string so we are sending only what we have to. */
            /* if id == 1-9 */
            int commandLength = 0;
            int i;
            for (i = 0; i < MAX_SIZE; i++) {
                if (command[i] == EOL) {
                    break;
                }
                commandLength = commandLength + 1;
            }
            
        
            char *setMessage = (char *)malloc(sizeof(char) * (commandLength));
            strncpy(setMessage, command, commandLength);
            sendMessage(setMessage);
            bzero(setMessage, commandLength);
            free(setMessage);
            

            char buffer[256];
            bzero(buffer, 256);
            m = read(socket_fd, buffer, 255);

            if (m == 0) {
                connectNext();
                return 1;
            }
            if (m < 0) {
                error("ERROR reading from socket");
            }

            return 1;
        }
    }

    /* Looks for 'get' command. */
    if(strncmp(command, COMMAND_GET_UPPER, 3) == IDENTICAL || strncmp(command, COMMAND_GET_LOWER, 3) == IDENTICAL) {  

        if(isGetCorrectFormat(command)) {  
            int commandLength = 6;
            if (command[5] == EOL) {
                commandLength = 5;
            }
            /* Cuts the string so we are sending only what we have to. */
            char *getMessage = (char *) malloc(sizeof(char) * (commandLength));
            strncpy(getMessage, command, commandLength);
            sendMessage(getMessage);
            free(getMessage);

            char buffer[256];
            bzero(buffer, 256);
            m = read(socket_fd, buffer, 255);

            if (m == 0) {
                connectNext();
                return 1;
            }

            if (m < 0) {
                error("ERROR reading from socket");
            }

            printf("Data I got: %s \n",buffer);

            return 1;
        }
        return 1;
    }

    /* If no standard command was found. */
    printHint();
}

/* Checks validity of user's command and performs action accordingly. */
void processCommand(char *command) {

    /* Checks if any known command was entered. If not, continues function. */
    if (checkKeywords(command) == 1) {
        return;
    }

    if (strlen(command) < SHORTEST_COMMAND) {
        reportError("Command is too short.\n", FALSE);
        return;
    }
}

/* After connecting to server, allow user to send commands to server. */
void openCommandLine() {

    while (running == TRUE) {
        
        char command[MAX_SIZE];

        printf("Insert command: ");
        fgets(command, MAX_SIZE, stdin);
        processCommand(command);
    }
}


/* Not-so-dumb client. */
int main(int argc, char *argv[]) {
    int response = 0;
    checkParameters(argc, argv);
    // trying 1st server
    response = connectToServer(main_server_ip, main_server_port);

    if (response == 1) {
        //trying 2nd server
        printf("Couldn't connect to first server!\n");
        response = connectToServer(backup_server_ip, backup_server_port);

        if (response == 0) {
             // connected to 2nd server
            server_connected = 2;
            openCommandLine(); 
        } else {
            // failed to connect to both servers
            printf("Could not connect to second server!\n");
            exitProgram();
        }
        
    } else {
        //connected to 1st server
        server_connected = 1;
        openCommandLine(); 
    }

    printf("THE END!\n");
    return 0;
}