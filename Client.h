#pragma once
#include <chrono>

#define RECV_WAIT_TIME		1000
#define UM_RECV_PACKET		(WM_USER + 3000)
#define NTP_PACKET_SIZE     48

enum RECV_PACKET_CODE
{
	NTP_SET_FAILED,
	NTP_SET_SUCCESS,
};

typedef struct
{

	uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
							 // li.   Two bits.   Leap indicator.
							 // vn.   Three bits. Version number of the protocol.
							 // mode. Three bits. Client will pick mode 3 for client.

	uint8_t stratum;         // Eight bits. Stratum level of the local clock.
	uint8_t poll;            // Eight bits. Maximum interval between successive messages.
	uint8_t precision;       // Eight bits. Precision of the local clock.

	uint32_t rootDelay;      // 32 bits. Total round trip delay time.
	uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
	uint32_t refId;          // 32 bits. Reference clock identifier.

	uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
	uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

	uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
	uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

	uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
	uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

	uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
	uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

} ntp_packet;              // Total: 384 bits or 48 bytes.

class CClient
{
public:
	CClient();
	~CClient();

	BOOL StartClient(char* csIP, int iPort, int iTimeout);
	BOOL StartClient(char* csIP, int iPort);
	void StartRecv();
	static BOOL RecvThread(CClient* pClient);
	BOOL RecvProc();
	BOOL Send(char* pBuf);
	void SetOwner(CWnd* pOwner) { if(pOwner) m_pOwner = pOwner; }
	BOOL GetConnect() { return m_bConnected; }
	void Disconnect();
	void Reconnect();
	void UnixTimeToSystemTime(time_t t, SYSTEMTIME* st);

public:
	CWnd* m_pOwner;

	SOCKET m_sock;
	char m_csIP[20];
	int m_iPort;
	int m_iTimeout;
	char buf[BUFSIZ];
	std::chrono::system_clock::time_point ntp_rtt[4];
	HANDLE m_hClose;
	BOOL m_bConnected;

	sockaddr_in m_ServerAddr;
};

