#pragma once

#include <string>


class IChatClientWindowDlgOutside
{
public:
	virtual void CmdSendMsg(std::string a_sToClientId, std::string a_Msg) = 0;
};


// ChatClientWindowDlg dialog

class ChatClientWindowDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ChatClientWindowDlg)

public:
	ChatClientWindowDlg(IChatClientWindowDlgOutside * a_pOutside, std::string a_sRemoteClientId, CWnd* pParent = NULL);   // standard constructor
	virtual ~ChatClientWindowDlg();

	void PutMsg(std::string a_sMsg);

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATWINDOW_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL	OnInitDialog();
	void			OnClose();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedSendBtn();

private:
	IChatClientWindowDlgOutside *	m_pOutside;
	CEdit							m_ChatEdit;
	CEdit							m_LineEdit;
	std::string						m_sRemoteClientId;
};
