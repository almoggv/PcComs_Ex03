#include <iostream>
#include <string.h>
#include <time.h>
#include <winsock2.h>




using namespace std;


// Successful Status Codes
#define OK 200
#define CREATED 201
#define NO_CONTENT 204
#define ACCEPTED 202

const int BUFFER_LEN = 10000;

struct SocketState
{
	SOCKET id;			// Socket handle
	int	recv;			// Receiving?
	int	send;			// Sending?
	int sendSubType;	// Sending sub-type
	char buffer[BUFFER_LEN];
	int len;
	string lang;		// Language of page
	string data;		// Data in the body
	string FileName;	// if exists
	clock_t StartTime;	// timeout reference
};

string getCurrWorkingDir();
void GetHandler(int index, char* sendBuff, SocketState sockets[]); // handles get request
void HeadHandler(int index, char* sendBuff, SocketState sockets[]); // handles head request
void PutHandler(int index, char* sendBuff, SocketState sockets[]); // handles put request
void DeleteHandler(int index, char* sendBuff, SocketState sockets[]); // handles delete request