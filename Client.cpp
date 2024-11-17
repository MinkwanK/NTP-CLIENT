#include "pch.h"
#include <WinSock2.h>
#include <WS2tcpip.h>  
#include <thread>
#include "Client.h"
#include <ctime>

// 1900년 1월 1일 이후의 초 시간 계산용
const uint64_t NTP_TIMESTAMP_DELTA = 2208988800ull;

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
}

//논 블록 클라이언트 시작
BOOL CClient::StartClient(char* csIP, int iPort, int iTimeout)
{
	if (m_bConnected)
		return FALSE;

	strncpy_s(m_csIP, sizeof(m_csIP), csIP, _TRUNCATE);
	m_iPort = iPort;
	m_iTimeout = iTimeout;

	memset(&m_ServerAddr, 0, sizeof(m_ServerAddr));

	m_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_sock == INVALID_SOCKET)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return FALSE;
	}

	u_long mode = 1;	
	if (ioctlsocket(m_sock, FIONBIO, &mode) != NO_ERROR)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return FALSE;
	}
	m_ServerAddr.sin_family = AF_INET;
	m_ServerAddr.sin_port = htons(iPort);
	inet_pton(AF_INET, m_csIP, &(m_ServerAddr.sin_addr.s_addr));
	CString sIP;
	sIP.Format(_T("%s \n"), m_csIP);
	OutputDebugString(sIP);
	while (TRUE)
	{
		WaitForSingleObject(m_hClose, iTimeout *1000);

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
				timeout.tv_sec = iTimeout;
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
				m_bConnected = TRUE;
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
			m_bConnected = TRUE;
			OutputDebugString(_T("클라이언트 접속 성공\n"));
			return true;
		}
	}
}

BOOL CClient::StartClient(char* csIP, int iPort)
{
	return 0;
}

void CClient::StartRecv()
{
	std::thread recv(RecvThread, this);
	recv.detach();
}

BOOL CClient::RecvThread(CClient* pClient)
{
	if (pClient)
		return pClient->RecvProc();
	return FALSE;
}


/*
수신 스레드
연결 끊김 및 오류 발생시 재접속 시도
*/
BOOL CClient::RecvProc()
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
			//ntp_packet ntp = { 0 };
			//memcpy(&ntp, buf, NTP_PACKET_SIZE);

			uint32_t originSecond, originFraction;
			originSecond = (buf[24] << 24 | (buf[24 + 1] << 16) | (buf[24 + 2] << 8) | buf[24+3]);

			//NTP 타임스탬프는 64비트 값. 상위 32비트는 초, 하위 32비트는 sosubu
			//uint32_t secondsSince1900 = (buf[43] & 0xFF) | (buf[42] & 0xFF) << 8 | (buf[41] & 0xFF) << 16 | (buf[40] & 0xFF) << 24;
			//time_t  timestamp = secondsSince1900 - NTP_TIMESTAMP_DELTA;	//NTP 시간 -> unix 시간 변환

			//std::chrono::system_clock::time_point server_time = std::chrono::system_clock::from_time_t(timestamp);
			//server_time += one_way_delay;  // 편도 지연 시간 적용

			//auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(one_way_delay).count(); //밀리초 변환

			//CString strDelay;
			//strDelay.Format(_T("지연 시간: %lld 밀리초\n"), milliseconds);  // %lld는 long long 정수형 포맷
			//OutputDebugString(strDelay);

			//timestamp = std::chrono::system_clock::to_time_t(server_time);
			//SYSTEMTIME st;
			//UnixTimeToSystemTime(timestamp, &st);

			//// 10. 시스템 시간 설정 (관리자 권한 필요)
			//if (!SetSystemTime(&st)) 
			//{
			//	DWORD error = GetLastError();
			//	CString sError;
			//	sError.Format(_T("시스템 에러: %d"), error);
			//	OutputDebugString(sError);
			//	m_pOwner->PostMessage(UM_RECV_PACKET, NULL, (LPARAM)NTP_SET_FAILED);
			//}
			//else {
			//	//로직 수행
			//	m_pOwner->PostMessage(UM_RECV_PACKET, NULL, (LPARAM)NTP_SET_SUCCESS);
			//	OutputDebugString(_T("시스템 타임 설정 성공\n"));
			//}

		}
	}

	if (iRet <= 0)
	{
		OutputDebugString(_T("클라이언트 접속 끊김\n"));
		Reconnect();
	}

	return 0;
}

BOOL CClient::Send(char* pBuf)
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
	m_bConnected = FALSE;
}

void CClient::Reconnect()
{
	Disconnect();
	StartClient(m_csIP, m_iPort, m_iTimeout);
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