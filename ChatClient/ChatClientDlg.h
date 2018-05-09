
// ChatClientDlg.h : header file
//

#pragma once
#include "ChatClientConnection.h"
#include "ChatClientWindowDlg.h"
#include <unordered_map>



// CChatClientDlg dialog
class CChatClientDlg :	public CDialogEx, 
						public IChatClientConnectionOutside, 
						public IChatClientWindowDlgOutside
{
// Construction
public:
	CChatClientDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedConnectBtn();
	afx_msg void OnBnClickedDisconnectBtn();
	afx_msg void OnBnClickedRefreshBtn();

	afx_msg LRESULT OnNewClientsOnMainThread(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNewMessageOnMainThread(WPARAM wParam, LPARAM lParam);

public:
	// IChatClientConnectionOutside
	virtual void OnNewClients(std::vector<std::string> a_Clients);
	virtual void OnNewMessage(std::string a_sFromClientId, std::string a_Msg);

	// IChatClientWindowDlgOutside
	virtual void CmdSendMsg(std::string a_sToClientId, std::string a_Msg);

private:
	std::shared_ptr<ChatClientConnection>	m_pClientConn;

	CEdit									m_IpOrHostNameEdit;
	CEdit									m_PortEdit;
	CListBox								m_ClientsListBox;
	std::unordered_map<std::string, ChatClientWindowDlg *>
											m_ClientChatWindows;
public:
	afx_msg void OnLbnDblclkClientlist();
};
