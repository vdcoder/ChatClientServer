#pragma once
#include <asio.hpp>
#include "ChatClientLink.h"

class ChatServerEngine: public IChatClientLinkOutside
{
public:
	ChatServerEngine(asio::io_service & a_IoService, unsigned short a_nPort);
	virtual ~ChatServerEngine();

	bool HasError();

	// IChatClientConnectionOutside
	virtual std::string DescribeConnectedClients();
	virtual void MessageConnectedClient(std::string a_sClientId, std::shared_ptr<std::vector<unsigned char>> a_pBuffer);

private:
	void Listen();

	void AcceptHandler(const std::error_code& a_Error);

	void CleanUpWorkerThread();

	std::recursive_mutex		m_lock;
	bool						m_bError;
	asio::io_service &			m_IoService;
	asio::ip::tcp::acceptor		m_Acceptor;
	std::shared_ptr<asio::ip::tcp::socket>		
								m_pAcceptSocket;
	std::list<std::shared_ptr<ChatClientLink>>
								m_ConnectedClients;
	std::thread					m_CleanUpWorkerThread;
	HANDLE						m_hTerminateEvent;
};