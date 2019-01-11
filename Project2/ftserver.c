/********************************************************************
*Program: 2
*File Name: ftserver.py
*Course: CS372 INTRO to Computer Networks
*Programmer: Mariam Ben-Neticha
*Date: 3.12.2017
*Description: Implements the client side of a 
*			  2-connection client-server network application.
*			  The client will create a TCP socket for the server on
*			  a remote port.
*			  Once a TCP control connection, P, has been established,
*			  client sends a command and server initiates a TCP data
*			  connection, Q, and sends directory listing or completes file transfer.
*			  Connection Q is closed immediately after action completed by server.
*			  Server waits for new connections on Connection P.
*			  Connection P closed when client terminates it on SIGINT.
*Resources: - Beej's Guide to Network Programming: Using Internet
*			Sockets Documentation: 
*			https://beej.us/guide/bgnet/
*			- Lecture slides on FTP connections and sockets
*			- Sockets Tutorial by Tutorials Point:
*			http://www.tutorialspoint.com/unix_sockets/socket_server_example.htm
*		    - Stackoverflow.com for implementation references
*		    and error/debugging solutions
*
********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h> 

#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LISTEN_QUEUE 5
#define LENGTH 1024 
//#define NUM_ALLOWED_CHARS 27
#define BUFFER_LENGTH 50 //Maximum string buffer length
#define FILE_LIST_LENGTH 1024 //Maximum length for directory listing

//Global variable 
int number_children = 0;

//function declaration
void handle_request(int command_socket, char *client_host_name);
void send_file_to_client(int socket, int file_pointer);
void receive_client_command(int socket, char *client_command, char *transfer_file_name, int *dataPort, char *client_host_name);
void send_client_file_status(int socket, char *server_response);
int get_num_commas(char *strValue, int strLen);
int get_comma_index(char *strValue, int strLen, int occurance);
void output_server_reqMsg(char *client_command, char *transfer_file_name, int dataPort);
void send_file_list_to_server(int sockfd);
void send_file_to_server(int sockfd, char *transfer_file_name);
void get_file_list(char *returnArr);
int hostname_to_ip(char *hostname, char *ip);
int startup (int argc, char *argv[]);


//function for parent to wait for child process to complete
static void wait_for_child(int sig)
{
	while (waitpid(-1, NULL, WNOHANG) > 0);
	number_children--;
}

//main method; runs the program
int main (int argc, char *argv[])
{
	return startup(argc, argv);
}

//runs the server, establishes connections and waits for requests, etc
int startup (int argc, char *argv[])
{
	//Ensure proper number of arguments entered
	if (argc < 2)
	{
		printf("ERROR: Enter ftserver <port>\n");
		exit(1);
	}
	
	//setup socket connection P
	int sockfd, newsockfd, sin_size, pid;
	struct sockaddr_in addr_local; // client addr
	struct sockaddr_in addr_remote; // server addr
	int portNumber = atoi(argv[1]);
	struct sigaction sa;
	char cHostName[BUFFER_LENGTH];
	bzero(cHostName, BUFFER_LENGTH);	

	//Get socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to obtain sockfd. (errno = %d)\n", errno);
		exit(1);
	}

	//fill the client-socket address struct
	addr_local.sin_family = AF_INET; //Protocol Family
	addr_local.sin_port = htons(portNumber); //Port number
	addr_local.sin_addr.s_addr = INADDR_ANY; //AutoFill local address
	bzero(&(addr_local.sin_zero), 8); //Flush the rest of struct

	//Bind a port to the socket
	if ( bind(sockfd, (struct sockaddr*)&addr_local, sizeof(struct sockaddr)) == -1 )
	{
		fprintf(stderr, "ERROR: Failed to bind to port. Please choose another port. (errno = %d)\n", errno);
		exit(1);
	}

	//Listen to port
	if (listen(sockfd, LISTEN_QUEUE) == -1)
	{
		fprintf(stderr, "ERROR: Failed to listen to port. (errno = %d)\n", errno);
		exit(1);
	}
	else
	{
		printf ("Server open on %d\n", portNumber);
	}

	//Set up sig-handler to clean-up zombie childrn
	sa.sa_handler = wait_for_child;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}

	//loop to accept/process client connections/requests
	while (1)
	{
		sin_size = sizeof(struct sockaddr_in);

		// wait for connections from the client
		if ((newsockfd = accept(sockfd, (struct sockaddr *)&addr_remote, &sin_size)) == -1)
		{
			fprintf(stderr, "ERROR: Obtaining new socket descriptor. (errno = %d)\n", errno);
			exit(1);
		}
		else
		{
			//Code snippet from http://beej.us/guide/bgnet/output/html/multipage/gethostbynameman.html
			struct hostent *he;
			struct in_addr ipv4addr;
			inet_pton(AF_INET, inet_ntoa(addr_remote.sin_addr), &ipv4addr);
			he = gethostbyaddr(&ipv4addr, sizeof ipv4addr, AF_INET);
			strncpy(cHostName, he->h_name, BUFFER_LENGTH);
			printf("Connection from %s\n", cHostName);
		}

		//create child process for multiple connections handling
		number_children++; //increment number of active children
		pid = fork();
		if (pid < 0)
		{
			perror("ERROR on fork");
			exit(1);
		}

		if (pid == 0)
		{
			//handle client requests
			close(sockfd);
			handleRequest(newsockfd, cHostName);
			exit(0);
		}
		else
		{
			close(newsockfd);
		}
	}
}


//function to process client requests
void handle_request(int command_socket, char *client_host_name)
{
	int dataPort = -1;
	char client_command[BUFFER_LENGTH];
	bzero(client_command, BUFFER_LENGTH);
	char transfer_file_name[BUFFER_LENGTH];
	bzero(transfer_file_name, BUFFER_LENGTH);
	char client_host_name[BUFFER_LENGTH]; //the ftserver hostname
	bzero(client_host_name, BUFFER_LENGTH);
	receive_client_command(command_socket, client_command, transfer_file_name, &dataPort, client_host_name);
	
	output_server_reqMsg(client_command, transfer_file_name, dataPort);
	close(command_socket);

	// Get IP address from ftclient host name
	char IP[100];
	hostname_to_ip(client_host_name, IP);
	
	int data_socket;
	struct sockaddr_in remote_addr;
	
	//get the Socket file descriptor 
	if ((data_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Error: Failed to obtain sockfd.\n");
		exit(2);
	}
	
	//fill the socket address struct   
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(dataPort);
	inet_pton(AF_INET, IP, &remote_addr.sin_addr);
	bzero(&(remote_addr.sin_zero), 8);

	// Try to connect the client data port 
	if (connect(data_socket, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) == -1)
	{
		printf("Error: could not contact client on port %d\n", dataPort);
		exit(2);
	}

	if (strcmp(client_command, "-l") == 0)
	{
		printf("Sending directory contents to %s:%d.\n", client_host_name, dataPort);
		send_file_list_to_server(data_socket);
	}
	else if (strcmp(client_command, "-g") == 0)
	{
		//Reference: http://stackoverflow.com/questions/230062/whats-the-best-way-to-check-if-a-file-exists-in-c-cross-platform
		//if file exists
		if(access(transfer_file_name, F_OK) != -1) 
		{
			send_client_file_status(data_socket, "s");
			printf("Sending \"%s\" to %s:%d\n", transfer_file_name, client_host_name, dataPort);
			send_file_to_server(data_socket, transfer_file_name);
		}
		//else file does not exist
		else {
			printf("File not found. Sending error message to %s:%d\n", client_host_name, dataPort);
			send_client_file_status(data_socket, "e");
		}
	}
	close(data_socket);
	exit(0);
}

//function that gets the ip_address from the host name
int hostname_to_ip(char *hostname , char *ip)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;
    
    if ((he = gethostbyname(hostname)) == NULL) 
    {
        herror("gethostbyname");
        return 1;
    }
 
    addr_list = (struct in_addr **) he->h_addr_list;
    
    //iterate through the network info struct to get the IP. 
    for(i = 0; addr_list[i] != NULL; i++) 
    {
        //Return the first one;
        strcpy(ip ,inet_ntoa(*addr_list[i]));
        return 0;
    }
     
    return 1;
}

//function to send directory listing to client
void send_file_list_to_server(int sockfd)
{
	char fileList[FILE_LIST_LENGTH];
	bzero(fileList, FILE_LIST_LENGTH);
	get_file_list(fileList);
	
	int sendSize = FILE_LIST_LENGTH;
	if (send(sockfd, fileList, sendSize, 0) < 0)
	{
		printf("Error: Failed to send directory list.\n");
	}
}

//gets all filenames listed in directory
void get_file_list(char *dirList)
{
	//Get host name of server to add to the beginning of the list 
    char name[50];
    gethostname(name, 50);
    strcat(dirList, name);
    strcat(dirList, ",");
    
    DIR *directoryHandle;
    struct dirent *directoryStruct;
    
    // Loop through all the files and add them to the list
    directoryHandle = opendir(".");
    if (directoryHandle)
    {
    	int currDirLen = 0;
    	int currDirListLen = 0;
        while ((directoryStruct = readdir(directoryHandle)) != NULL)
        {
            if (strcmp(directoryStruct->d_name, ".") != 0 && strcmp(directoryStruct->d_name, "..") != 0) 
            {
            	// Make sure we dont overflow our send buffer
            	currDirLen = strnlen(directoryStruct->d_name, NAME_MAX);
	    		currDirListLen = strnlen(dirList, FILE_LIST_LENGTH);
	    		if ((currDirListLen + currDirLen) > FILE_LIST_LENGTH)
	    		{
	    			break;
	    		}
	    		
	    		// Add the directory to the comma delimited list	
    			strcat(dirList, directoryStruct->d_name);
    			strcat(dirList, ",");
            }
        }
        
        closedir(directoryHandle);
    }
	
	// Remove the last comma 
	int lastCommaIdx = strnlen(dirList, FILE_LIST_LENGTH) - 1;
	dirList[lastCommaIdx] = 0;
}

//function that transfers the requested file to the client
void send_file_to_server(int sockfd, char *transfer_file_name)
{
	FILE * transfer_file_pointer;
	transfer_file_pointer = fopen(transfer_file_name, "r+");
	
	int fd = fileno(transfer_file_pointer);
	send_file_to_client(sockfd, fd);
	
	fclose(transfer_file_pointer);
}

//displays server request message to console
void output_server_reqMsg(char *client_command, char *transfer_file_name, int dataPort)
{
	if (strcmp(client_command, "-l") == 0)
	{
		printf("List directory requested on port %d.\n", dataPort);
	}
	
	if (strcmp(client_command, "-g") == 0)
	{
		printf("File \"%s\" requested on port %d.\n", transfer_file_name, dataPort);
	}
}

//function that tells client whether file is available
void send_client_file_status(int socket, char *server_response)
{
	char sendBuffer[2]; // Send buffer
	bzero(sendBuffer, 2);
	strncpy(sendBuffer, server_response, 1);

	if (send(socket, sendBuffer, 1, 0) < 0)
	{
		printf("ERROR: Failed to send client the following message: %s", server_response);
		exit(1);
	}
}

//gets command from client
void receive_client_command(int socket, char *client_command, char *transfer_file_name, int *dataPort, char *client_host_name)
{
	// Wait for client to send message
	char receiveBuffer[BUFFER_LENGTH];
	bzero(receiveBuffer, BUFFER_LENGTH);
	recv(socket, receiveBuffer, LENGTH, 0);

	// Get the comma instance in the command string
	int numCommas = get_num_commas(receiveBuffer, BUFFER_LENGTH);
	int commaIdx = get_comma_index(receiveBuffer, BUFFER_LENGTH, 1);

	// Get the client command from the sent string	
	strncpy(client_command, receiveBuffer, commaIdx);
	
	// Get the data port from the sent string	
	char dataPortBuffer[BUFFER_LENGTH];
	bzero(dataPortBuffer, BUFFER_LENGTH);
	strncpy(dataPortBuffer, receiveBuffer + commaIdx + 1, BUFFER_LENGTH);
	(*dataPort) = atoi(dataPortBuffer);

	if (numCommas == 2)
	{
		int commaIdx = get_comma_index(receiveBuffer, BUFFER_LENGTH, 2);
		strncpy(client_host_name, receiveBuffer + commaIdx + 1, BUFFER_LENGTH);
	}

	// Get file name if a third param is passed through	
	if (numCommas == 3)
	{
		// Example = "-l, 3333, filename, hostname". 
		// This code block finds the second and third comma and gets the value
		// in between
		int commaIdxStart = get_comma_index(receiveBuffer, BUFFER_LENGTH, 2);
		int commaIdxEnd = get_comma_index(receiveBuffer, BUFFER_LENGTH, 3);
		strncpy(transfer_file_name, receiveBuffer + commaIdxStart + 1, commaIdxEnd - commaIdxStart - 1);
		
		strncpy(client_host_name, receiveBuffer + commaIdxEnd + 1, BUFFER_LENGTH);
	}
	
	// Clean the newline character from the client host name
	int i = 0;
	char currChar = ' ';
	while(currChar != 0)
	{
		if (i == BUFFER_LENGTH)
		{
			break;
		}
		
		currChar = client_host_name[i];	
		if (currChar == '\n')
		{
			client_host_name[i] = 0;
			break;
		}
		
		i++;
	}
}

//function that gets the index of the occurance #th comma 
int get_comma_index(char *strValue, int strLen, int occurance)
{
	int i = 0;
	int commaIdx = -1;
	int numCommas = 0;
	char currChar = ' ';
	
	while(currChar != '\n')
	{
		currChar = strValue[i++];
		if (currChar == ',')
		{
			numCommas++;
		}
		
		if (numCommas == occurance)
		{
			commaIdx = i - 1;
			break;
		}
		
		// Exit loop if accessing an out of bounds index
		if (i >= strLen)
		{
			break;
		}
	}
	
	return commaIdx;
}

//gets the total number of commas in the string
int get_num_commas(char *strValue, int strLen)
{
	int i = 0;
	int numCommas = 0;
	char currChar = ' ';
	
	while(currChar != '\n')
	{
		currChar = strValue[i++];
		if (currChar == ',')
		{
			numCommas++;
		}
		
		if (i >= strLen)
		{
			break;
		}
	}
	return numCommas;
}

//function to send requested file to the client
void send_file_to_client(int socket, int file_pointer)
{
	char sendBuffer[LENGTH]; // Send buffer
	if (file_pointer == 0)
	{
		fprintf(stderr, "ERROR: File not found on server.");
		exit(1);
	}
	
	bzero(sendBuffer, LENGTH);
	int readSize;
	while ((readSize = read(file_pointer, sendBuffer, LENGTH)) > 0)
	{
		if (send(socket, sendBuffer, readSize, 0) < 0)
		{
			fprintf(stderr, "ERROR: Failed to send file.");
			exit(1);
		}
		bzero(sendBuffer, LENGTH);
	}
}