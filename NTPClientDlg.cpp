
// NTPClientDlg.cpp: 구현 파일

#include "pch.h"
#include "framework.h"
#include "NTPClient.h"
#include "NTPClientDlg.h"
#include "afxdialogex.h"
#include "CommonDefine.h"
#include <chrono>
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CNTPClientDlg 대화 상자



CNTPClientDlg::CNTPClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NTPCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CNTPClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list);
	DDX_Control(pDX, IDC_COMBO_NTP_SERVER, m_cmbServer);
	DDX_Control(pDX, IDC_IPADDRESS1, m_ipControl);
}

BEGIN_MESSAGE_MAP(CNTPClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &CNTPClientDlg::OnBnClickedButtonStart)
	ON_WM_CLOSE()
	ON_MESSAGE(UM_RECV_PACKET, &CNTPClientDlg::OnRecvPacket)
	ON_CBN_SELCHANGE(IDC_COMBO_NTP_SERVER, &CNTPClientDlg::OnCbnSelchangeComboNtpServer)
END_MESSAGE_MAP()


// CNTPClientDlg 메시지 처리기

BOOL CNTPClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	int iSel = m_cmbServer.AddString(_T("KR Pool (210.103.208.198)"));
	m_cmbServer.AddString(_T("한국 표준과학연구원 (KRISS) (203.248.240.140)"));
	m_cmbServer.AddString(_T("한국인터넷진흥원 (KISA) (168.126.1.44)"));
	m_cmbServer.AddString(_T("Google Public NTP (216.239.35.0)"));
	m_cmbServer.AddString(_T("직접 입력"));
	m_sConstIP[0] = _T("210.103.208.198");
	m_sConstIP[1] = _T("203.248.240.140");
	m_sConstIP[2] = _T("168.126.1.44");
	m_sConstIP[3] = _T("216.239.35.0");
	m_cmbServer.SetItemData(0, reinterpret_cast<DWORD_PTR>(&m_sConstIP[0]));
	m_cmbServer.SetItemData(1, reinterpret_cast<DWORD_PTR>(&m_sConstIP[1]));
	m_cmbServer.SetItemData(2, reinterpret_cast<DWORD_PTR>(&m_sConstIP[2]));
	m_cmbServer.SetItemData(3, reinterpret_cast<DWORD_PTR>(&m_sConstIP[3]));
	m_cmbServer.SetCurSel(0);
	CString sPort(_T("123"));
	GetDlgItem(IDC_EDIT)->SetWindowText(sPort);
	m_ipControl.EnableWindow(FALSE);
	m_client.SetOwner(this);
	m_hClose = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_client.SetCallback(CallbackClientConnect,CallbackClientSend);
	
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void CNTPClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CNTPClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CNTPClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CNTPClientDlg::OnBnClickedButtonStart()
{
	if (m_client.GetConnect())
	{
		//중지
		CString sListOut;
		sListOut.Format(_T("NTP 서버 접속 중지"));
		m_list.AddString(sListOut);
		GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("접속"));
	}
	else
	{
		CString sIP, sPort, sIPName;
		int iCurSel = m_cmbServer.GetCurSel();
		if (iCurSel == MAX_CONST_IP)	//직접 입력 모드
		{
			GetDlgItem(IDC_IPADDRESS1)->GetWindowText(sIP);
		}
		else
		{
			sIP = *(reinterpret_cast<CString*>(m_cmbServer.GetItemData(iCurSel)));
			m_cmbServer.GetLBText(iCurSel, sIPName);
		}

		GetDlgItem(IDC_EDIT)->GetWindowText(sPort);
		char csIP[20];
		memset(csIP, 0x00, 20);
		memcpy(csIP, sIP, sIP.GetLength());

		int iPort = _ttoi(sPort);
		CString sListOut;

		if (m_client.StartClient(csIP, iPort))
		{
			CString sListOut;
			sListOut.Format(_T("%s NTP 서버 접속 완료"), sIPName);
			m_list.AddString(sListOut);
			m_client.StartRecv();

			std::thread sendThread(NtpRequest, this);
			sendThread.detach();
			GetDlgItem(IDC_BUTTON_START)->SetWindowText(_T("중지"));
		}
		else
		{
			sListOut.Format(_T("%s NTP 서버 접속 실패"), sIPName);
			m_list.AddString(sListOut);
		}
	}
}


void CNTPClientDlg::OnClose()
{
	SetEvent(m_client.GetCloseHandle());
	SetEvent(m_hClose);
	CDialogEx::OnClose();
}

LRESULT CNTPClientDlg::OnRecvPacket(WPARAM wParam, LPARAM lParam)
{
	int iResult = (int)lParam;
	
	switch (iResult)
	{
	case NTP_SET_FAILED:
	{
		m_list.AddString(_T("클라이언트 Recv 성공 <시간동기화 실패>"));
	}break;
	case NTP_SET_SUCCESS:
	{
		m_list.AddString(_T("클라이언트 Recv 성공 <시간동기화 완료>"));
	}break;
	}
	return 0L;
}

void CNTPClientDlg::NtpRequest(CNTPClientDlg* pDlg)
{
	if (pDlg)
		pDlg->NtpRequestProc();
}

void CNTPClientDlg::NtpRequestProc()
{
	OutputDebugString(_T("NTP 클라이언트 Send 스레드 시작\n"));
	while (TRUE)
	{
		WaitForSingleObject(m_hClose, NTP_SEND_CYCLE);

		if (WAIT_OBJECT_0)
			return;
		
		if (m_client.GetConnect())
		{
			char packet[NTP_PACKET_SIZE];
			memset(packet, 0x00, NTP_PACKET_SIZE);

			/*
			NTP는 1900년 1월 1일부터 시작되는 시간을 기준으로 함.
			시스템 시간에는 1970년 1월 1일을 기준으로 함
			두 시스템간에는 70년의 시간 차이가 있음(2208988800초)
			*/
			packet[0] = 0x1B;
			auto now = std::chrono::system_clock::now().time_since_epoch(); //현재 시스템 시간을 UTC(epohc) 이후 경과시간으로 반환
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now).count(); //경과시간 초단위 반환
			UINT64 ntpTime = seconds + NTP_TIMESTAMP_DELTA;

			// Transmit Timestamp (8바이트) , 보내는 시간
			packet[40] = (ntpTime >> 24) & 0xFF;
			packet[41] = (ntpTime >> 16) & 0xFF;
			packet[42] = (ntpTime >> 8) & 0xFF;
			packet[43] = ntpTime & 0xFF;

			m_client.Send(packet);

		}
	}
}

void CNTPClientDlg::CallbackClientConnect(bool bResult)
{
	if (bResult == TRUE)
	{

	}
	else
	{

	}
}

void CNTPClientDlg::CallbackClientSend(bool bResult, int iSentSize, CString sIP)
{
	CString sMsg;

	if (bResult)
	{
		sMsg.Format(_T("[Send 성공] %d size send 성공 :: %s IP"), iSentSize, sIP);

	}
	else
	{
		sMsg.Format(_T("[Send 실패] %d size send 실패 :: %s IP"), iSentSize, sIP);
	}


	OutputDebugString(sMsg);
}

void CNTPClientDlg::OnCbnSelchangeComboNtpServer()
{
	int iSel = m_cmbServer.GetCurSel();
	if (iSel < MAX_CONST_IP)
	{
		CString sIP = *(CString*)m_cmbServer.GetItemData(iSel);
		OutputDebugString(sIP);
		m_ipControl.EnableWindow(FALSE);
	}
	else
	{
		m_ipControl.EnableWindow(TRUE);
	}
}
