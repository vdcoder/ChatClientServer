#pragma once
#include <asio.hpp>
#include <queue>

#define CONN_BUFFER_SIZE 100000

class IChatClientConnectionOutside
{
public:
	virtual void OnNewClients(std::vector<std::string> a_Clients) = 0;
	virtual void OnNewMessage(std::string a_sFromClientId, std::string a_Msg) = 0;
};

class ChatClientConnection
{
public:
	ChatClientConnection(IChatClientConnectionOutside * a_pOutside);
	virtual ~ChatClientConnection();

	void Connect(std::string a_sIpOrHostName, std::string a_sServiceOrPort);

	bool HasError();

	void RequestClientList();

	void SendChatMsg(std::string a_sToClientId, std::string a_Msg);

private:
	void IoContextRunThread();
	void PinPonThread();

	// Read
	void Recv();
	void ReadLengthHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred);
	void ReadDataHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred);

	// Process
	void ProcessRequest();

	// Write
	void AddToWrite(std::shared_ptr<std::vector<unsigned char>> a_pBuffer);
	void WriteNextPacket();
	void WriteDataHandler(const std::error_code& a_Error, std::size_t a_nBytesTransferred);

	IChatClientConnectionOutside *	m_pOutside;
	std::recursive_mutex			m_lock;
	bool							m_bError;
	asio::io_service 				m_IoService;
	asio::executor_work_guard<asio::io_context::executor_type>
									m_IoContextWorkGard;
	std::shared_ptr<asio::ip::tcp::socket>
									m_pSocket;
	unsigned int					m_nPacketSize;
	unsigned char					m_Buffer[CONN_BUFFER_SIZE];
	unsigned int					m_nWritePacketSize;
	unsigned char					m_WriteBuffer[CONN_BUFFER_SIZE];
	//std::string					m_sUserName;
	std::queue<std::shared_ptr<std::vector<unsigned char>>>
									m_PendingWriteQueue;
	time_t							m_TimeStamp;
	std::thread						m_IoContextRunThread;
	std::thread						m_PinPonThread;
	HANDLE							m_hTerminateEvent;
};