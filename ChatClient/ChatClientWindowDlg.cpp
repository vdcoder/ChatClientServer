// ChatClientWindowDlg.cpp : implementation file
//

#include "stdafx.h"
#include "ChatClient.h"
#include "ChatClientWindowDlg.h"
#include "afxdialogex.h"

using namespace std;

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// ChatClientWindowDlg dialog

IMPLEMENT_DYNAMIC(ChatClientWindowDlg, CDialogEx)

ChatClientWindowDlg::ChatClientWindowDlg(IChatClientWindowDlgOutside * a_pOutside, std::string a_sRemoteClientId, CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_CHATWINDOW_DIALOG, pParent)
	, m_pOutside(a_pOutside)
	, m_sRemoteClientId(a_sRemoteClientId)
{
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ChatClientWindowDlg::~ChatClientWindowDlg()
{
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientWindowDlg::PutMsg(std::string a_sMsg)
{
	// get the initial text length
	int nLength = m_ChatEdit.GetWindowTextLength();
	
	// put the selection at the end of text
	m_ChatEdit.SetSel(nLength, nLength);
	
	// add CR/LF to text
	CA2W unicodeStr(a_sMsg.c_str());
	CString strLine;
	strLine.Format(_T("\r\n%s"), unicodeStr.m_psz);

	// replace the selection
	m_ChatEdit.ReplaceSel(strLine);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientWindowDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_CHAT_EDIT, m_ChatEdit);
	DDX_Control(pDX, IDC_LINE_EDIT, m_LineEdit);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

BEGIN_MESSAGE_MAP(ChatClientWindowDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_SEND_BTN, &ChatClientWindowDlg::OnBnClickedSendBtn)
END_MESSAGE_MAP()

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// CChatClientDlg message handlers

BOOL ChatClientWindowDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set title
	CA2W str(m_sRemoteClientId.c_str());
	SetWindowTextW(str.m_psz);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

void ChatClientWindowDlg::OnClose()
{
	ShowWindow(SW_HIDE);
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// ChatClientWindowDlg message handlers

void ChatClientWindowDlg::OnBnClickedSendBtn()
{
	// Send text
	CString sLineText;
	m_LineEdit.GetWindowText(sLineText);
	if (sLineText.Trim() != L"")
	{
		CW2A str(sLineText);
		m_pOutside->CmdSendMsg(m_sRemoteClientId, str.m_psz);

		// Put msg
		PutMsg("ME>> " + string(str.m_psz));
	}
	
	// Clear edit line
	m_LineEdit.SetWindowText(L"");
}

//^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
