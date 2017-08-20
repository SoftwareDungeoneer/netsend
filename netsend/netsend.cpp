#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>

#include <stdio.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <fstream>

#pragma comment(lib, "ws2_32.lib")

int SocketError();

struct WSAGuard {
	WSAGuard(WSADATA& data) { WSAStartup(MAKEWORD(2, 2), &data); }
	~WSAGuard() { WSACleanup(); }
};

int SocketError()
{
	int sockErr = WSAGetLastError();
	DWORD size{ 4095 };
	TCHAR* buffer = new TCHAR[size + 1];
	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, sockErr, 0, buffer, size, NULL);
	printf("Winsock error: %d\n\t%S\n", sockErr, buffer);
	delete[] buffer;
	return sockErr;
}

void print_usage()
{
	printf(
		"netsend [-h <host>] [-p <port>] [-f <file>]\n"
		"\tSends data over a network connection.\n"
		"\n"
		"\tArguments:\n"
		"\t\t-h <host>: Connects to the specified host\n"
		"\t\t-p <port>: Specifies the remote port to connect to\n"
		"\t\t-f <file>: Transmit the given file, if none then transmit from cli\n"
	);
}

const unsigned short g_defaultPort{ htons(23455) };

int main(int argc, char** argv)
{
	using namespace std;
	ios_base::sync_with_stdio();
	ifstream infile;

	const char* remoteHostStr{ 0 };
	//unsigned short remotePort{ g_defaultPort };
	const char* remotePortStr{ 0 };
	const char* xmitFileName{ 0 };

	try
	{
		if (argc < 2)
			throw argc;

		int arg;
		for (arg = 1; arg < (argc - 1); ++arg)
		{
			if (!strcmp("-h", argv[arg]))
				remoteHostStr = argv[++arg];
			else if (!strcmp("-p", argv[arg]))
				remotePortStr = argv[++arg];
				//remotePort = strtoushort(argv[++arg]);
			else if (!strcmp("-f", argv[arg]))
				xmitFileName = argv[++arg];
			else
			{
				printf("Invalid argument: %s\n", argv[arg]);
				throw arg;
			}
		}
		if (arg < argc) // Pick up the hanging argument as either the remote host or the file
		{
			if (!remoteHostStr)
				remoteHostStr = argv[argc - 1];
			else if (!xmitFileName)
				xmitFileName = argv[argc - 1];
		}

		if (!remoteHostStr)
			throw remoteHostStr;
	}
	catch (...) {
		print_usage();
		return EXIT_FAILURE;
	}

	if (xmitFileName)
		infile.open(xmitFileName);

	WSADATA wsaData;
	WSAGuard wsaGuard{ wsaData };

	addrinfo* pAddrinfo{ 0 };
	try
	{
		if (getaddrinfo(remoteHostStr, remotePortStr, NULL, &pAddrinfo))
			throw SocketError();

		SOCKET sock = socket(pAddrinfo->ai_family, pAddrinfo->ai_socktype, pAddrinfo->ai_protocol);
		if (sock == INVALID_SOCKET)
			throw SocketError();
		if (connect(sock, pAddrinfo->ai_addr, (int)pAddrinfo->ai_addrlen))
			throw SocketError();

		if (infile.is_open())
			cin.rdbuf(infile.rdbuf());

		while (cin)
		{
			string readStr;
			getline(cin, readStr);
			readStr += "\n";
			if (send(sock, readStr.c_str(), (int)readStr.length(), 0) == SOCKET_ERROR)
				throw SocketError();
		}
		shutdown(sock, SD_BOTH);
		closesocket(sock);
	}
	catch (...)
	{
	}
	freeaddrinfo(pAddrinfo);

	return EXIT_SUCCESS;
}
