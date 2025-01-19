#pragma once
#include <chrono>

#define RECV_WAIT_TIME		1000

using Callback = void (*)(bool);

enum RECV_PACKET_CODE
{
	NTP_SET_FAILED,
	NTP_SET_SUCCESS,
};

typedef struct _TIMEOUT_
{
	int iConnect = 0;
	int iRecv = 0;
	int iSend = 0;
}TIMEOUT;


class CClient
{
public:
	CClient();
	~CClient();

	bool StartClient(char* csIP, int iPort);
	void StartRecv();

	static bool RecvThread(CClient* pClient);
	bool RecvProc();

	bool Send(char* pBuf);
	void SetOwner(CWnd* pOwner) { if(pOwner) m_pOwner = pOwner; }
	bool GetConnect() { return m_bClientConnected; }
	void Disconnect();
	void Reconnect();
	void UnixTimeToSystemTime(time_t t, SYSTEMTIME* st);
	void SetTimeout(int iConnect, int iRecv, int iSend);

	HANDLE GetCloseHandle();
	void SetCloseHandle();

	Callback m_cbConnect;
	void SetCallback(Callback cbConnect);

public:
	CWnd* m_pOwner;
	SOCKET m_sock;
	char m_csIP[20];

	char buf[BUFSIZ];
	std::chrono::system_clock::time_point ntp_rtt[4];
	BOOL m_bClientConnected;
	sockaddr_in m_ServerAddr;

	int m_iPort;
	TIMEOUT m_timeout;

private:
	HANDLE m_hClose;
};

