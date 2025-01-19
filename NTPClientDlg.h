
// NTPClientDlg.h: 헤더 파일
//
#include "Client.h"
#pragma once


#define MAX_CONST_IP		4

// CNTPClientDlg 대화 상자
class CNTPClientDlg : public CDialogEx
{
// 생성입니다.
public:
	CNTPClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	
public:
	CClient m_client;
	HANDLE m_hClose;
	CListBox m_list;
	CComboBox m_cmbServer;
	CString m_sConstIP[MAX_CONST_IP];
	CIPAddressCtrl m_ipControl;

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
	afx_msg void OnCbnSelchangeComboNtpServer();
	static void NtpRequest(CNTPClientDlg* pDlg);
	void NtpRequestProc();


	//멤버 함수는 특정 객체 에속한다. 호출시 암묵적으로 &객체 멤버변수가 들어감
	//static을 붙이면 객체에 종속되지 않아서 일반 함수처럼 주소를 할당 가능
	static void CallbackClientConnect(bool bResult);

};
