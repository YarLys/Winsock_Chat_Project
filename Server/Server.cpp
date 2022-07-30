#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
#include <thread>
#include <string>

#pragma comment(lib, "ws2_32.lib")  //Link with Windows library

using namespace std;

const short BUFF_SIZE = 1024; // Max line of the msg
const short MAXXCONN = 50; // 50 connections maximally
vector<char> servBuff(BUFF_SIZE); 
vector<string> names(MAXXCONN); // Client's names
int timeout = 0, socknum = 0, i = 0, j = 0;
u_long nfds = 1;
bool ServerWorks = true, close_conn = false, compress = false;
string Startmsg = "Hello! Type your name here: \0", MsgtoClient;

void quit(SOCKET &ServSock)
{
	closesocket(ServSock);
	WSACleanup();
}

int main()
{
	// Winsock Initialization // 
	WSADATA wsData; //information about sockets on this OS and other system inf
	int erStatus = WSAStartup(514, &wsData); //541 = MAKEWORD(2,2), because we need socket version 2 || &wsData - the reference to our data-structure about sokets on this PC
	if (erStatus != 0)
	{
		cout << "Error: Winsock version initialization - " << WSAGetLastError() << endl; //Get last error if sockets initialization doesn't work
		return 1;
	}
	else
	{
		cout << "Winsock initialization is OK!" << endl;
	}

	// Server Socket Initialization //
	SOCKET ServSock = socket(AF_INET, SOCK_STREAM, 0); //  IPv4 adress family, TCP protocol and unnecessary type of protocol = 0
	if (ServSock == INVALID_SOCKET)
	{
		cout << "Error: Server socket initialization - " << WSAGetLastError() << endl;
		quit(ServSock); // ESSENTIAL THINGS: to close all sockets and uninitialize Win32API sockets 
		return 1;
	}
	else
	{
		cout << "Server socket initialization is OK!" << endl;
	}

	// IP Translation //
	in_addr ip_to_num; // Our ipv4 addres, but in other shape especially for TCP/IP
	erStatus = inet_pton(AF_INET, "127.0.0.1", &ip_to_num); // Transforms it from 127.0.0.1. to other special format
	if (erStatus <= 0)
	{
		cout << "Error: IP translation to special numeric format" << endl;
		quit(ServSock);
		return 1;
	}
	else
	{
		cout << "IP translation is OK!" << endl;
	}

	// Let's make our socket non-blocking //
	u_long mode = 1;
	erStatus = ioctlsocket(ServSock, FIONBIO, &mode);
	if (erStatus != NO_ERROR)
	{
		cout << "Error: In ioctlsocket - " << WSAGetLastError() << endl;
		quit(ServSock);
		return 1;
	}
	else cout << "Non-blocking mode established succesfully" << endl;

	// Binding Socket //
	sockaddr_in servInfo; // Preparation for socket's binding	
	ZeroMemory(&servInfo, sizeof(servInfo));

	servInfo.sin_family = AF_INET;
	servInfo.sin_addr = ip_to_num;
	servInfo.sin_port = htons(1234); // Socket's preparation is done

	erStatus = bind(ServSock, (sockaddr*)&servInfo, sizeof(servInfo)); // Binding socket to server info || (sockaddr*) converts servInfo to this type
	if (erStatus != 0)
	{
		cout << "Error: Socket binding to server - " << WSAGetLastError() << endl;
		quit(ServSock);
		return 1;
	}
	else
	{
		cout << "Socket binding to server is OK!" << endl;
	}

	// Socket Listening //
	erStatus = listen(ServSock, SOMAXCONN); // Listening to connections 
	if (erStatus != 0)
	{
		cout << "Error: In listening to - " << WSAGetLastError() << endl;
		quit(ServSock);
		return 1;
	}
	else
	{
		cout << "Listening..." << endl;
	}

	// Initializing pollfd structure // 
	struct pollfd fds[MAXXCONN]; 
	memset(fds, 0, sizeof(fds));
	fds[0].fd = ServSock;
	fds[0].events = POLLIN;
	timeout = 60000; // The server will work for 60 sec without new connections and then it will stop

	do
	{
		MsgtoClient = "";
		i = 0;
		while (servBuff[i] != '\0')
		{
			servBuff[i] = '\0';
			i++;
		}

		erStatus = WSAPoll(fds, nfds, timeout);
		if (erStatus == SOCKET_ERROR)
		{
			cout << "Error: In WSAPolling - " << WSAGetLastError() << endl;
			break;
		}
		if (erStatus == 0)
		{
			cout << "The time for connection expired" << endl;
			break;
		}

		socknum = nfds;
		for (i = 0; i < socknum; i++)
		{
			if (fds[i].revents == 0) continue;			

			if (fds[i].fd == ServSock) // If its our listening socket
			{
				SOCKET Clientconn = 0;
				do
				{
					sockaddr_in clientInfo;  // Information about connected client
					ZeroMemory(&clientInfo, sizeof(clientInfo));
					int clientInfo_size = sizeof(clientInfo);

					Clientconn = accept(ServSock, (sockaddr*)&clientInfo, &clientInfo_size); // We have to accept all incoming connections
					if (Clientconn == -1) break; // Почему-то заоблачное значение в Clientconn, которое похоже на переполнение, проходит проверку на == -1, но не проходит проверку на < 0. Из-за этого создаётся лишний несуществующий сокет. Похоже реально переполнение
					
					if (Clientconn < 0)
					{
						if (WSAGetLastError() != WSAEWOULDBLOCK) // We've accepted all incoming connections
						{
							cout << "Error: In accepting a new connection" << endl;
							ServerWorks = false;
						}
						break;
					}
					else cout << "Client connected succesfully!" << endl;					

					fds[nfds].fd = Clientconn;
					fds[nfds].revents = 0;
					fds[nfds].events = POLLIN; 		

					erStatus = send(fds[nfds].fd, Startmsg.data(), Startmsg.size(), 0);
					if (erStatus < 0)
					{
						cout << "Error: In sending - " << WSAGetLastError() << endl; // Ignore now, just for debug
					}

					nfds++;
					
				} while (Clientconn != -1);
			}
			else
			{
				close_conn = false;
				if (fds[i].revents == 3) // It means that client has disconnected
				{
					close_conn = true; 
					cout << "Client " << names[i] << " has disconnected" << endl;
					names[i] = "";
				}
				else
				{
					erStatus = recv(fds[i].fd, servBuff.data(), servBuff.size(), 0);
					if (erStatus == WSAEWOULDBLOCK) break;
					if (erStatus < 0)
					{
						if ((erStatus == WSAENOTCONN) or (erStatus == WSAECONNRESET)) cout << "Client has disconnected" << endl;
						else cout << "Error: recv() failed, closing this connection - " << WSAGetLastError() << endl;
						close_conn = true;
						break;
					}
					
					if (names[i] == "") // If a user doesn't have the name
					{										
						MsgtoClient = servBuff.data();
						MsgtoClient = MsgtoClient.substr(0, MsgtoClient.length()-1); // Clear '\n'
						names[i] = MsgtoClient;
						MsgtoClient = MsgtoClient + " has connected!\n";						
					}
					else MsgtoClient = names[i] + ':' + servBuff.data();
					MsgtoClient = MsgtoClient + '\0';

					for (j = 1; j < nfds; j++) // Sending a msg to all clients
					{
						if (j != i)
						{							
							erStatus = send(fds[j].fd, MsgtoClient.data(), MsgtoClient.size(), 0);
							//erStatus = send(fds[j].fd, servBuff.data(), servBuff.size(), 0);
							if (erStatus < 0)
							{
								cout << "Error: In sending - " << WSAGetLastError() << endl; // Ignore now, just for debug
							}
						}
					} // Sending a msg to all clients					
				}
				if (close_conn)
				{
					closesocket(fds[i].fd);
					fds[i].fd = -1;
					compress = true;
				}
			}
		}

		if (compress)  // if some connections were closed, we have to clean up them from our structure
		{
			compress = false;
			for (i = 0; i < nfds; i++)
			{
				if (fds[i].fd == -1)
				{
					for (j = i; j < nfds; j++)
					{
						fds[j] = fds[j + 1];
						names[j] = names[j + 1];
					}
					nfds--;
					i--;
				}
			}
		}
	} while (ServerWorks);

	for (int i = 0; i < nfds; i++)
	{
		closesocket(fds[i].fd);
	}
	quit(ServSock);
	return 0;		
}