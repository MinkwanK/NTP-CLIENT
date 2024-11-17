
// NTPClientDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "NTPClient.h"
#include "NTPClientDlg.h"
#include "afxdialogex.h"
#include <chrono>
#include <thread>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 1900년 1월 1일 이후의 초 시간 계산용
const uint64_t NTP_TIMESTAMP_DELTA = 2208988800ull;
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
}

BEGIN_MESSAGE_MAP(CNTPClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &CNTPClientDlg::OnBnClickedButtonStart)
	ON_WM_CLOSE()
	ON_MESSAGE(UM_RECV_PACKET, &CNTPClientDlg::OnRecvPacket)
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
	CString sStartIP(_T("216.239.35.0"));
	GetDlgItem(IDC_IPADDRESS1)->SetWindowText(sStartIP);
	CString sPort(_T("123"));
	GetDlgItem(IDC_EDIT)->SetWindowText(sPort);

	m_client.SetOwner(this);
	m_hClose = CreateEvent(NULL, FALSE, FALSE, NULL);
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
	CString sIP, sPort;
	GetDlgItem(IDC_IPADDRESS1)->GetWindowText(sIP);
	GetDlgItem(IDC_EDIT)->GetWindowText(sPort);
	char csIP[20];
	memset(csIP, 0x00, 20);
	memcpy(csIP, sIP, sIP.GetLength());

	int iPort = _ttoi(sPort);
	if (m_client.StartClient(csIP, iPort, 3))
	{
		m_list.AddString(_T("NTP 서버 접속 완료"));
		m_client.StartRecv();

		std::thread sendThread(NtpRequest, this);
		sendThread.detach();
	}
	else
	{
		if(m_client.GetConnect())
			m_list.AddString(_T("NTP 서버가 이미 접속 중입니다."));
		else
			m_list.AddString(_T("NTP 서버 접속 실패"));
	}
}


void CNTPClientDlg::OnClose()
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	SetEvent(m_client.m_hClose);
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

			packet[0] = 0x1B;
			auto now = std::chrono::system_clock::now().time_since_epoch(); //현재 시스템 시간을 UTC 이후 경과시간으로 반환
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now).count(); //경과시간 초단위 반환
			UINT64 ntpTime = seconds + NTP_TIMESTAMP_DELTA;

			// Transmit Timestamp (8바이트) , 보내는 시간
			packet[40] = (ntpTime >> 24) & 0xFF;
			packet[41] = (ntpTime >> 16) & 0xFF;
			packet[42] = (ntpTime >> 8) & 0xFF;
			packet[43] = ntpTime & 0xFF;

			int iResult = m_client.Send(packet);
			if (iResult <= 0)
			{
				m_list.AddString(_T("클라이언트 Send 실패\n"));
				OutputDebugString(_T("클라이언트 Send 실패\n"));
			}
			else
			{
				m_list.AddString(_T("클라이언트 Send 성공\n"));
				OutputDebugString(_T("클라이언트 Send 성공\n"));
			}


		}


	}
}

