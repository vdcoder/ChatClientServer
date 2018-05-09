#include "stdafx.h"
#include "ChatClientConnection.h"
#include <string>
#include <sstream>
#include <vector>

using namespace std;
using asio::ip::tcp;

#define PINPON_INTERVAL_MS 60000 // 1 minute

#define CHAT_CMD_INITIATION 0
#define CHAT_CMD_PINPON		1
#define CHAT_CMD_CLIENTS	2
#define CHAT_CMD_MSG		3

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ChatClientConnection::ChatClientConnection(
	IChatClientConnectionOutside *	a_pOutside)
	: m_pOutside(a_pOutside)
	, m_bError(true)
	, m_IoContextWorkGard( asio::make_work_guard(m_IoService) ) // Avoid io_context running out of work
{
	m_hTerminateEvent = CreateEvent(0, TRUE, FALSE, NULL);

	m_IoContextRunThread = thread(&ChatClientConnection::IoContextRunThread, this);

	m_PinPonThread = std::thread(&ChatClientConnection::PinPonThread, this);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ChatClientConnection::~ChatClientConnection()
{
	SetEvent(m_hTerminateEvent);

	m_PinPonThread.join();

	m_IoContextWorkGard.reset();
	m_IoContextRunThread.join();

	CloseHandle(m_hTerminateEvent);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::Connect(std::string a_sIpOrHostName, std::string a_sServiceOrPort)
{
	m_bError = false;

	try
	{
		tcp::resolver resolver(m_IoService);
		tcp::resolver::query query(a_sIpOrHostName, a_sServiceOrPort);
		tcp::resolver::iterator iterator = resolver.resolve(query);

		m_pSocket = make_shared<tcp::socket>(m_IoService);
		asio::connect(*m_pSocket.get(), iterator);

		Recv();

		// Send init

		// get username
		WCHAR sUnBuffer[UNLEN + 1];
		DWORD nUnSize = UNLEN + 1;
		GetUserName(sUnBuffer, &nUnSize);
		CW2A s(sUnBuffer);
		string sUserName(s.m_psz);

		std::shared_ptr<std::vector<unsigned char>> pBuffer = make_shared<std::vector<unsigned char>>();
		pBuffer->push_back(CHAT_CMD_INITIATION);
		for (auto ch : sUserName)
			pBuffer->push_back(ch);
		pBuffer->push_back(0);

		AddToWrite(pBuffer);
	}
	catch (...)
	{
		m_pSocket = nullptr;
		m_bError = true;
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

bool ChatClientConnection::HasError()
{
	return m_bError;
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::RequestClientList()
{
	if (!m_bError)
	{
		std::shared_ptr<std::vector<unsigned char>> pBuffer = make_shared<std::vector<unsigned char>>();
		pBuffer->push_back(CHAT_CMD_CLIENTS);

		AddToWrite(pBuffer);
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::SendChatMsg(std::string a_sToClientId, std::string a_Msg)
{
	if (!m_bError)
	{
		std::shared_ptr<std::vector<unsigned char>> pBuffer = make_shared<std::vector<unsigned char>>();
		pBuffer->push_back(CHAT_CMD_MSG);
		for (auto ch : a_sToClientId)
			pBuffer->push_back(ch);
		pBuffer->push_back(0);
		for (auto ch : a_Msg)
			pBuffer->push_back(ch);
		pBuffer->push_back(0);

		AddToWrite(pBuffer);
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::IoContextRunThread()
{
	for (;;)
	{
		try
		{
			m_IoService.run();
			break; // all done
		}
		catch (...)
		{
		}
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::PinPonThread()
{
	while (WAIT_TIMEOUT == WaitForSingleObject(m_hTerminateEvent, PINPON_INTERVAL_MS))
	{
		lock_guard<recursive_mutex> LockGuard(m_lock);

		if (!m_bError)
		{
			// send pin
			std::shared_ptr<std::vector<unsigned char>> pBuffer = make_shared<std::vector<unsigned char>>();
			pBuffer->push_back(CHAT_CMD_PINPON);

			//AddToWrite(pBuffer);
		}
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::Recv()
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	asio::async_read(
		*m_pSocket,
		asio::buffer((void *)&m_nPacketSize, sizeof(unsigned int)),
		std::bind(
			&ChatClientConnection::ReadLengthHandler,
			this,
			std::placeholders::_1,
			std::placeholders::_2));
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::ReadLengthHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred)
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	if (a_Error ||
		a_nBytesTransferred != sizeof(unsigned int) ||
		m_nPacketSize > CONN_BUFFER_SIZE)
	{
		m_pSocket == nullptr;
		m_bError = true;
	}
	else
	{
		asio::async_read(
			*m_pSocket,
			asio::buffer((void *)&m_Buffer, m_nPacketSize),
			std::bind(
				&ChatClientConnection::ReadDataHandler,
				this,
				std::placeholders::_1,
				std::placeholders::_2));
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::ReadDataHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred)
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	if (a_Error ||
		a_nBytesTransferred != m_nPacketSize)
	{
		m_pSocket == nullptr;
		m_bError = true;
	}
	else
	{
		ProcessRequest();

		Recv();
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::ProcessRequest()
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	switch (m_Buffer[0])
	{
		case CHAT_CMD_PINPON:
		{
			// got pon
			break;
		}
		case CHAT_CMD_CLIENTS:
		{
			// got clients
			std::string sCommaSeparatedClients = (char *)&m_Buffer[1];

			// split
			std::vector<std::string> Clients;
			std::stringstream ss(sCommaSeparatedClients);
			std::string item;
			while (std::getline(ss, item, ',')) 
			{
				Clients.push_back(std::move(item)); // C++11 
			}

			m_pOutside->OnNewClients(Clients);
			break;
		}
		case CHAT_CMD_MSG:
		{
			// got msg
			std::string sFromClientId = (char *)&m_Buffer[1];
			std::string sMsg = (char *)&m_Buffer[2 + sFromClientId.length()];
			m_pOutside->OnNewMessage(sFromClientId, sMsg);
			break;
		}
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::AddToWrite(std::shared_ptr<std::vector<unsigned char>> a_pBuffer)
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	// Queue write job
	m_PendingWriteQueue.push(a_pBuffer);

	// If only one, then start writing
	if (m_PendingWriteQueue.size() == 1)
	{
		WriteNextPacket();
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::WriteNextPacket()
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	// Get next packet to send
	shared_ptr<vector<unsigned char>> pBuffer = m_PendingWriteQueue.front();
	m_PendingWriteQueue.pop();

	// size
	unsigned int nSize = pBuffer->size();
	m_nWritePacketSize = nSize + sizeof(unsigned int);

	// does it fit?
	if (m_nWritePacketSize <= CONN_BUFFER_SIZE)
	{
		// Copy to send buffer
		memcpy(m_WriteBuffer, &nSize, sizeof(unsigned int));
		memcpy(&m_WriteBuffer[sizeof(unsigned int)], &pBuffer->at(0), nSize);

		// Send async
		asio::async_write(
			*m_pSocket,
			asio::buffer((void *)&m_WriteBuffer, m_nWritePacketSize),
			std::bind(
				&ChatClientConnection::WriteDataHandler,
				this,
				std::placeholders::_1,
				std::placeholders::_2));

		// Timestamp send
		m_TimeStamp = time(nullptr);
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientConnection::WriteDataHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred)
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	// Validate
	if (a_Error ||
		a_nBytesTransferred != m_nWritePacketSize)
	{
		m_pSocket == nullptr;
		m_bError = true;
	}
	else
	{
		// If more pending jobs then keep writing
		if (m_PendingWriteQueue.size() > 0)
		{
			WriteNextPacket();
		}
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
