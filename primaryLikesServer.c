#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>

int resetLog() {
	FILE *file;
	//Path of directory to be created
	char dirPath[] = "/tmp/PrimaryLikesServer";
	//Path of log file to be created
	char filePath[] = "/tmp/PrimaryLikesServer/log.txt";

	//Create directory with chmod 777
	struct stat st = {0};
	if(stat(dirPath, &st) == -1) {
		if(mkdir(dirPath, 0777) != 0) {
			printf("Error Creating Directory\n");
			return 1;
		}
	}
	//Open/Create the file to write the information to it
	file = fopen(filePath, "w");
	if(file == NULL) {
		printf("Error Creating File\n");
		return 1;
	}
	fclose(file);
	return 0;
}

/*
* logData is a helper function that logs the information received
* from the socket to the primaryLikesServer log file
*/
int logData(char *likesServerInfo, int currTotalLikes) {
	//Similar to parentProcess.c where directory and log file are
	//created if they do not exist
	FILE *file;
	//Path of log file to be created
	char filePath[] = "/tmp/PrimaryLikesServer/log.txt";

	//Open/Create the file to write the information to it
	file = fopen(filePath, "a");
	if(file == NULL) {
		printf("Error Creating File\n");
		return 1;
	}

	//This segment of code gets a timestamp for the log file
	time_t currTime;
	struct tm* currTimeInfo;
	char currLogTime[256];
	time(&currTime);
	currTimeInfo = localtime(&currTime);
	strftime(currLogTime, 256, "%Y-%m-%d %H:%M:%S", currTimeInfo);

	//Print the likesServer and the amount of likes received
	fprintf(file, "[%s] %s\n", currLogTime, likesServerInfo);
	//Print the total amount of likes at that given time
	fprintf(file, "[%s] Total     %d\n", currLogTime, currTotalLikes);

	fclose(file);
	return 0;
}

int main() {
	int checkReset = resetLog();
	if(checkReset != 0) {
		printf("Error Reseting Log\n");
		return 1;
	}

	//Will keep track of the total like count over the period of time
	int totalLikes = 0;

	//Create the socket to connect to the LikesServer
	int primaryLikes = socket(AF_INET, SOCK_STREAM, 0);
	if(primaryLikes < 0) {
		printf("Error Creating Socket!\n");
		return 1;
	}

	//reference for socket calls: opensource.com/article/19/4/interprocess-communication-linux-networking
	//Setup for the socket to be able to communicate with the likesServer via IPC
	int PortNumber = 8080;
	struct sockaddr_in saddr;
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	saddr.sin_port = htons(PortNumber);

	//Bind the socket to the local machine (parent process only)
	if(bind(primaryLikes, (struct sockaddr *) &saddr, sizeof(saddr)) < 0) {
		printf("Error Binding!\n");
		return 1;
	}
	//Listen on the socket for a max of 10 connections
	//Only 10 because of the amount of LikesServers sending data
	if(listen(primaryLikes, 10) < 0) {
		printf("Error Listening!\n");
		return 1;
	}

	//Used for program time, set to 305 for edge case where likesServer waits
	//for 5 seconds at 299 seconds
	int programTime = 305;
	time_t startTime = time(NULL);

	//sets the socket to non-blocking mode so the program will eventually terminate
	//reference: ibm.com/docs/en/zos/2.2.0?topic=otap-clientserver-socket-programs-blocking-nonblocking-asynchronous-socket-calls
	if(fcntl(primaryLikes, F_SETFL, O_NONBLOCK) < 0) {
		printf("Error setting accept to non-blocking!\n");
		return 1;
	}

	while(1) {
		//Check if time is at limit
		time_t currentTime = time(NULL);
		int elapsedTime = currentTime - startTime;
		if(elapsedTime >= programTime) {
			break;
		}

		struct sockaddr_in caddr;
		socklen_t len = sizeof(caddr);

		//Setup the client socket to recieve the data
		int client_socket = accept(primaryLikes, (struct sockaddr*) &caddr, &len);
		if(client_socket < 0) {
			continue;
		}
		//Buffer to hold the value sent by the LikesServer
		char buffer[256];
		//Read the data from the socket
		int rec = read(client_socket, buffer, sizeof(buffer));
		//String used to make sure data string starts with "LikesServer"
		char checkString[] = "LikesServer";
		//Error code 1 if the data sent is not in valid format
		char invalidMessage[] = "1";
		//Used for current iteration of tracking
		int currLike = 0;
		if(rec > 0) {
			//End buffer with null terminator
			buffer[rec] = '\0';
			//Get the first token of the string (should be "LikesServer%d")
			char *token = strtok(buffer, " ");
			//Check if the string is valid
			int stringCheck = strncmp(token, checkString, 11);
			if(stringCheck != 0) {
				write(client_socket, invalidMessage, sizeof(invalidMessage));
			} else {
				//Gets the next token (should be an integer between 0-42)
				while(token != NULL) {
					token = strtok(NULL, " ");
					if(token != NULL) {
						currLike = atoi(token);
						break;
					}
				}
				//Checks to make sure it is an integer between 0 and 42
				if(currLike >= 0 && currLike <= 42) {
					totalLikes = totalLikes + currLike;

					char successResponse[] = "0";
					write(client_socket, successResponse, sizeof(successResponse));
					//log the data with the correct information
					int logSuccess = logData(buffer, totalLikes);
					if(logSuccess == 1) {
						printf("Error Logging Info\n");
						return 1;
					}
				} else {
					//If it is not a valid integer
					write(client_socket, invalidMessage, sizeof(invalidMessage));
				}
			}
		} else {
			//If there was an error receiving information
			char failureResponse[] = "1";
			write(client_socket, failureResponse, sizeof(failureResponse));
		}
		//Cleanup work, close the client socket each iteration
		close(client_socket);

	}
	//Finally, close the primaryLikesServer
	close(primaryLikes);

	return 0;
}
