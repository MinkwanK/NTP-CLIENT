
// NTPClientDlg.h: 헤더 파일
//
#include "Client.h"
#pragma once

#define UM_RECV_PACKET		(WM_USER + 3000)
#define NTP_SEND_CYCLE		5000
#define NTP_PACKET_SIZE		48
// CNTPClientDlg 대화 상자
class CNTPClientDlg : public CDialogEx
{
// 생성입니다.
public:
	CNTPClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	
public:
	CClient m_client;
	HANDLE m_hClose;
// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NTPCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnClose();
	afx_msg LRESULT OnRecvPacket(WPARAM wParam, LPARAM lParam);

	static void NtpRequest(CNTPClientDlg* pDlg);
	void	NtpRequestProc();
	CListBox m_list;
};
