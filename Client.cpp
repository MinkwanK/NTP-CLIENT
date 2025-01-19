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

//���� Ŭ���̾�Ʈ ����
bool CClient::StartClient(char* csIP, int iPort)
{
	if (m_bClientConnected)
	{
		OutputDebugString(_T("�̹� ���� ���Դϴ�.\n"));
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
		OutputDebugString(_T("���� ���� ���� \n"));
		return FALSE;
	}

	u_long mode = 1;
	if (ioctlsocket(m_sock, FIONBIO, &mode) != NO_ERROR)
	{
		OutputDebugString(_T("���� ���� ���� ����\n"));
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

				//ù ��°�� �׻� ���õ�, �ι�°���� �б� ���� ����
				iRet = select(0, (fd_set*)0, &fdSend, (fd_set*)0, &timeout);

				if (SOCKET_ERROR == iRet) //���� ������ ���� �缳���� ���� �ٽ� ����
				{
					Reconnect();
					OutputDebugString(_T("Ŭ���̾�Ʈ ���� ����: ���� ����\n"));
					break;
				}
				if (!FD_ISSET(m_sock, &fdSend))
				{
					OutputDebugString(_T("Ŭ���̾�Ʈ ���� ����: FD_SET ���� ���� ���� X\n"));
					continue;
				}
				m_bClientConnected = TRUE;
				OutputDebugString(_T("Ŭ���̾�Ʈ ���� ����\n"));
				return true;
			}
			else
			{
				OutputDebugString(_T("Ŭ���̾�Ʈ ���� ����: Connect �õ� ����\n"));
				Reconnect();
				break;
			}
		}
		else
		{
			m_bClientConnected = TRUE;
			OutputDebugString(_T("Ŭ���̾�Ʈ ���� ����\n"));
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
���� ������
���� ���� �� ���� �߻��� ������ �õ�
*/
bool CClient::RecvProc()
{
	OutputDebugString(_T("Ŭ���̾�Ʈ Recv ������ ����\n"));
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

		//���� ����
		if (SOCKET_ERROR == iRet)
			break;
		

		if (!FD_ISSET(m_sock, &fdRecv))
			continue;
		
		sockaddr_in	responseAddr;
		int	responseAddrLen = sizeof(responseAddr);
		iRet = recvfrom(m_sock, buf, NTP_PACKET_SIZE,0,(sockaddr*)&responseAddr,&responseAddrLen);
	
		if (iRet == NTP_PACKET_SIZE)
		{
			//NTP Ÿ�ӽ������� 64��Ʈ ��. ���� 32��Ʈ�� ��, ���� 32��Ʈ�� �Ҽ���
			uint32_t originSecond, originFraction;
			originSecond = (buf[24] << 24 | (buf[24 + 1] << 16) | (buf[24 + 2] << 8) | buf[24+3]);
		}
	}

	if (iRet <= 0)
	{
		OutputDebugString(_T("Ŭ���̾�Ʈ ���� ����\n"));
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
		ntp_rtt[0] = std::chrono::system_clock::now();	//�۽� �ð� ���
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
	gmtime_s(&tm,&t); //Unix �ð� struct tm ��ȯ

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
