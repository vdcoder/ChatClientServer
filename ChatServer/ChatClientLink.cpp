#include "stdafx.h"
#include "ChatClientLink.h"

using namespace std;
using asio::ip::tcp;

#define CHAT_CMD_INITIATION 0
#define CHAT_CMD_PINPON		1
#define CHAT_CMD_CLIENTS	2
#define CHAT_CMD_MSG		3

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ChatClientLink::ChatClientLink(
	IChatClientLinkOutside *		a_pOutside,
	asio::io_service &				a_IoService, 
	std::shared_ptr<tcp::socket>	a_pNewSocket)
	: m_pOutside(a_pOutside)
	, m_IoService(a_IoService)
	, m_pSocket(a_pNewSocket)
{
	Recv();
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

std::string ChatClientLink::ClientId()
{
	return Describe();
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

std::string ChatClientLink::Describe()
{
	return m_sUserName + " (@ " + m_sClientIp + " : " + to_string(m_nClientPort) + " )";
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

int	ChatClientLink::ElapsedActivity()
{
	return ((long)time(nullptr) - (long)m_TimeStamp) * 1000;
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

bool ChatClientLink::HasError()
{
	return m_bError;
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientLink::Recv()
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	asio::async_read(
		*m_pSocket, 
		asio::buffer((void *)&m_nPacketSize, sizeof(unsigned int)),
		std::bind(
			&ChatClientLink::ReadLengthHandler,
			this, 
			std::placeholders::_1,
			std::placeholders::_2));
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientLink::ReadLengthHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred)
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
				&ChatClientLink::ReadDataHandler,
				this,
				std::placeholders::_1,
				std::placeholders::_2));
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientLink::ReadDataHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred)
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

void ChatClientLink::ProcessRequest()
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	switch (m_Buffer[0])
	{
		case CHAT_CMD_INITIATION:
		{
			m_sUserName = (char *)&m_Buffer[1];
			m_sClientIp = m_pSocket->remote_endpoint().address().to_string();
			m_nClientPort = m_pSocket->remote_endpoint().port();

			break;
		}
		case CHAT_CMD_PINPON:
		{
			// send pon
			std::shared_ptr<std::vector<unsigned char>> pBuffer = make_shared<std::vector<unsigned char>>();
			pBuffer->push_back(CHAT_CMD_PINPON);

			AddToWrite(pBuffer);
			break;
		}
		case CHAT_CMD_CLIENTS:
		{
			// send clients
			string sClients = m_pOutside->DescribeConnectedClients();

			std::shared_ptr<std::vector<unsigned char>> pBuffer = make_shared<std::vector<unsigned char>>();
			pBuffer->push_back(CHAT_CMD_CLIENTS);
			for (unsigned int i = 0; i < sClients.length(); i++)
				pBuffer->push_back(sClients[i]);
			pBuffer->push_back(0);

			AddToWrite(pBuffer);
			break;
		}
		case CHAT_CMD_MSG:
		{
			std::string sClientId = (char *)&m_Buffer[1];
			std::string sMsg = (char *)&m_Buffer[2 + sClientId.length()];

			std::shared_ptr<std::vector<unsigned char>> pBuffer = make_shared<std::vector<unsigned char>>();
			std::string sMyClientId = ClientId();
			pBuffer->push_back(CHAT_CMD_MSG);
			for (unsigned int i = 0; i < sMyClientId.length(); i++)
				pBuffer->push_back(sMyClientId[i]);
			pBuffer->push_back(0);
			for (unsigned int i = 0; i < sMsg.length(); i++)
				pBuffer->push_back(sMsg[i]);
			pBuffer->push_back(0);

			// send msg to client
			m_pOutside->MessageConnectedClient(sClientId, pBuffer);
			break;
		}
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientLink::AddToWrite(std::shared_ptr<std::vector<unsigned char>> a_pBuffer)
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

void ChatClientLink::WriteNextPacket()
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
				&ChatClientLink::WriteDataHandler,
				this,
				std::placeholders::_1,
				std::placeholders::_2));

		// Timestamp send
		m_TimeStamp = time(nullptr);
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientLink::WriteDataHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred)
{
	lock_guard<recursive_mutex> LockGuard(m_lock);

	// Balidate
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
