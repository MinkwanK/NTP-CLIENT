#include "pch.h"
#include <WinSock2.h>
#include <WS2tcpip.h>  
#include <thread>
#include "Client.h"
#include <ctime>
#include "CommonDefine.h"

CClient::CClient()
{
	m_pOwner = nullptr;
	WSADATA wsaData;
	int iResult;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		WSACleanup();
		return;
	}

	m_hClose = CreateEvent(NULL, FALSE, FALSE, NULL);
}

CClient::~CClient()
{
	WSACleanup();

	if (m_hClose)
	{
		CloseHandle(m_hClose);
		m_hClose = nullptr;;
	}
}

//논블록 클라이언트 시작
bool CClient::StartClient(char* csIP, int iPort)
{
	if (m_bClientConnected)
	{
		OutputDebugString(_T("이미 접속 중입니다.\n"));
		return FALSE;
	}

	strncpy_s(m_csIP, sizeof(m_csIP), csIP, _TRUNCATE);
	m_iPort = iPort;
	
	memset(&m_ServerAddr, 0x00, sizeof(m_ServerAddr));

	m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_sock == INVALID_SOCKET)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		OutputDebugString(_T("소켓 생성 실패 \n"));
		return FALSE;
	}

	u_long mode = 1;
	if (ioctlsocket(m_sock, FIONBIO, &mode) != NO_ERROR)
	{
		OutputDebugString(_T("논블록 소켓 설정 실패\n"));
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return FALSE;
	}

	m_ServerAddr.sin_family = AF_INET;
	m_ServerAddr.sin_port = htons(iPort);
	inet_pton(AF_INET, m_csIP, &(m_ServerAddr.sin_addr.s_addr));

	CString sIP;
	sIP.Format(_T("%s \n"), m_csIP);

	while (TRUE)
	{
		WaitForSingleObject(m_hClose, 1000);

		if (WAIT_OBJECT_0)
			return FALSE;


		int iResult = connect(m_sock, (struct sockaddr*)&m_ServerAddr, sizeof(m_ServerAddr));
		if (iResult == SOCKET_ERROR)
		{
			int iError = WSAGetLastError();
			if (iError == WSAEWOULDBLOCK)
			{
				struct timeval timeout;
				fd_set fdSend;
				int	iRet;

				memset(&timeout, 0x00, sizeof(timeout));
				timeout.tv_sec = m_timeout.iConnect;
				timeout.tv_usec = 0;

				FD_ZERO(&fdSend);
				FD_SET(m_sock, &fdSend);

				//첫 번째는 항상 무시됨, 두번째부터 읽기 쓰기 예외
				iRet = select(0, (fd_set*)0, &fdSend, (fd_set*)0, &timeout);

				if (SOCKET_ERROR == iRet) //소켓 에러는 소켓 재설정을 위해 다시 시작
				{
					Reconnect();
					OutputDebugString(_T("클라이언트 접속 실패: 소켓 에러\n"));
					break;
				}
				if (!FD_ISSET(m_sock, &fdSend))
				{
					OutputDebugString(_T("클라이언트 접속 실패: FD_SET 쓰기 가능 상태 X\n"));
					continue;
				}
				m_bClientConnected = TRUE;
				OutputDebugString(_T("클라이언트 접속 성공\n"));
				return true;
			}
			else
			{
				OutputDebugString(_T("클라이언트 접속 실패: Connect 시도 실패\n"));
				Reconnect();
				break;
			}
		}
		else
		{
			m_bClientConnected = TRUE;
			OutputDebugString(_T("클라이언트 접속 성공\n"));
			return true;
		}
	}
}

void CClient::StartRecv()
{
	std::thread recv(RecvThread, this);
	recv.detach();
}

bool CClient::RecvThread(CClient* pClient)
{
	if (pClient)
		return pClient->RecvProc();
	return FALSE;
}

/*
수신 스레드
연결 끊김 및 오류 발생시 재접속 시도
*/
bool CClient::RecvProc()
{
	OutputDebugString(_T("클라이언트 Recv 스레드 시작\n"));
	struct timeval timeout;
	fd_set fdRecv;
	int	iRet;
	memset(&timeout, 0x00, sizeof(timeout));
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;
	FD_ZERO(&fdRecv);

	while (TRUE)
	{
		WaitForSingleObject(m_hClose, RECV_WAIT_TIME);

		if (WAIT_OBJECT_0)
			return FALSE;


		FD_SET(m_sock, &fdRecv);
		iRet = select(0, &fdRecv, (fd_set*)0, (fd_set*)0, &timeout);

		//소켓 에러
		if (SOCKET_ERROR == iRet)
			break;
		

		if (!FD_ISSET(m_sock, &fdRecv))
			continue;
		
		sockaddr_in	responseAddr;
		int	responseAddrLen = sizeof(responseAddr);
		iRet = recvfrom(m_sock, buf, NTP_PACKET_SIZE,0,(sockaddr*)&responseAddr,&responseAddrLen);
	
		if (iRet == NTP_PACKET_SIZE)
		{
			//NTP 타임스탬프는 64비트 값. 상위 32비트는 초, 하위 32비트는 소수부
			uint32_t originSecond, originFraction;
			originSecond = (buf[24] << 24 | (buf[24 + 1] << 16) | (buf[24 + 2] << 8) | buf[24+3]);
		}
	}

	if (iRet <= 0)
	{
		OutputDebugString(_T("클라이언트 접속 끊김\n"));
		Reconnect();
	}

	return 0;
}

bool CClient::Send(char* pBuf)
{
	int iResult = 0;
	if (pBuf)
	{
		iResult = sendto(m_sock, pBuf, NTP_PACKET_SIZE, 0, (sockaddr*)&m_ServerAddr, sizeof(m_ServerAddr));
		ntp_rtt[0] = std::chrono::system_clock::now();	//송신 시간 기록
	}
	return iResult;
}

void CClient::Disconnect()
{
	closesocket(m_sock);
	m_sock = INVALID_SOCKET;
	m_bClientConnected = FALSE;
}

void CClient::Reconnect()
{
	Disconnect();
	StartClient(m_csIP, m_iPort);
}

void CClient::UnixTimeToSystemTime(time_t t, SYSTEMTIME* st)
{
	struct tm tm;
	memset(&tm, 0x00, sizeof(tm));
	gmtime_s(&tm,&t); //Unix 시간 struct tm 변환

	if (st != nullptr)
	{
		st->wYear = tm.tm_year + 1900;
		st->wMonth = tm.tm_mon + 1;
		st->wDay = tm.tm_mday;
		st->wHour = tm.tm_hour;
		st->wMinute = tm.tm_min;
		st->wSecond = tm.tm_sec;
		st->wMilliseconds = 0;
	}
}

void CClient::SetTimeout(int iConnect, int iRecv, int iSend)
{
	m_timeout.iConnect = iConnect;
	m_timeout.iRecv = iRecv;
	m_timeout.iSend = iSend;
}

HANDLE CClient::GetCloseHandle()
{
	return m_hClose;
}

void CClient::SetCloseHandle()
{
	SetEvent(m_hClose);
}

void CClient::SetCallback(Callback cbConnect)
{
	m_cbConnect = cbConnect;
}
