#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#define TRUE 1
#define FALSE 0

#define IDENTICAL 0

/* Global constants. */
int PARAMETERS_REQUIRED = 4;
int MAX_SIZE = 256;
int INT_MAX = 15;
char folder[80] = "serverfiles/";
char EOL = 10;

/* Global variables. */
int running = 1;
int connectedToBackup = 0;
int backupConnectedToMe = 0;

int this_server_port;   
char *other_server_ip;
int other_server_port;
char buffer[256];    

/* File descriptors for used sockets. */
int sockfd, newsockfd;

/* Structures for server-client. */
struct sockaddr_in serv_addr, cli_addr;
socklen_t clilen;   
int sockfd, newsockfd;  /* File descriptors for used sockets. */

/* Structures for server-server. */
struct sockaddr_in other_server_address;
int other_server_socket_fd;
struct hostent *server;


/* Server commands. */
char COMMAND_DISCONNECT[3] = "dis";
char COMMAND_GET[3] = "get";
char COMMAND_SET[3] = "set";
char COMMAND_UPD[3] = "upd";
char COMMAND_SYN[3] = "syn";
char COMMAND_OUK[3] = "ouk";


/* Getting ID from data*/
int readID(char* data) {
    return atoi(&data[4]);
}

/* Closes socket the server is listening to. */
void closeConnection(int sock_fd) {
    close(sock_fd);
    printf("Client has been disconnected.\n");
}

/* Returns time of the system. */
int getTimestamp() {
    time_t result = time(NULL);
    return (int)result;
}

/* Throws error message and exits the program. */
void error(const char *msg) {

    perror(msg);
    closeConnection(sockfd);
    exit(1);
}

/* Connects to other server. */
int connectToServer(char* server_ip, int server_port) {
    
    other_server_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    /* If socket system call fails, it returns -1. */
    if (other_server_socket_fd < 0) {
        printf("There was error opening a socket.\n"); 
        return 0;       
    }
    
    /* Finds server by name. */
    server = gethostbyname(server_ip);

    /* If server of such name was not found. */
    if (server == NULL) {
        printf("Server was not found.\n");
        return 0;
    }

    /* Sets the fields in serv_addr. */
    bzero((char *) &other_server_address, sizeof(other_server_address));
    other_server_address.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&other_server_address.sin_addr.s_addr, server->h_length);
    other_server_address.sin_port = htons(server_port);

    printf("Server found.\n");

    /* Establishes connection with server. 

        arg1 - socket file descriptor
        arg2 - address of host we are connecting to
        arg3 - size of the address

        @returns - 0 on success, -1 on fail
    */

    if (connect(other_server_socket_fd, (struct sockaddr *) &other_server_address, sizeof(other_server_address)) < 0) {
        printf("Failed to connect.\n");
        return 0;
    }

    return 1;
}

/* Checks input parameters. */
void checkParameters(int argc, char *argv[]) {

    if (argc != PARAMETERS_REQUIRED) {
        error("Invalid amount of parameters. Expected:\nserver <listening-port> <other-server-port> <other-server-ip-address>\n");
    }

    this_server_port = atoi(argv[1]);
    other_server_port = atoi(argv[2]);
    other_server_ip = argv[3];

    /* Converts input ports to integer and checks for errors. */    
    if (this_server_port == 0) {
        error("Invalid port of this server.\n");
    }
    if (other_server_port == 0) {
        error("Invalid port of other server.\n");
    }

    if (other_server_ip == 0) {
        error("Invalid port of other server.\n");
    }

    printf("This server will listen to following port: %d \n", this_server_port);
    printf("Address of the other server: %s:%d \n", other_server_ip, other_server_port);
}

char* getFileContent(int id) {
    char *fileContent = malloc(sizeof(char) * (MAX_SIZE + INT_MAX));
    bzero(fileContent, MAX_SIZE + INT_MAX);

    /* Checks if folder exists. */
    if (stat(folder, 0) == -1) {
        mkdir(folder, 0700);
    }

    char filepath[80];
    char tmp[2];
    FILE *fp;
    
    // Read DATA from disk
    sprintf(tmp, "%d", id);
    strcpy(filepath, folder);
    strcat(filepath, tmp);
    strcat(filepath, ".oss");
    fp = fopen(filepath, "rt");

    // File does not exist
    if (fp == NULL) {
        return NULL;
    }

    rewind(fp);

    char firstLine[MAX_SIZE];
    char secondLine[INT_MAX];

    fgets(firstLine, MAX_SIZE, fp);
    fgets(secondLine, MAX_SIZE, fp);

    strcat(fileContent, firstLine);
    strcat(fileContent, secondLine);
    
    fclose(fp);   
    return fileContent;
}

/* Setting data on the server side */
int updateData(char* data) {

    /* Checks if folder exists. */
    if (stat(folder, 0) == -1) {
        mkdir(folder, 0700);
    }

    int id;
    char filepath[80];
    char tmp[2];
    FILE *fp;
    
    // Read ID of DATA
    id = readID(data);

    // Read DATA from disk
    sprintf(tmp, "%d", id);
    strcpy(filepath, folder);
    strcat(filepath, tmp);
    strcat(filepath, ".oss");
    fp = fopen(filepath, "rt");

    rewind(fp);
    
    char local_firstLine[MAX_SIZE];
    char local_secondLine[INT_MAX];
    int local_timestamp;

    /* Reads local data. */
    //Read first line of file (including EOL char)
    fgets(local_firstLine, MAX_SIZE, fp);
    fgets(local_secondLine, MAX_SIZE, fp);
    //Cut the timestamp from string
    local_firstLine[strlen(local_firstLine) - 1] = 0;
    local_timestamp = atoi(local_secondLine);

    /* Gets timestamp from sent data. */
    char* ext_secondLine;
    int ext_timestamp;
    ext_secondLine = strchr(data, EOL);
    ext_secondLine = ext_secondLine + 1;
    ext_timestamp = atoi(ext_secondLine);

    /* If local data is older, it is replaced with recieved data. */
    if (ext_timestamp > local_timestamp) {
        printf("Rewriting file %d.\n", id);        
        setData(data);
    }

    fclose(fp);   
    return 0;
}

/* Setting data on the server side */
int setData(char* data) {

    /* Checks if folder exists. */
    if (stat(folder, 0) == -1) {
        mkdir(folder, 0700);
    }

    int id;
    char filepath[80];
    char tmp[2];
    FILE *fp;
    
    // Get ID and DATA separate
    id = readID(data);

    // Save DATA on disk
    sprintf(tmp, "%d", id);
    strcpy(filepath, folder);
    strcat(filepath, tmp);
    strcat(filepath, ".oss");

    fp = fopen(filepath, "w");

    /* Make sure we are saving only data and nothing in front of it. */
    int timestamp;
    timestamp = getTimestamp();
    if (id < 10) {

        fprintf(fp, "%s\n", &data[6]);
        fprintf(fp, "%d", timestamp);

        char integer[INT_MAX];
        sprintf(integer, "%d", timestamp);
        strcat(data, "\n");
        strcat(data, integer);
    } else {
        fprintf(fp, "%s\n", &data[7]);
        fprintf(fp, "%d", timestamp);

        char integer[INT_MAX];
        sprintf(integer, "%d", timestamp);
        strcat(data, "\n");
        strcat(data, integer);
    }

    fclose(fp);
}

/* Getting data from the server */
int getData(char* data, int clientsockfd) {
    int id;
    char filepath[80];
    char fileContent[MAX_SIZE + INT_MAX];
    char tmp[2];
    char word[MAX_SIZE];
    FILE *fp;

    // Read ID of DATA
    id = readID(data);

    // Read DATA from disk
    sprintf(tmp, "%d", id);
    strcpy(filepath, folder);
    strcat(filepath, tmp);
    strcat(filepath, ".oss");
    fp = fopen(filepath, "rt");

    // File does not exist
    if (fp == NULL) {
        sendData("Data don't exist.", clientsockfd);
        return 1;
    }

    rewind(fp);
    
    //Read first line of file (including EOL char)
    fgets(fileContent, MAX_SIZE, fp);
    //Cut the timestamp from string
    fileContent[strlen(fileContent) - 1] = 0;

    fclose(fp);   

    // Send it to client
    sendData(fileContent, clientsockfd);

    // Return 0 on success 1 on failure
}

/* Send data back to client */
int sendData(char* data, int clientsockfd) {
    int n = 0;
    n = write(clientsockfd, data, 255);

    if (n < 0) {
        printf("ERROR writing to socket");
    }
}

/* Sends all of the data to other server so it can 
update itself if necessary. */
void synchronize() {
    printf("Sending stored data to remote server.\n");
    int i;
    for (i = 1; i < 33; i++) {
        int size = MAX_SIZE + INT_MAX + 7;
        char buffer[size];
        char command[4] = "upd ";
        char number[2];

        sprintf(number, "%d", i);

        bzero(buffer, size);

        /* Returns NULL for non-existing files. */
        if (getFileContent(i) == NULL) {
            bzero(command, 4);
            bzero(number, 2);
            continue;
        }

        strcat(buffer, command);
        strcat(buffer, number);
        strcat(buffer, " ");
        strcat(buffer, getFileContent(i));

        sendData(buffer, other_server_socket_fd);

        bzero(command, 4);
        bzero(number, 2);
        bzero(buffer, size);
        sleep(1);
    }
    printf("Remote server is now up to date.\n");
}

int manageClient(int newsockfd) {
    char buffer[256];
    int n = 0;

    printf("Client connected.\n");
    
    while (running == 1) {

        bzero(buffer, 263);
        n = read(newsockfd, buffer, 262);

        if (n == 0) {
            printf("Connection brutally terminated. ");
            closeConnection(newsockfd);
            return 1;
        }

        if (n < 0) {
            connectedToBackup = 0;
            printf("Connection to backup server has been lost. \n");
        }

        /* Client is disconnecting. */
        if(strncmp(buffer, COMMAND_DISCONNECT, 3) == IDENTICAL){
            running = 0;      

        /* Client is trying to SET new data. */      
        } else if(strncmp(buffer, COMMAND_SET, 3) == IDENTICAL) {
            printf("Setting: %s\n", buffer);  
            sendData(COMMAND_OUK, newsockfd);
            setData(buffer);

            if (connectedToBackup == 1) {
                //send this new data to other server            
                buffer[0] = COMMAND_UPD[0];
                buffer[1] = COMMAND_UPD[1];
                buffer[2] = COMMAND_UPD[2];
                sendData(buffer, other_server_socket_fd); 
            }

        /* Client is trying to GET data. */
        } else if(strncmp(buffer, COMMAND_GET, 3) == IDENTICAL) {
            printf("Getting: %s\n", buffer);  
            getData(buffer, newsockfd);

        /* Other server is sending update. */
        } else if(strncmp(buffer, COMMAND_UPD, 3) == IDENTICAL) {
            printf("Updating: %s\n", buffer);  
            sendData(COMMAND_OUK, newsockfd);
            updateData(buffer);
        }
    }

    /* Closes sockets. */
    closeConnection(newsockfd);
    return 0;
}

struct newclient_fd {
    int arg1;
};

void *runClientOnThread(void *arguments) {
    struct newclient_fd *args = (struct newclient_fd *)arguments;
    int newclient_fd = args -> arg1;
    manageClient(newclient_fd);
    return NULL;
}

/* Opens socket on specific port. */
void openPort() {

    /* Creates specific type of socket.
        AF_INET for internet domain
        AF_UNIX for unix domain

        SOCK_STREAM for Stream socket
        SOCK_DGRAM for Datagram socket

        0 for protocol type (should be always 0)
     */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);      

    if (sockfd < 0) {
        error("ERROR opening socket");        
    }

    /* Sets all values in a buffer to 0.
        arg1 - pointer to buffer
        arg2 - size of buffer

        = Initializes serv_addr to 0
     */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    
    /* Serv_addr is struct with four fields. 
        sin_family - should always be AF_INET
        sin_addr.s_addr - IP address of code, for server it is usually INADDR_ANY
        sin_port - port server is listening to, must be converted by htons(int) command
    */
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(this_server_port);

    /* Binds a socket to an address.

        arg1 - socket file descriptor, address to which it is bound and size of address
        arg2 - pointer to sockaddr, but we pass structure of sockaddr_in
        arg3 - size 

        Can fail for many reasons, usualy because socket is already in use.
    */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        error("ERROR on binding");
    }

    /* Listens on the socket for connections.
        arg1 - socket file descriptor
        arg2 - size of backlog queue (number of connections that can be waiting while
                process is handling particular connection)

        If arg1 is a valid socket, this cannot fail.
    */
    listen(sockfd, 5);

    clilen = sizeof(cli_addr);
}

/* Connects to backup server. */
void *connectToOtherServer() {

    connectedToBackup = 0;
    int DELAY = 3;

    while (connectedToBackup == 0) {
        printf("Looking for backup server. \n");
        sleep(DELAY);
        /* Returns 1 when connection has been established. */
        connectedToBackup = connectToServer(other_server_ip, other_server_port);
    }
    printf("Server has connected to backup server. \n");
    connectedToBackup = 1;
    synchronize();
}

/* Starts listening on given port for incoming clients. Creates thread for
every new incoming client. */
void startListening() {

    printf("Waiting for clients.\n");
    
    /* Waiting for client to connect, then fork() and manage client. */
    while(TRUE) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

        pthread_t newclient_thread;
        struct newclient_fd args;
        args.arg1 = newsockfd;

        if (pthread_create(&newclient_thread, NULL, &runClientOnThread, (void *)&args) != 0) {
            printf("ERROR creating thread");
        }
    }  
}

int main(int argc, char *argv[]) {

    pthread_t connect_to_other_server_thread;

    checkParameters(argc, argv);
    openPort();

    /* Creates thread for connecting to other server. */
    if(pthread_create(&connect_to_other_server_thread, NULL, connectToOtherServer, NULL)) {
        printf("ERROR creating thread");
        return 1;
    }

    startListening();

    closeConnection(sockfd);
    return 0; 
}