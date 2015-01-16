#include "StdAfx.h"
#include "resource.h"
#include "xClient.h"
#include "MD5.h"
#include "MainDlg.h"
#include "IPConfig.h"

xClient::xClient(CMainDlg *hDlg)
{
	m_hDlg = hDlg;
}

xClient::~xClient()
{

}

DWORD WINAPI WorkProc(LPVOID lpParam)
{
	xClient *pxClient = (xClient *) lpParam;

	//::SetThreadPriority(::GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	int retcode;
	bool  bIsServerFound = false;
	bool bIsLoginSuccess = false;
	
	UCHAR ServerMac[6];
	char pcap_filter[128];
	struct bpf_program pcap_fp;
	
	struct pcap_pkthdr *header;
	const UCHAR *captured;

	DWORD TikCount = 0;
	DWORD oldTikCount = 0;
	
	
	pxClient->SendStartPkt();
	
	::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1002);
	oldTikCount = ::GetTickCount();
	AtlTrace(TEXT("Finding server"));

	while(!bIsServerFound)
	{
		while ((retcode = pcap_next_ex( pxClient->hPcap, &header, &captured)) != 1)
		{
			//AtlTrace(TEXT("retcode = %d\n"), retcode);
			if (::WaitForSingleObject(pxClient->m_hStopEvent, 0) == WAIT_OBJECT_0)
			{				
				::CloseHandle(pxClient->m_hStopEvent);
				pcap_close(pxClient->hPcap);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1000);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1001);
				AtlTrace(TEXT("m_hThread exit!\n"));
				return 0;
			}
			//AtlTrace(TEXT("."));
			TikCount = ::GetTickCount();
			if (TikCount - oldTikCount > 30000)
			{
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1003);
				//�߳̿���  ֪ͨ���öϿ����Ӻ���
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_DISCONNETC, (LPARAM)0);
				::WaitForSingleObject(pxClient->m_hStopEvent, INFINITE);
				::CloseHandle(pxClient->m_hStopEvent);
				pcap_close(pxClient->hPcap);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1000);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1001);
				AtlTrace(TEXT("m_hThread exit!\n"));
				return 0;
			}
		}

		if ((EAP_Code)captured[18] == REQUEST)
		{
			bIsServerFound = true;
			::memcpy(ServerMac, captured+6, 6);
			AtlTrace(TEXT("\nServer founded!\n"));
		}
	}

	if (pxClient->pxcondata->pktcastset == 0)
	{
		::memcpy(pxClient->ethhdr+ 0, ServerMac, 6);
	}
	else
	{
		::memcpy(pxClient->ethhdr+ 0, MulticastAddr, 6);
	}
	
	// �����������ֻ����Ϊ802.1X��֤�豸�����İ�
	::sprintf_s(pcap_filter, 128,
						"(ether proto 0x888e) and (ether src host %02x:%02x:%02x:%02x:%02x:%02x) and (ether dst host %02x:%02x:%02x:%02x:%02x:%02x)",
						(UCHAR)ServerMac[0],(UCHAR)ServerMac[1],(UCHAR)ServerMac[2],
						(UCHAR)ServerMac[3],(UCHAR)ServerMac[4],(UCHAR)ServerMac[5],
						(UCHAR)pxClient->pxcondata->nicmac[0],(UCHAR)pxClient->pxcondata->nicmac[1],(UCHAR)pxClient->pxcondata->nicmac[2],
						(UCHAR)pxClient->pxcondata->nicmac[3],(UCHAR)pxClient->pxcondata->nicmac[4],(UCHAR)pxClient->pxcondata->nicmac[5]);

	if (pcap_compile(pxClient->hPcap, &pcap_fp, pcap_filter, 1, 0xff) == -1)
	{
		AtlTrace(TEXT("Failed at pcap_compile()\n"));
	}
	if (pcap_setfilter(pxClient->hPcap, &pcap_fp) == -1)
	{
		AtlTrace(TEXT("Failed at pcap_setfilter()\n"));
	}
   
	if ((EAP_Code)captured[22] == NOTIFICATION)
	{
		AtlTrace("[%d] Server: Request Notification!\n", (EAP_ID)captured[19]);
		::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1004);
		pxClient->SendResponseNotification(captured);
		AtlTrace("[%d] Client: Response Notification.\n", (EAP_ID)captured[19]);
	}

	while (TRUE)
	{			
		while ((retcode = pcap_next_ex( pxClient->hPcap, &header, &captured)) != 1)
		{
			//AtlTrace(TEXT("%d"), retcode);
			if (::WaitForSingleObject(pxClient->m_hStopEvent, 0) == WAIT_OBJECT_0)
			{
				if (bIsLoginSuccess)
				{
					AtlTrace(TEXT("Sending Logoff packet\n"));
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1005);
					pxClient->SendLogoffPkt();
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_IPCONFIG, (LPARAM)0);
					retcode = ::IPConfig(IPCON_RELEASE, pxClient->pxcondata->nicindex, pxClient->pxcondata->nicname);//�ͷ�IP��ַ
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_IPCONFIG, (LPARAM)1);
				}
				::CloseHandle(pxClient->m_hStopEvent);
				pcap_close(pxClient->hPcap);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1000);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1001);
				AtlTrace(TEXT("m_hThread exit!\n"));
				return 0;
			}
		}
	   
	   if((EAP_Code)captured[18] == REQUEST)	//�����յ���Request���ظ���Ӧ��Response��
	   {
	   		switch ((EAP_Type)captured[22])
	   		{
	   			case IDENTITY:
	   				AtlTrace("[%d] Server: Request Identity!\n", (EAP_ID)captured[19]);
	   				pxClient->SendResponseIdentity(captured);
	   				AtlTrace("[%d] Client: Response Identity.\n", (EAP_ID)captured[19]);
	   				break;
	   			case MD5:
	   				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1006);
	   				AtlTrace("[%d] Server: Request MD5-Challenge!\n", (EAP_ID)captured[19]);
	   				pxClient->SendResponseMD5(captured);
	   				AtlTrace("[%d] Client: Response MD5-Challenge.\n", (EAP_ID)captured[19]);
	   				break;
	   			case NOTIFICATION:
	   				AtlTrace("[%d] Server: Request Notification!\n", (EAP_ID)captured[19]);
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1004);
	   				pxClient->SendResponseNotification(captured);
	   				AtlTrace("[%d] Client: Response Notification.\n", (EAP_ID)captured[19]);
	   				break;
	   			default:
	   				AtlTrace("[%d] Server: Request (type:%d)!\n", (EAP_ID)captured[19], (EAP_Type)captured[22]);
	   				AtlTrace("Error! Unexpected request type\n");
	   				break;
			}
	   }
	   else if ((EAP_Code)captured[18] == FAILURE)	// ������֤ʧ����Ϣ
	   {	
	   		UCHAR errtype = captured[22];
	   		UCHAR msgsize = captured[23];
	   		const char *msg = (const char*) &captured[24];
			const char *ecode = (const char*) &captured[25];
	   		AtlTrace("[%d] Server: Failure.\n", (EAP_ID)captured[19]);
			::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1007);
	   		if (errtype==0x09 && msgsize>0)	// ���������ʾ��Ϣ
	   		{	
				//::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)msg);
				if (::memcmp(ecode, "2531", 4) == 0)//E2531:�û���������
				{
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1100);
				}
				else if (::memcmp(ecode, "2535", 4) == 0)//E2535:Service is paused
				{
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1101);
				}
				else if (::memcmp(ecode, "2542", 4) == 0)//E2542:���û��ʺ��Ѿ��ڱ𴦵�¼
				{
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1102);
				}
				else if (::memcmp(ecode, "2547", 4) == 0)//E2547:����ʱ������
				{
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1103);
				}
				else if (::memcmp(ecode, "2553", 4) == 0)//E2553:�������
				{
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1104);
				}
				else if(::memcmp(ecode, "2592", 4) == 0)//E2592:�豸IP�󶨼��ʧ��
				{
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1107);
				}
				else if (::memcmp(ecode, "2602", 4) == 0)//E2602:��֤�Ự������
				{
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1105);
				}
				else if (::memcmp(ecode, "3137", 4) == 0)//E3137:�ͻ��˰汾����Ч
				{
					::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1106);
				}
	   		}
	   		else if (errtype==0x08)	// ��������������ʱ�����������˴�802.1X��֤�Ự ��������ͻ������̷����µ���֤�Ự
	   		{	
	   			AtlTrace("Error Type:%02x\n", errtype);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1014);
	   		}
	   		else
	   		{
				AtlTrace("Error Type:0x%02x\n", errtype);
	   		}
			//�߳̿���  ֪ͨ���öϿ����Ӻ���
			::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_DISCONNETC, (LPARAM)0);
			::WaitForSingleObject(pxClient->m_hStopEvent, INFINITE);
			break;
	   }
	   else if ((EAP_Code)captured[18] == SUCCESS)
	   {
	   		AtlTrace("[%d] Server: Success.\n", captured[19]);
			bIsLoginSuccess = true;
			::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1008);
	   		// �ڴ�ˢ��IP��ַ
	   		AtlTrace( "Refreshing IP address...\n");
			::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1009);
			::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_IPCONFIG, (LPARAM)0);
			retcode = ::IPConfig(IPCON_RENEW, pxClient->pxcondata->nicindex,  pxClient->pxcondata->nicname);//ˢ��IP��ַ
			::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_IPCONFIG, (LPARAM)1);
			if (retcode != 1)//ˢ��IP��ַʧ�� �Ͽ�����
			{
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1013);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_DISCONNETC, (LPARAM)0);
				::WaitForSingleObject(pxClient->m_hStopEvent, INFINITE);
				break;
			}
			else
			{
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1010);
				::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_CONSUCCESS, (LPARAM)0);
			}
	   }
	   else	// TODO: ����û�д���Ϊ�Զ������ݰ� 
	   {
	   		AtlTrace("[%d] Server: (H3C data)\n", captured[19]);
	   }
	}
	::CloseHandle(pxClient->m_hStopEvent);
	pcap_close(pxClient->hPcap);
	::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1000);
	::SendMessage(pxClient->m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1001);
	return 0;
}

void xClient::Connect(PXCONDATA pcd)
{
	pxcondata = pcd;
	memcpy(ethhdr+ 6, pcd->nicmac, 6);
	ethhdr[12] = 0x88;
	ethhdr[13] = 0x8e;
	::SendMessage(m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1011);
	if (!OpenAdapter())
	{
		::SendMessage(m_hDlg->m_hWnd, XCLIENT_CMD, XC_MESSAGE, (LPARAM)IDS_STRING1012);
		::SendMessage(m_hDlg->m_hWnd, XCLIENT_CMD, XC_DISCONNETC, (LPARAM)0);
		return ;
	}

	m_hStopEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	m_hThread = ::CreateThread(NULL, 0, WorkProc,  this, 0, NULL);
	if (m_hThread == NULL || m_hStopEvent == NULL)
	{
		AtlMessageBox(m_hDlg->m_hWnd, IDS_STRING149);
	}
	::CloseHandle(m_hThread);
	return ;
}
void xClient::DisConnect()
{
	SetEvent(m_hStopEvent);
	return ;
}

bool xClient::OpenAdapter()
{     
	char pcap_filter[128];
	char nicname[MAX_ADPTER_NAME_BUFLEN+12];
	struct bpf_program pcap_fp;
	char errbuf[PCAP_ERRBUF_SIZE] = "";
	sprintf_s(nicname,  MAX_ADPTER_NAME_BUFLEN+12, "rpcap://\\Device\\NPF_%s",pxcondata->nicname);

	if(!(hPcap = pcap_open(nicname, 65536, PCAP_OPENFLAG_PROMISCUOUS , 1, NULL, errbuf)))
	{
		AtlTrace(TEXT("Open adapter failed!"));
		return false;
	}
	sprintf_s(pcap_filter, 128, "(ether proto 0x888e) and (ether dst %02x:%02x:%02x:%02x:%02x:%02x)",
		(UCHAR)pxcondata->nicmac[0],(UCHAR)pxcondata->nicmac[1],(UCHAR)pxcondata->nicmac[2],
		(UCHAR)pxcondata->nicmac[3],(UCHAR)pxcondata->nicmac[4],(UCHAR)pxcondata->nicmac[5]);

	if (pcap_compile(hPcap, &pcap_fp, pcap_filter, 1, 0xff) == -1)
	{
		CloseAdapter();
		return false;
	}
	if (pcap_setfilter(hPcap, &pcap_fp) == -1)
	{
		CloseAdapter();
		return false;
	}
	return true;
}


void xClient::CloseAdapter()
{
	pcap_close(hPcap);
}


void xClient::SendStartPkt()
{
	UCHAR packet[18];
	// Ethernet Header (14 Bytes)
	if (pxcondata->castset == 0)
	{
		memcpy(packet, BroadcastAddr, 6);
	}
	else
	{
		memcpy(packet, MulticastAddr, 6);
	}
	memcpy(packet+6, pxcondata->nicmac,   6);
	packet[12] = 0x88;
	packet[13] = 0x8e;

	// EAPOL (4 Bytes)
	packet[14] = 0x01;	// Version=1
	packet[15] = 0x01;	// Type=Start
	packet[16] = packet[17] =0x00;// Length=0x0000

	pcap_sendpacket(hPcap, packet, 18);
}
void xClient::SendLogoffPkt()
{
	UCHAR packet[18];

	// Ethernet Header (14 Bytes)

	memcpy(packet, MulticastAddr, 6);
	memcpy(packet+6, pxcondata->nicmac, 6);
	packet[12] = 0x88;
	packet[13] = 0x8e;

	// EAPOL (4 Bytes)
	packet[14] = 0x01;	// Version=1
	packet[15] = 0x02;	// Type=Logoff
	packet[16] = packet[17] =0x00;// Length=0x0000

	// ����
	pcap_sendpacket(hPcap, packet, 18);
}

void xClient::SendResponseIdentity(const UCHAR request[])
{
	UCHAR	packet[128];
	USHORT eaplen;
	size_t len;
	size_t usernamelen;

	ATLASSERT((EAP_Code)request[18] == REQUEST);
	ATLASSERT((EAP_Type)request[22] == IDENTITY);

	// Fill Ethernet header
	memcpy(packet, ethhdr, 14);

	// 802,1X Authentication
	// {
	packet[14] = 0x01;	// 802.1X Version 1
	packet[15] = 0x00;	// Type=0 (EAP Packet)
	packet[16] = 0x00;	
	packet[17] = 0x00;	// Length
	// Extensible Authentication Protocol
	// {
	packet[18] = (EAP_Code) RESPONSE;	// Code
	packet[19] = request[19];		// ID
	packet[20] = 0x00;	
	packet[21] = 0x00;	// Length
	packet[22] = (EAP_Type) IDENTITY;	// Type
	// Type-Data
	// {
	len = 23;
	packet[len++] = 0x15;	  // �ϴ�IP��ַ
	packet[len++] = 0x04;	  //
	memset(packet+len, 0x00, 4);//
	len += 4;			  //
	packet[len++] = 0x06;		  // Я���汾��
	packet[len++] = 0x07;		  //
	MakeBase64((char*)packet+len);//
	len += 28;			  //
	packet[len++] = ' '; // �����ո��
	packet[len++] = ' '; //
	usernamelen = strlen(pxcondata->account); //ĩβ����û���
	memcpy(packet+len, pxcondata->account, usernamelen);
	len += usernamelen;
	ATLASSERT(len <= sizeof(packet));
	// }
	// }
	// }
	// ����ǰ�����յ�����Length
	eaplen = htons(len-18);
	memcpy(packet+16, &eaplen, sizeof(eaplen));
	memcpy(packet+20, &eaplen, sizeof(eaplen));

	// ����
	pcap_sendpacket(hPcap, packet, len);
}

void xClient::SendResponseMD5(const UCHAR request[])
{
	UCHAR  packet[128];
	USHORT eaplen;
	size_t   usernamelen;
	size_t   len;
	
	ATLASSERT((EAP_Code)request[18] == REQUEST);
	ATLASSERT((EAP_Type)request[22] == MD5);

	usernamelen = strlen(pxcondata->account);
	eaplen = htons(22+usernamelen);
	len = 14+4+22+usernamelen; // ethhdr+EAPOL+EAP+usernamelen

	// Fill Ethernet header
	memcpy(packet, ethhdr, 14);

	// 802,1X Authentication
	// {
	packet[14] = 0x01;	// 802.1X Version 1
	packet[15] = 0x00;	// Type=0 (EAP Packet)
	memcpy(packet+16, &eaplen, sizeof(eaplen));	// Length
	// Extensible Authentication Protocol
	// {
	packet[18] = (EAP_Code) RESPONSE;// Code
	packet[19] = request[19];	// ID
	packet[20] = packet[16];	// Length
	packet[21] = packet[17];	//
	packet[22] = (EAP_Type) MD5;	// Type
	packet[23] = 16;		// Value-Size: 16 Bytes
	MakeMD5(packet+24, request[19], pxcondata->passwd, request+24);
	memcpy(packet+40, pxcondata->account, usernamelen);
	// }
	// }
	pcap_sendpacket(hPcap, packet, len);
}

void xClient::SendResponseNotification(const UCHAR request[])
{
	UCHAR	packet[60];
	size_t len;

	ATLASSERT((EAP_Code)request[18] == REQUEST);
	ATLASSERT((EAP_Type)request[22] == NOTIFICATION);

	// Fill Ethernet header
	memcpy(packet, ethhdr, 14);

	// 802,1X Authentication
	// {
	packet[14] = 0x01;	// 802.1X Version 1
	packet[15] = 0x00;	// Type=0 (EAP Packet)
	packet[16] = 0x00;	// Length
	packet[17] = 0x1b;	//

	// Extensible Authentication Protocol
	// {
	packet[18] = (EAP_Code) RESPONSE;	// Code
	packet[19] = (EAP_ID) request[19];	// ID
	packet[20] = packet[16];		// Length
	packet[21] = packet[17];		//
	packet[22] = (EAP_Type) NOTIFICATION;	// Type

	len=23;
	/* Notification Data (44 Bytes) */
	// ����ǰ2+20�ֽ�Ϊ�ͻ��˰汾
	packet[len++] = 0x01; // type 0x01
	packet[len++] = 0x1;   // length
	MakeClientVersion(packet+len);
	len += 20;

	memset(packet+len, 0, 60-len);

	pcap_sendpacket(hPcap, packet, 60);
}

void xClient::MakeMD5(UCHAR digest[], UCHAR id, const char passwd[], const UCHAR srcMD5[])
{
	UCHAR	msgbuf[128]; // msgbuf = ��id�� + ��passwd�� + ��srcMD5��
	size_t	msglen;
	size_t	passlen;
	MD5_CTX md5T;

	passlen = strlen(passwd);
	msglen = 1 + passlen + 16;
	ATLASSERT(sizeof(msgbuf) >= msglen);

	msgbuf[0] = id;
	memcpy(msgbuf+1,	 passwd, passlen);
	memcpy(msgbuf+1+passlen, srcMD5, 16);

	md5T.MD5Update(msgbuf, msglen);
	md5T.MD5Final(digest);
}

void xClient::XOR(UCHAR data[], unsigned dlen, const char key[], unsigned klen)
{
	unsigned int	i,j;
	// �Ȱ�������һ��
	for (i=0; i<dlen; i++)
		data[i] ^= key[i%klen];
	// �ٰ�������ڶ���
	for (i=dlen-1,j=0;  j<dlen;  i--,j++)
		data[i] ^= key[j%klen];
}

void xClient::MakeClientVersion(UCHAR area[])
{
	UINT32 random;
	char	 RandomKey[8+1];

	random = (UINT32) time(NULL);    // ע������ѡ����32λ����
	sprintf_s(RandomKey, 9, "%08x", random);// ����RandomKey[]�ַ���

	// ��һ��������㣬��RandomKeyΪ��Կ����16�ֽ�
	memcpy(area, pxcondata->clientver, sizeof(pxcondata->clientver));
	XOR(area, 16, RandomKey, strlen(RandomKey));

	// ��16�ֽڼ���4�ֽڵ�random������ܼ�20�ֽ�
	random = htonl(random); // �������Ϊ�����ֽ���
	memcpy(area+16, &random, 4);

	// �ڶ���������㣬��H3C_KEYΪ��Կ����ǰ�����ɵ�20�ֽ�
	XOR(area, 20, H3C_KEY, strlen(H3C_KEY));
}

void xClient::MakeBase64(char area[])
{
	UCHAR version[20];
	const char Tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"abcdefghijklmnopqrstuvwxyz"
		"0123456789+/"; // ��׼��Base64�ַ�ӳ���
	UCHAR	c1,c2,c3;
	int	i, j;

	// ��������20�ֽڼ��ܹ���H3C�汾����Ϣ
	MakeClientVersion(version);

	// Ȼ����Base64���뷨��ǰ�����ɵ�20�ֽ�����ת��Ϊ28�ֽ�ASCII�ַ�
	i = 0;
	j = 0;
	while (j < 24)
	{
		c1 = version[i++];
		c2 = version[i++];
		c3 = version[i++];
		area[j++] = Tbl[ (c1&0xfc)>>2                               ];
		area[j++] = Tbl[((c1&0x03)<<4)|((c2&0xf0)>>4)               ];
		area[j++] = Tbl[               ((c2&0x0f)<<2)|((c3&0xc0)>>6)];
		area[j++] = Tbl[                                c3&0x3f     ];
	}
	c1 = version[i++];
	c2 = version[i++];
	area[24] = Tbl[ (c1&0xfc)>>2 ];
	area[25] = Tbl[((c1&0x03)<<4)|((c2&0xf0)>>4)];
	area[26] = Tbl[               ((c2&0x0f)<<2)];
	area[27] = '=';
}