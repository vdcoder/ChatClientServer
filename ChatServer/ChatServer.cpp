// ChatServer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <asio.hpp>
#include "ChatServerEngine.h"

int main()
{
	unsigned short nPort;
	std::cout << "Enter port:" << std::endl;
	std::cin >> nPort;

	try
	{
		asio::io_service IoService;
		
		ChatServerEngine Server(IoService, nPort);

		IoService.run();
	}
	catch (std::exception& e)
	{
		std::cerr << e.what() << std::endl;
	}

    return 0;
}

