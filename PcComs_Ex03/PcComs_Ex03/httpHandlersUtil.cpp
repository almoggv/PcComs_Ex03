#define _CRT_SECURE_NO_WARNINGS
#include "httpHandlersUtil.h"


string getCurrWorkingDir(){

	TCHAR fileLocation[100];
	GetModuleFileName(NULL, fileLocation, 100);
	std::wstring ws(fileLocation);
	std::string result(ws.begin(), ws.end());

	while (result.back() != '\\') {
		result.pop_back();
	}
	//result.pop_back();
	//result.pop_back();
	
	return result;
}


void GetHandler(int index, char* sendBuff, SocketState sockets[])
{
	time_t timer;
	time(&timer);
	string content;
	FILE* file;

	string fileName = getCurrWorkingDir();
	fileName.append(sockets[index].FileName); // Creates file name along with the current working directory

	file = fopen(fileName.c_str(), "r");
	char c;
	if (file && strcmp(sockets[index].FileName.c_str(), "NotFound") != 0)
	{
		while ((c = getc(file)) != EOF)
			content.push_back(c);
		fclose(file);
		if (strcmp(sockets[index].lang.c_str(), "fr") == 0 || strcmp(sockets[index].lang.c_str(), "FR") == 0)
		{
			sprintf(sendBuff, "HTTP/1.1 200 OK\r\n\Connection: keep-alive\r\naccept-language:he=0.7, en=0.8,fr=1\r\ncontent-language:fr\r\ncontent-type:text/html\r\nContent-Length: %d\r\nDate: %s \r\n\r\n%s", content.length(), ctime(&timer), content.c_str());
		}
		else if (strcmp(sockets[index].lang.c_str(), "he") == 0 || strcmp(sockets[index].lang.c_str(), "HE") == 0)
		{
			sprintf(sendBuff, "HTTP/1.1 200 OK\r\n\Connection: keep-alive\r\naccept-language:he=1, en=0.8,fr=0.7\r\ncontent-language:he\r\ncontent-type:text/html\r\nContent-Length: %d\r\nDate: %s \r\n\r\n%s", content.length(), ctime(&timer), content.c_str());
		}
		else
		{
			sprintf(sendBuff, "HTTP/1.1 200 OK\r\n\Connection: keep-alive\r\naccept-language:he=0.8, en=1,fr=0.7\r\ncontent-language:en\r\ncontent-type:text/html\r\nContent-Length: %d\r\nDate: %s \r\n\r\n%s", content.length(), ctime(&timer), content.c_str());
		}
	}
	else
	{
		string notFound = "<!DOCTYPE html><html><body><h1>404 Page Not Found</h1></body></html>";
		sprintf(sendBuff, "HTTP/1.1 404 Not Found\r\ncontent-language:en\r\ncontent-type:text/html\r\nContent-Length: %d\r\nDate: %s \r\n\r\n%s", notFound.length(), ctime(&timer), notFound.c_str());
	}
}

void HeadHandler(int index, char* sendBuff, SocketState sockets[])
{
	time_t timer;
	time(&timer);
	if (strcmp(sockets[index].lang.c_str(), "French") == 0)
	{
		sprintf(sendBuff, "HTTP/1.1 200 OK\r\n\Connection: keep-alive\r\naccept-language:he=0.7, en=0.8,fr=1\r\ncontent-language:fr\r\ncontent-type:text/html\r\nContent-Length: 0\r\nDate: %s \r\n\r\n", ctime(&timer));
	}
	else if (strcmp(sockets[index].lang.c_str(), "Hebrew") == 0)
	{
		sprintf(sendBuff, "HTTP/1.1 200 OK\r\n\Connection: keep-alive\r\naccept-language:he-IL=1, en=0.8,fr=0.7\r\ncontent-language:he\r\ncontent-type:text/html\r\nContent-Length: 0\r\nDate: %s \r\n\r\n", ctime(&timer));
	}
	else if (strcmp(sockets[index].lang.c_str(), "English") == 0)
	{
		sprintf(sendBuff, "HTTP/1.1 200 OK\r\n\Connection: keep-alive\r\naccept-language:he=0.8, en=1,fr=0.7\r\ncontent-language:en\r\ncontent-type:text/html\r\nContent-Length: 0\r\nDate: %s \r\n\r\n", ctime(&timer));
	}
	else
		sprintf(sendBuff, "HTTP/1.1 404 \r\n\Connection: keep-alive\r\naccept-language:he=0.8, en=1,fr=0.7\r\ncontent-language:en\r\ncontent-type:text/html\r\nContent-Length: 0\r\nDate: %s \r\n\r\n", ctime(&timer));
}

void PutHandler(int index, char* sendBuff, SocketState sockets[])
{
	time_t timer;
	time(&timer);
	FILE* file;
	string fileName = getCurrWorkingDir();
	fileName.append(sockets[index].FileName);

	int code;
	if (file = fopen(fileName.c_str(), "r"))
	{
		char c = fgetc(file);
		if (c == EOF)
			code = NO_CONTENT;
		else
			code = OK;
		fclose(file);
	}
	else
		code = CREATED;

	file = fopen(fileName.c_str(), "w");
	if (!file)
		cout << "error opening file" << endl;
	else {
		fprintf(file, "%s", sockets[index].data.c_str());
		fclose(file);
	}
	string status = "HTTP/1.1";
	if (code == OK)
		status.append(" 200 OK\r\n");
	else if (code == NO_CONTENT)
		status.append(" 204 No Content\r\n");
	else
		status.append(" 201 Created\r\n");

	sprintf(sendBuff, "%sConnection: keep-alive\r\ncontent-type:text/html\r\nContent-Length: 0\r\nDate: %s \r\n\r\n", status.c_str(), ctime(&timer));
}

void DeleteHandler(int index, char* sendBuff, SocketState sockets[])
{
	time_t timer;
	time(&timer);
	FILE* file;
	string fileName = getCurrWorkingDir();
	fileName.append(sockets[index].FileName);
	int isRemoved;
	int code;
	if (file = fopen(fileName.c_str(), "r"))
	{
		fclose(file);
		code = NO_CONTENT;
		isRemoved = remove(fileName.c_str());
	}
	else
	{
		code = ACCEPTED;
		isRemoved = -1;
	}

	string status = "HTTP/1.1";
	if (code == NO_CONTENT)
		status.append(" 204 No Content\r\n");
	else
		status.append(" 202 Accepted\r\n");

	sprintf(sendBuff, "%sConnection: keep-alive\r\ncontent-type:text/html\r\nContent-Length:0\r\nDate: %s \r\n\r\n", status.c_str(), ctime(&timer));
}