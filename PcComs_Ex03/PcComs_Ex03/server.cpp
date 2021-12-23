#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <time.h>
#include "httpHandlersUtil.h"




//Socket Status
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;

//Supported methods
const int GET = 1;
const int POST = 2;
const int HEAD = 3;
const int PUT = 4;
const int DEL = 5;
const int TRACE = 6;
const int OPTIONS = 7;

const char* GET_string = "GET";
const char* POST_string = "POST";
const char* HEAD_string = "HEAD";
const char* PUT_string = "PUT";
const char* DEL_string = "DELETE";
const char* TRACE_string = "TRANCE";
const char* OPTIONS_string = "OPTIONS";
const char* FILE_NOT_FOUND = "FileNotFound.txt";



//Functions
bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);
void extractHttpFirstVariable(string source, string* key, string* value, int offset);
string extractHttpData(string source, int offset);
int extractHttpMethod(string buffer, int bufferLen, int offset);
void cleanupBuffer(int index);


//Sockets
const int HTTP_PORT = 8080;
const int MAX_SOCKETS = 60;

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;


void main()
{
	// Initialize Winsock (Windows Sockets).

	// Create a WSADATA object called wsaData.
	// The WSADATA structure contains information about the Windows 
	// Sockets implementation.
	WSAData wsaData;

	// Call WSAStartup and return its value as an integer and check for errors.
	// The WSAStartup function initiates the use of WS2_32.DLL by a process.
	// First parameter is the version number 2.2.
	// The WSACleanup function destructs the use of WS2_32.DLL by a process.
	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
		cout << "Http Server: Error at WSAStartup()\n";
		return;
	}

	// Server side:
	// Create and bind a socket to an internet address.
	// Listen through the socket for incoming connections.

	// After initialization, a SOCKET object is ready to be instantiated.

	// Create a SOCKET object called listenSocket. 
	// For this application:	use the Internet address family (AF_INET), 
	//							streaming sockets (SOCK_STREAM), 
	//							and the TCP/IP protocol (IPPROTO_TCP).
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	// Check for errors to ensure that the socket is a valid socket.
	// Error detection is a key part of successful networking code. 
	// If the socket call fails, it returns INVALID_SOCKET. 
	// The if statement in the previous code is used to catch any errors that
	// may have occurred while creating the socket. WSAGetLastError returns an 
	// error number associated with the last error that occurred.
	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Http Server: Error at socket(): " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	// For a server to communicate on a network, it must bind the socket to 
	// a network address.

	// Need to assemble the required data for connection in sockaddr structure.

	// Create a sockaddr_in object called serverService. 
	sockaddr_in serverService;
	// Address family (must be AF_INET - Internet address family).
	serverService.sin_family = AF_INET;
	// IP address. The sin_addr is a union (s_addr is a unsigned long 
	// (4 bytes) data type).
	// inet_addr (Iternet address) is used to convert a string (char *) 
	// into unsigned long.
	// The IP address is INADDR_ANY to accept connections on all interfaces.
	serverService.sin_addr.s_addr = INADDR_ANY;
	// IP Port. The htons (host to network - short) function converts an
	// unsigned short from host to TCP/IP network byte order 
	// (which is big-endian).
	serverService.sin_port = htons(HTTP_PORT);

	// Bind the socket for client's requests.

	// The bind function establishes a connection to a specified socket.
	// The function uses the socket handler, the sockaddr structure (which
	// defines properties of the desired connection) and the length of the
	// sockaddr structure (in bytes).
	if (SOCKET_ERROR == bind(listenSocket, (SOCKADDR*)&serverService, sizeof(serverService)))
	{
		cout << "Http Server: Error at bind(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}

	// Listen on the Socket for incoming connections.
	// This socket accepts only one connection (no pending connections 
	// from other clients). This sets the backlog parameter.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Http Server: Error at listen(): " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return;
	}
	addSocket(listenSocket, LISTEN);

	cout << "Http Server - listening on port:" << HTTP_PORT << "\n";	//logging

	// Accept connections and handles them one by one.
	while (true)
	{
		// The select function determines the status of one or more sockets,
		// waiting if necessary, to perform asynchronous I/O. Use fd_sets for
		// sets of handles for reading, writing and exceptions. select gets "timeout" for waiting
		// and still performing other operations (Use NULL for blocking). Finally,
		// select returns the number of descriptors which are ready for use (use FD_ISSET
		// macro to check which descriptor in each set is ready to be used).
		fd_set waitRecv;
		FD_ZERO(&waitRecv);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) || (sockets[i].recv == RECEIVE))
				FD_SET(sockets[i].id, &waitRecv);
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);
		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
				FD_SET(sockets[i].id, &waitSend);
		}

		//
		// Wait for interesting event.
		// Note: First argument is ignored. The fourth is for exceptions.
		// And as written above the last is a timeout, hence we are blocked if nothing happens.
		//
		int nfd;
		nfd = select(0, &waitRecv, &waitSend, NULL, NULL);
		if (nfd == SOCKET_ERROR)
		{
			cout << "Http Server: Error at select(): " << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;
				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;
				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	// Closing connections and Winsock.
	cout << "Http Server: Closing Connection.\n";
	closesocket(listenSocket);
	WSACleanup();
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			socketsCount++;
			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	sockets[index].len = 0;
	sockets[index].buffer[0] = '\0';
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Time Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Http Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Time Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
		cout << "\t\tToo many connections, dropped!\n";
		closesocket(id);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;
	string newFileName;
	string temp;
	int len = sockets[index].len;

	if ( ((double)len / (double)BUFFER_LEN) * 100 >= 90 ) //buffer filled more than 90% 
	{
		cleanupBuffer(index);
	}


	int bytesRecv = recv(msgSocket, &sockets[index].buffer[0], sizeof(sockets[index].buffer) - len, 0);
	sockets[index].len = bytesRecv;

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Time Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	
	if (bytesRecv == 0) //Socket ending connection
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}
	else
	{
		sockets[index].buffer[len + bytesRecv] = '\0'; //add the null-terminating to make it a string
		std::cout << "Http Server: Recieved: " << bytesRecv << " bytes of \"\n" << &sockets[index].buffer[len] << "\n\" message.\n"; //logging
		
		int method = extractHttpMethod(sockets[index].buffer, sockets[index].len, 0);

		

		switch (method)
		{
		case GET:
		case HEAD:
		{
			string fileName;
			string lang;
			extractHttpFirstVariable((string)(sockets[index].buffer), &fileName, &lang, 0);

			sockets[index].FileName = fileName;
			if (strcmp(lang.c_str(), "en") == 0 || strcmp(lang.c_str(), "EN") == 0)
			{
				lang = "English";
			}
			else if (strcmp(lang.c_str(), "he") == 0 || strcmp(lang.c_str(), "HE") == 0)
			{
				lang = "Hebrew";
			}
			else if (strcmp(lang.c_str(), "fr") == 0 || strcmp(lang.c_str(), "FR") == 0)
			{
				lang = "French";
			}
			else if (strcmp(lang.c_str(), "unknown") == 0)
			{
				lang = "Unknown";
			}
			else
			{
				sockets[index].FileName = "NotFound";
				lang = "NotFound";
			}
			
			if(strcmp(fileName.c_str(), "favicon.ico") != 0 )
			{
				sockets[index].lang = lang;
				sockets[index].sendSubType = method;
				sockets[index].send = SEND;
			}
			else 
			{
				sockets[index].sendSubType = 0;
			}
			
		}
			break;
		case POST:
		{
			sockets[index].data = extractHttpData((string)sockets[index].buffer , 0);
			sockets[index].send = SEND;
			sockets[index].sendSubType = POST;
		}
			break;
		case PUT:
		{
			extractHttpFirstVariable((string)(sockets[index].buffer), &newFileName, &temp, 0);
			sockets[index].FileName = newFileName;
			sockets[index].data = extractHttpData((string)(sockets[index].buffer), sockets[index].len);
			sockets[index].send = SEND;
			sockets[index].sendSubType = PUT;
		}
			break;
		case DEL:
		{
			extractHttpFirstVariable((string)(sockets[index].buffer), &newFileName, &temp, 0);
			sockets[index].FileName = newFileName;
			sockets[index].data = extractHttpData((string)(sockets[index].buffer), sockets[index].len);
			sockets[index].send = SEND;
			sockets[index].sendSubType = DEL;
		}
			break;
		case TRACE:
		{
			extractHttpFirstVariable((string)(sockets[index].buffer), &newFileName, &temp, 0);
			sockets[index].FileName = newFileName;
			sockets[index].send = SEND;
			sockets[index].sendSubType = TRACE;
		}
			break;
		case OPTIONS:
			sockets[index].send = SEND;
			sockets[index].sendSubType = OPTIONS;

			break;
		default:

			break;
		}
	}
	sockets[index].len = 0;
	sockets[index].StartTime = clock();
}

void sendMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;
	int bytesSent = 0;
	char* sendBuff = new char[BUFFER_LEN];
	time_t timer;
	time(&timer);
	clock_t end = clock();
	double timePassed = ((double)end - sockets[index].StartTime) / CLOCKS_PER_SEC;

	if (timePassed > 120)
	{
		sprintf(sendBuff, "HTTP/1.1 408 Request Timeout\r\nconnection: close\r\n\r\n");
	}
	else
	{
		if (sockets[index].sendSubType == GET)
		{
			GetHandler(index, sendBuff, sockets);
		}
		else if (sockets[index].sendSubType == POST)
		{
			cout << "The data recived in this request: " << sockets[index].data << endl;
			sprintf(sendBuff, "HTTP/1.1 200 OK\r\n\Connection: keep-alive\r\naccept-language:he=0.8, en=1,fr=0.7\r\ncontent-language:en\r\ncontent-type:text/html\r\nContent-Length: 0\r\nDate: %s \r\n\r\n", ctime(&timer));
		}
		else if (sockets[index].sendSubType == HEAD)
		{
			HeadHandler(index, sendBuff, sockets);
		}
		else if (sockets[index].sendSubType == PUT)
		{
			PutHandler(index, sendBuff, sockets);
		}
		else if (sockets[index].sendSubType == DEL)
		{
			DeleteHandler(index, sendBuff, sockets);
		}
		else if (sockets[index].sendSubType == TRACE)
		{
			string path = "c:/temp/";
			path.append(sockets[index].FileName);
			sprintf(sendBuff, "HTTP/1.1 200 OK\r\ncontent-type:text/html\r\nContent-Length: %d\r\nDate: %s \r\n\r\n%s", path.length(), ctime(&timer), path.c_str());
		}
		else if (sockets[index].sendSubType == OPTIONS)
		{
			sprintf(sendBuff, "HTTP/1.1 204 No Content\r\n\Allow: GET, HEAD, POST, PUT, DELETE, TRACE, OPTIONS\r\ncontent-type:text/html\r\nContent-Length: %d\r\nDate: %s \r\n\r\n", sockets[index].len, ctime(&timer));
		}
	}

	sendBuff[BUFFER_LEN - 1] = '\0';
	bytesSent = send(msgSocket, sendBuff, (int)strlen(sendBuff), 0);
	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;
		return;
	}

	cout << "Web Server: Sent: " << bytesSent << "\\" << strlen(sendBuff) << " bytes of \"" << sendBuff << endl << endl << endl;

	sockets[index].send = IDLE;
	delete[] sendBuff;

	sockets[index].send = IDLE;
}


void extractHttpFirstVariable(string source, string* key, string* value, int offset = 0) 
{
	int fileNameStart = source.find("/", offset);													// find file's name start position in request
	int fileEndOfVariables = source.find("HTTP", offset);
	int fileNameEnd = source.find("=", fileNameStart + offset);										// find file's name end position in request

	if (fileEndOfVariables < fileNameEnd || fileNameEnd == string::npos)
	{
		// no '=' found in argumnets -> a check a single key
		int fileNameEnd = source.find(" ", fileNameStart + offset);
		*key = source.substr(offset + fileNameStart + 1, offset + fileNameEnd - fileNameStart - 1);				// create sub string containing file name
		*value = string("unknown");
	}
	else 
	{
		*key = source.substr(offset+ fileNameStart + 1, offset + fileNameEnd - fileNameStart - 1);				// create sub string containing file name

		int langEnd = source.find(" ", offset + fileNameEnd);											// find FileName variable's value
		*value = source.substr(offset + fileNameEnd + 1, offset + langEnd - fileNameEnd - 1);						// create sub string containing language

	}

	if ((*key).find(".txt") == string::npos) 
	{
		(*key) = string(FILE_NOT_FOUND);
		(*value) = string(FILE_NOT_FOUND);
	}


}

string extractHttpData(string source, int offset = 0)
{
	int numOfLines = 12;
	int currEndLinePos = 0;
	string result;

	for (size_t i = 0; i < numOfLines ; i++)
	{
		currEndLinePos = source.find("\n\r", offset + currEndLinePos);
	}
	result = source.substr(currEndLinePos + offset);

	if (strcmp(&result[0], "GET") == 0) {
		result = string(" ");
	}
	return result;
}

int extractHttpMethod(string buffer, int bufferLen, int offset = 0) 
{
	int lenOfMethod = buffer.find(" ", offset );
	string method = buffer.substr(offset, lenOfMethod);
	int result = 0;

	if (strcmp(method.c_str(), GET_string) == 0) {
		result = GET;
	}
	else if (strcmp(method.c_str(), POST_string) == 0) {
		result = POST;
	}
	else if (strcmp(method.c_str(), HEAD_string) == 0) {
		result = HEAD;
	}
	else if (strcmp(method.c_str(), PUT_string) == 0) {
		result = PUT;
	}
	else if (strcmp(method.c_str(), DEL_string) == 0) {
		result = DEL;
	}
	else if (strcmp(method.c_str(), TRACE_string) == 0) {
		result = TRACE;
	}
	else if (strcmp(method.c_str(), OPTIONS_string) == 0) {
		result = OPTIONS;
	}

	return result;
}

void cleanupBuffer(int index) {
	sockets[index].len = 0;
	sockets[index].buffer[0] = '\0';
}

