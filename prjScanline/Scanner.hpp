#include <WinSock2.h>
#include <iostream>
#include <future>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

class Scanner
{
private:

	//-----------------------------------------------------------------
	//				Variables
	//-----------------------------------------------------------------
	SOCKET Sock;

	//-----------------------------------------------------------------
	//				Init Winsock
	//-----------------------------------------------------------------
	bool InitWinsock(SOCKET &argSock)
	{
		WSADATA WsaDat;

		// Initialize Winsock
		if (WSAStartup(MAKEWORD(2, 2), &WsaDat) != 0)
		{
			WSACleanup();
			return(false);
		}

		// Define Socket
		argSock = socket(AF_INET, SOCK_STREAM, 0);
		if (argSock == INVALID_SOCKET)
		{
			WSACleanup();
			return(false);
		}

		return(true);
	}

	//-----------------------------------------------------------------
	//				Stop Winsock
	//-----------------------------------------------------------------
	void StopWinsock(SOCKET &argSock)
	{
		WSACleanup();
		closesocket(Sock);
	}

	//-----------------------------------------------------------------
	//				Increment IP Address
	//-----------------------------------------------------------------
	void AdvGetNextIPPointer(std::string *host)
	{
		struct in_addr paddr;

		// Convert String To ULONG
		u_long addr = inet_addr(host->c_str()); // inet_addr() Doesnt Support IPV6

		// Convert ULONG To Network Byte Order
		addr = ntohl(addr);

		// Incremement By 1
		addr += 1;

		// Convert Network Byte Order Back To ULONG
		addr = htonl(addr);

		// Convert ULONG Back To in_addr Struct
		paddr.S_un.S_addr = addr;

		// Convert Back To String
		*host = inet_ntoa(paddr);
	}

	//-----------------------------------------------------------------
	//			This Function Checks If A Remote Port Is Open
	//-----------------------------------------------------------------
	bool PortCheck(std::string ip, int port)
	{
		std::mutex outputmutex;
		struct sockaddr_in	sin;
		unsigned long		blockcmd = 1;

		outputmutex.lock();
		{
			std::cout << "Thread: " << std::this_thread::get_id() << " Currently Looking At: " << ip << ":" << port << std::endl;
		}
		outputmutex.unlock();

		// Setup Winsock Struct
		sin.sin_family           = AF_INET;
		sin.sin_addr.S_un.S_addr = inet_addr(ip.c_str());	// Update this function to be safe
		sin.sin_port             = htons(port);

		// Set Socket To Non-Blocking
		ioctlsocket(Sock, FIONBIO, &blockcmd);

		// Make Connection
		connect(Sock, (struct sockaddr *)&sin, sizeof(sin));

		// Setup Timeout
		TIMEVAL	tv;
		tv.tv_sec = 5; // Seconds
		tv.tv_usec = 0;

		// Setup Result Set
		FD_SET rset;
		FD_ZERO(&rset);
		FD_SET(Sock, &rset);

		// Move Pointer
		int i = select(0, 0, &rset, 0, &tv);

		// Close Socket
		closesocket(Sock);

		// Return Result
		return i > 0;
	}

	void ScanThread(std::string argStartAddress, std::string argEndAddress, int argPort)
	{
		std::mutex hLockingMutex;

		std::vector<std::pair<std::string, std::future<bool>>> JobPool;
		
		while (argStartAddress != argEndAddress)
		{
			hLockingMutex.lock();
			{
				// Incremement The Address
				AdvGetNextIPPointer(&argStartAddress);

				auto f = std::async(PortCheck, argStartAddress, argPort);

				// Push The Job To Our Stack
				JobPool.push_back(std::make_pair(
					argStartAddress,
					f
				));
			}
			hLockingMutex.unlock();
		}

		// At This Point All Jobs Have Been Submitted! Now We Wait For Our Callbacks To Give Us Results
		for (auto &Job : JobPool)
		{
			std::cout << "Result Of " << Job.first.c_str() << " : " << Job.second.get() << std::endl;
		}

		std::cout << "Done With Scanning!" << std::endl;
	}

public:

	Scanner()
	{
		// Try To Start Winsock
		if (InitWinsock(Sock) != true)
		{
			std::wcout << "Failed To Initalize Winsock!" << std::endl;
		}

		// Demo Code
		ScanThread("192.168.0.1", "192.168.0.255", 80);
	}

	~Scanner()
	{
		StopWinsock(Sock);
	}


};