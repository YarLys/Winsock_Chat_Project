#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <thread>

#pragma comment(lib, "ws2_32.lib") // Link with Windows library

using namespace std;

int SendingInf(short& packet_size, SOCKET& ClientSock, vector<char> &clientBuff)
{
	//cout << "Your client message to server: ";
	fgets(clientBuff.data(), clientBuff.size(), stdin);
	cout << endl;

	// Checks if chat wants to quit chatting 
	//if (clientBuff[0] == 'q' and clientBuff[1] == 'u' and clientBuff[2] == 'i' and clientBuff[3] == 't')
	//{
	//	shutdown(ClientSock, SD_BOTH); // Disable send and recieve operations with socket		
	//	return 1;
	//}

	packet_size = send(ClientSock, clientBuff.data(), clientBuff.size(), 0); // Send information from client

	if (packet_size == SOCKET_ERROR)
	{
		cout << "Can't send message to the server - " << WSAGetLastError() << endl;
		return 1;
	}

	return -1; // Message sent succesfully
}

void ReceivingInf(short &packet_size, SOCKET& ClientSock, vector<char> &servBuff)
{
	while (true)
	{
		packet_size = recv(ClientSock, servBuff.data(), servBuff.size(), 0);
		if (packet_size == SOCKET_ERROR)
		{
			cout << "Error: Can't receive information from the server - " << WSAGetLastError() << endl;
			break;
		}
		else cout << servBuff.data() << endl;	
	}
}

void quit(SOCKET& ClientSock)
{
	closesocket(ClientSock);
	WSACleanup();
}

int main()
{
	const short BUFF_SIZE = 1024;

    WSADATA wsData; //information about sockets on this OS and other system inf
	int erStatus = WSAStartup(514, &wsData); //541 = MAKEWORD(2,2), because we need socket version 2 || and the reference to our data-structure about sokets on this PC
	if (erStatus != 0)
	{
		cout << "Error: Winsock version initialization - " << WSAGetLastError() << endl; //Get last error if sockets initialization doesn't work
		return 1;
	}
	else
	{
		cout << "Winsock initialization is OK!" << endl;
	}

	SOCKET ClientSock = socket(AF_INET, SOCK_STREAM, 0); //  IPv4 adress family, TCP protocol and unnecessary type of protocol = 0
	if (ClientSock == INVALID_SOCKET)
	{
		cout << "Error: Client socket initialization - " << WSAGetLastError() << endl;
		quit(ClientSock); // ESSENTIAL THINGS: to close all sockets and uninitialize Win32API sockets 
		return 1;
	}
	else
	{
		cout << "Client socket initialization is OK!" << endl;
	}

	in_addr ip_to_num; // Our ipv4 addres, but in other shape especially for TCP/IP
	erStatus = inet_pton(AF_INET, "127.0.0.1", &ip_to_num); // Transforms it from 127.0.0.1. to other special format
	if (erStatus <= 0)
	{
		cout << "Error: IP translation to special numeric format" << endl;
		return 1;
	}
	else
	{
		cout << "IP translation is OK!" << endl;
	}

	sockaddr_in servInfo; // Preparation for socket's creation	
	ZeroMemory(&servInfo, sizeof(servInfo));

	servInfo.sin_family = AF_INET;
	servInfo.sin_addr = ip_to_num;
	servInfo.sin_port = htons(1234); // Socket's preparation is done!

	erStatus = connect(ClientSock, (sockaddr*)&servInfo, sizeof(servInfo)); // Binding socket to server info || (sockaddr*) converts servInfo to this type
	if (erStatus != 0)
	{
		cout << "Error: Connection to server is failed  - " << WSAGetLastError() << endl;
		quit(ClientSock);
		return 1;
	}
	else
	{
		cout << "Connection to server is OK! Ready to send messages to Server " << endl;
	}

	vector <char> clientBuff(BUFF_SIZE), servBuff(BUFF_SIZE); // Buffers for sending and recieving information 
	short packet_size = 0;

	thread Receive([&packet_size, &ClientSock, &servBuff]()
		{
			ReceivingInf(packet_size, ClientSock, servBuff);			
		});

	while (true)
	{
		// Sends messages to the server
		erStatus = SendingInf(packet_size, ClientSock, clientBuff);		
		if (erStatus == 1) break;
	}

	Receive.join(); // Receives messages from the server

	quit(ClientSock);
	return 0;
}