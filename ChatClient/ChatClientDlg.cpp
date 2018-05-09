
// ChatClientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ChatClient.h"
#include "ChatClientDlg.h"
#include "ChatClientWindowDlg.h"
#include "afxdialogex.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

using namespace std;

#define WM_NEWCLIENTS	WM_USER + 1
#define WM_NEWMESSAGE	WM_USER + 2


//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// CChatClientDlg dialog

CChatClientDlg::CChatClientDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CHATCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void CChatClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_IPORHOSTNAMEEDIT, m_IpOrHostNameEdit);
	DDX_Control(pDX, IDC_PORTEDIT, m_PortEdit);
	DDX_Control(pDX, IDC_CLIENTLIST, m_ClientsListBox);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

BEGIN_MESSAGE_MAP(CChatClientDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_CONNECT_BTN, &CChatClientDlg::OnBnClickedConnectBtn)
	ON_BN_CLICKED(IDC_DISCONNECT_BTN, &CChatClientDlg::OnBnClickedDisconnectBtn)
	ON_BN_CLICKED(IDC_REFRESH_BTN, &CChatClientDlg::OnBnClickedRefreshBtn)
	ON_LBN_DBLCLK(IDC_CLIENTLIST, &CChatClientDlg::OnLbnDblclkClientlist)

	ON_MESSAGE(WM_NEWCLIENTS, OnNewClientsOnMainThread)
	ON_MESSAGE(WM_NEWMESSAGE, OnNewMessageOnMainThread)
END_MESSAGE_MAP()

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// CChatClientDlg message handlers

BOOL CChatClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_IpOrHostNameEdit.SetWindowTextW(L"localhost");
	m_PortEdit.SetWindowTextW(L"1234");

	return TRUE;  // return TRUE  unless you set the focus to a control
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CChatClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CChatClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void CChatClientDlg::OnBnClickedConnectBtn()
{
	if (m_pClientConn == nullptr)
	{
		CString sIpOrHostName;
		m_IpOrHostNameEdit.GetWindowTextW(sIpOrHostName);
		CT2A sIpOrHostNameA(sIpOrHostName);

		CString sServiceOrPort;
		m_PortEdit.GetWindowTextW(sServiceOrPort);
		CT2A sServiceOrPortA(sServiceOrPort);

		m_pClientConn = make_shared<ChatClientConnection>(this);
		m_pClientConn->Connect(sIpOrHostNameA.m_psz, sServiceOrPortA.m_psz);

		if (!m_pClientConn->HasError())
		{
			m_pClientConn->RequestClientList();
		}
		else
		{
			MessageBox(
				L"Server Not found",
				L"Error",
				MB_ICONERROR
			);

			m_pClientConn == nullptr;
		}
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void CChatClientDlg::OnBnClickedDisconnectBtn()
{
	if (m_pClientConn != nullptr && !m_pClientConn->HasError())
	{
		MessageBox(
			L"Disconnected",
			L"Status",
			MB_OK
		);

		m_pClientConn = nullptr;
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void CChatClientDlg::OnBnClickedRefreshBtn()
{
	if (m_pClientConn != nullptr && !m_pClientConn->HasError())
	{
		m_pClientConn->RequestClientList();
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void CChatClientDlg::OnNewClients(std::vector<std::string> a_Clients)
{
	std::vector<std::string> * pClients = new std::vector<std::string>();
	for (auto item : a_Clients)
		pClients->push_back(item);

	PostMessage(WM_NEWCLIENTS, 0, (LPARAM)pClients);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

LRESULT CChatClientDlg::OnNewClientsOnMainThread(WPARAM wParam, LPARAM lParam)
{
	std::vector<std::string> * pClients = (std::vector<std::string> *)lParam;

	// Replace clients in UI
	m_ClientsListBox.ResetContent();
	for (auto s : *pClients)
	{
		CA2W unicodeStr(s.c_str());
		m_ClientsListBox.AddString(unicodeStr.m_psz);
	}

	delete pClients;
	return 0;
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void CChatClientDlg::OnNewMessage(std::string a_sFromClientId, std::string a_Msg)
{
	std::vector<std::string> * pParams = new std::vector<std::string>();
	pParams->push_back(a_sFromClientId);
	pParams->push_back(a_Msg);

	PostMessage(WM_NEWMESSAGE, 0, (LPARAM)pParams);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

LRESULT CChatClientDlg::OnNewMessageOnMainThread(WPARAM wParam, LPARAM lParam)
{
	std::vector<std::string> * pParams = (std::vector<std::string> *)lParam;
	string sFromClientId = pParams->at(0);
	string sMsg = pParams->at(1);

	auto it = m_ClientChatWindows.find(sFromClientId);
	if (it == m_ClientChatWindows.end())
	{
		// Create chat window
		ChatClientWindowDlg * pChatWindow = new	ChatClientWindowDlg(this, sFromClientId);
		pChatWindow->Create(IDD_CHATWINDOW_DIALOG);

		// Add to map
		m_ClientChatWindows[sFromClientId] = pChatWindow;
	}

	// Put msg in chat window
	if (sMsg != "")
	{
		m_ClientChatWindows[sFromClientId]->PutMsg(sMsg);
	}
	m_ClientChatWindows[sFromClientId]->ShowWindow(SW_SHOW);

	delete pParams;
	return 0;
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void CChatClientDlg::CmdSendMsg(std::string a_sToClientId, std::string a_Msg)
{
	if (m_pClientConn != nullptr && !m_pClientConn->HasError())
	{
		m_pClientConn->SendChatMsg(a_sToClientId, a_Msg);
	}
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^


void CChatClientDlg::OnLbnDblclkClientlist()
{
	int nSel = m_ClientsListBox.GetCurSel();
	if (nSel != LB_ERR)
	{
		CString ItemSelected;
		m_ClientsListBox.GetText(nSel, ItemSelected);
		CW2A str(ItemSelected);

		OnNewMessage(str.m_psz, "");
	}
}
