#include "stdafx.h"
#include "ChatServerEngine.h"

using namespace std;
using asio::ip::tcp;

#define CLEANUP_INTERVAL_MS 600000 // 10 minutes

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ChatServerEngine::ChatServerEngine(asio::io_service & a_IoService, unsigned short a_nPort)
	: m_IoService(a_IoService)
	, m_Acceptor(a_IoService, tcp::endpoint(tcp::v6(), a_nPort))
	, m_bError(false)
{
	m_hTerminateEvent = CreateEvent(0, TRUE, FALSE, NULL);

	m_CleanUpWorkerThread = std::thread(&ChatServerEngine::CleanUpWorkerThread, this);

	Listen();
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ChatServerEngine::~ChatServerEngine()
{
	SetEvent(m_hTerminateEvent);

	m_CleanUpWorkerThread.join();

	CloseHandle(m_hTerminateEvent);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

bool ChatServerEngine::HasError()
{
	return m_bError;
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

std::string ChatServerEngine::DescribeConnectedClients()
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	string sClients = "";
	for (auto it = begin(m_ConnectedClients); it != end(m_ConnectedClients); it++)
	{
		sClients += (sClients != "" ? "," : "") + (*it)->Describe();
	}
	return sClients;
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatServerEngine::MessageConnectedClient(std::string a_sClientId, std::shared_ptr<std::vector<unsigned char>> a_pBuffer)
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	for (auto it = begin(m_ConnectedClients); it != end(m_ConnectedClients); it++)
	{
		if (a_sClientId == (*it)->ClientId())
		{
			(*it)->AddToWrite(a_pBuffer);
			break;
		}
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatServerEngine::Listen()
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	m_pAcceptSocket = make_shared<tcp::socket>(m_IoService);

	m_Acceptor.async_accept(
		*m_pAcceptSocket,
		std::bind(&ChatServerEngine::AcceptHandler,
			this, std::placeholders::_1));
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatServerEngine::AcceptHandler(const std::error_code& a_Error)
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	if (a_Error)
	{
		m_pAcceptSocket == nullptr;
		m_bError = true;
	}
	else
	{
		// Add client (also starts async communications)
		m_ConnectedClients.push_back(
			make_shared<ChatClientLink>(
				this, 
				m_IoService, 
				m_pAcceptSocket));

		// Back to listening
		Listen();
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatServerEngine::CleanUpWorkerThread()
{
	while (WAIT_TIMEOUT == WaitForSingleObject(m_hTerminateEvent, CLEANUP_INTERVAL_MS))
	{
		lock_guard<recursive_mutex> LockGuard(m_lock);

		vector<shared_ptr<ChatClientLink>> TempList;

		// Collect active clients
		for (auto it = begin(m_ConnectedClients); it != end(m_ConnectedClients); it++)
		{
			if ((*it)->ElapsedActivity() < CLEANUP_INTERVAL_MS)
			{
				TempList.push_back(*it);
			}
		}

		// Clear active and inactive
		m_ConnectedClients.clear();

		// Add active back
		for (auto it = begin(TempList); it != end(TempList); it++)
		{
			m_ConnectedClients.push_back(*it);
		}
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
