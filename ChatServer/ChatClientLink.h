#pragma once
#include <asio.hpp>
#include <queue>

#define CONN_BUFFER_SIZE 100000

class IChatClientLinkOutside
{
public:
	virtual std::string DescribeConnectedClients() = 0;
	virtual void MessageConnectedClient(std::string a_sClientId, std::shared_ptr<std::vector<unsigned char>> a_pBuffer) = 0;
};

class ChatClientLink
{
public:
	ChatClientLink(IChatClientLinkOutside * a_pOutside, asio::io_service & a_IoService, std::shared_ptr<asio::ip::tcp::socket> a_pNewSocket);

	std::string ClientId();
	std::string Describe();
	int	ElapsedActivity();

	bool HasError();

	void AddToWrite(std::shared_ptr<std::vector<unsigned char>> a_pBuffer);

private:
	// Read
	void Recv();
	void ReadLengthHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred);
	void ReadDataHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred);

	// Process
	void ProcessRequest();

	// Write
	void WriteNextPacket();
	void WriteDataHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred);

	IChatClientLinkOutside *		m_pOutside;
	std::recursive_mutex			m_lock;
	bool							m_bError;
	asio::io_service &				m_IoService;
	std::shared_ptr<asio::ip::tcp::socket>	
									m_pSocket;
	unsigned int					m_nPacketSize;
	unsigned char					m_Buffer[CONN_BUFFER_SIZE];
	unsigned int					m_nWritePacketSize;
	unsigned char					m_WriteBuffer[CONN_BUFFER_SIZE];
	std::string						m_sUserName;
	std::string						m_sClientIp;
	unsigned short					m_nClientPort;
	std::queue<std::shared_ptr<std::vector<unsigned char>>>
									m_PendingWriteQueue;
	time_t							m_TimeStamp;
};