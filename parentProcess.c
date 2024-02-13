/*
* File: parentProcess.c
* Author: Joey Hardester
* Class: CMSC 421
* Section: 
* Description: This is the file that spins off the LikesServers, which generates
*              random likesAmount to be sent to the primaryLikesServer.
*/
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <sys/wait.h>

int resetLogs() {
	FILE *file;
	//Location of the directory of the log file
	char dirPath[256];
	//Location of the actual log file
	char filePath[256];

	for(int i=0; i<10; i++) {
		//Create the string for the directory path
		snprintf(dirPath, sizeof(dirPath), "/tmp/LikesServer%d", i);
		//Create the string for the file path
		snprintf(filePath, sizeof(filePath), "/tmp/LikesServer%d/log.txt", i);

		//Use mkdir with chmod 777 on the path if the directory does not exist
		//Reference: stackoverflow.com/questions/7430248/creating-a-new-directory-in-c
		struct stat st = {0};
		if(stat(dirPath, &st) == -1) {
			if(mkdir(dirPath, 0777) != 0) {
				printf("Error Creating Directory\n");
				return 1;
			}
		}

		//Open/Create the file, reset it
		file = fopen(filePath, "w");
		if(file == NULL) {
			printf("Error Creating File\n");
			return 1;
		}
		fclose(file);
	}
	return 0;
}

/*
* getTimeStamp() is a helper function used to get the current time
* of the operation at hand, for logging purposes
*/
char* getTimeStamp() {
	//Reference: ibm.com/docs/en/workload-automation/10.2.0?topic=troubleshooting-date-time-format-reference-strftime
	time_t currTime;
	struct tm* currTimeInfo;
	static char logTime[256];
	time(&currTime);
	currTimeInfo = localtime(&currTime);
	strftime(logTime, 256, "%Y-%m-%d %H:%M:%S", currTimeInfo);
	return logTime;
}

/*
* logNumLikes is a helper function that writes to the log files of
* the LikesServers. It does this by using the index of the for loop
* and the randomly generated amount of likes.
*/
int logNumLikes(int index, int likes, int returnCode) {
	//the file to be written to
	FILE *file;
	//location of the file
	char filePath[256];

	snprintf(filePath, sizeof(filePath), "/tmp/LikesServer%d/log.txt", index);

	//Open/Create the file in that location
	file = fopen(filePath, "a");
	if(file == NULL) {
		printf("Error Creating File\n");
		return 1;
	}

	//Put the desired information within the log file
	char* logLikesTime = getTimeStamp();
	fprintf(file, "[%s] LikesServer%d sending %d recieved %d\n", logLikesTime, index, likes, returnCode);

	fclose(file);
	return 0;
}

int main() {
	int checkReset = resetLogs();
	if (checkReset != 0) {
		printf("Error Reseting Logs\n");
		return 1;
	}

	//Use for the amount of LikesServers to be sent to the
	//PrimaryLikesServer
	int likesServerAmount = 10;
	pid_t child_pid;
	pid_t likesServerPIDS[likesServerAmount];

	//Repeat process in logNumLikes for ParentProcessStatus log file
	FILE *parentFile;
	char *dirPath = "/tmp/ParentProcessStatus";
	char *parentFilePath = "/tmp/ParentProcessStatus/log.txt";

	//Make the path for the ParentProcessStatus log
	struct stat st = {0};
	if(stat(dirPath, &st) == -1) {
		if(mkdir(dirPath, 0777) != 0) {
			printf("Error creating directory\n");
			return 1;
		}
	}

	//Open/Create the ParentProcessStatus log
	parentFile = fopen(parentFilePath, "w");
	if(parentFile == NULL) {
		printf("Error creating file\n");
		return 1;
	}
	fclose(parentFile);

	//Variable used to count how many child processes fail
	//Used for logging purposes
	int failCount = 0;
	//For loop to create 10 child processes
	for(int i=0; i<likesServerAmount; i++) {
		//Fork call to start each LikesServer
		child_pid = fork();
		if(child_pid == 0) {
			//this segment of code writes to the log file of the parent process
			//saying that child process has began running
			parentFile = fopen(parentFilePath, "a");
			char* startLogTime = getTimeStamp();
			fprintf(parentFile, "[%s] LikesServer%d created\n", startLogTime, i);
			fclose(parentFile);

			//Program timer that will be used to have the program run
			//for 5 minutes
			int programTime = 300;
			time_t startTime = time(NULL);

			//Gets a random interval between 1-5 seconds to send the signal
			unsigned int intervalSeed = getpid() ^ (unsigned int)time(NULL);
			srand(intervalSeed);
			int intervalTime = (rand() % 5) + 1;

			while(1) {
				//Check if the program has reached 5 minutes
				time_t currentTime = time(NULL);
				int elapsedTime = currentTime - startTime;
				if(elapsedTime >= programTime) {
					//this segment of code writes to the log file of the parent process
					//saying that the child process has finished executing
					parentFile = fopen(parentFilePath, "a");
					char* endLogTime = getTimeStamp();
					fprintf(parentFile, "[%s] LikesServer%d finished\n", endLogTime, i);
					fclose(parentFile);
					exit(0);
					break;
				}

				//Generate a random number of likes between 0-42
				unsigned int seed = getpid() ^ (unsigned int)time(NULL);
				srand(seed);
				int numLikes = rand() % 43;

				//Sleeps for the interval time before sending likes to
				//PrimaryLikesServer
				sleep(intervalTime);

				//Information that will be used in the setup of the socket
				int primaryLikesServer;
				struct sockaddr_in server_addr;
				//Port number that the socket will be listening on
				int port = 8080;

				//reference for socket calls: geeksforgeeks.org/socket-programming-cc/
				primaryLikesServer = socket(AF_INET, SOCK_STREAM, 0);
				if(primaryLikesServer == -1) {
					perror("Error creating socket!");
					parentFile = fopen(parentFilePath, "a");
					char* failSocketTime = getTimeStamp();
					fprintf(parentFile, "[%s] LikesServer%d failed\n", failSocketTime, i);
					fclose(parentFile);
					failCount++;
					exit(1);
				}

				server_addr.sin_family = AF_INET;
				server_addr.sin_port = htons(port);
				//Specified to listen on the localhost for IPC
				server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

				//Connects to the PrimaryLikesServer
				if(connect(primaryLikesServer, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
					printf("Error connecting!\n");
					char* failConnectTime = getTimeStamp();
					parentFile = fopen(parentFilePath, "a");
					fprintf(parentFile, "[%s] LikesServer%d failed\n", failConnectTime, i);
					fclose(parentFile);
					failCount++;
					exit(1);
				}

				//Setup of data to be transferred to primaryLikesServer
				char data[256];
				//Have to use memset here to avoid issue of "syscall param write(buf)
				//points to uninitialised byte"
				//Reference: itecnote.com/tecnote/valgrind-error-when-writing-struct-to-file
				//-with-fwrite-syscall-param-writebuf-points-to-unintialised-bytes/
				memset(data, 0, sizeof(data));
				snprintf(data, sizeof(data), "LikesServer%d %d", i, numLikes);
				ssize_t checkWrite = write(primaryLikesServer, data, sizeof(data));
				if(checkWrite < 0) {
					printf("Error Writing\n");
					parentFile = fopen(parentFilePath, "a");
					char* failWriteTime = getTimeStamp();
					fprintf(parentFile, "[%s] LikesServer%d failed\n", failWriteTime, i);
					fclose(parentFile);
					failCount++;
					exit(1);
				}

				//Used to retrieve return message from the PrimaryLikesServer
				//0 for success
				char returnBuffer[256];
				int rec = read(primaryLikesServer, returnBuffer, sizeof(returnBuffer));
				if(rec > 0) {
					returnBuffer[rec] = '\0';
					int returnCode = atoi(returnBuffer);
					int logSuccess = logNumLikes(i, numLikes, returnCode);
					if(logSuccess == 1) {
						printf("Error Logging LikesServer%d\n", i);
						parentFile = fopen(parentFilePath, "a");
						char* failRecTime = getTimeStamp();
						fprintf(parentFile, "[%s] LikesServer%d failed\n", failRecTime, i);
						fclose(parentFile);
						failCount++;
						exit(1);
					}
				}
				//Close the socket connection for this iteration
				close(primaryLikesServer);
			}
		} else if (child_pid < 0) {
			//Code for if the child process fails to fork
			printf("Fork Failed\n");
			parentFile = fopen(parentFilePath, "a");
			char* failLogTime = getTimeStamp();
			fprintf(parentFile, "[%s] LikesServer%d failed\n", failLogTime, i);
			fclose(parentFile);
			failCount++;
		} else {
			likesServerPIDS[i] = child_pid;
		}

	}
	//Final cleanup, log any failures in the parent log file
	int likesServerStatus;
	for(int i=0; i<likesServerAmount; i++) {
		waitpid(likesServerPIDS[i], &likesServerStatus, 0);
	}

	parentFile = fopen(parentFilePath, "a");
	char* finalLogTime = getTimeStamp();
	fprintf(parentFile, "[%s] Process terminated with %d failures in the LikesServer\n", finalLogTime, failCount);
	fclose(parentFile);
	return 0;
}
