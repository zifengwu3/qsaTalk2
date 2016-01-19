//UDP
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>

#define _LIB_QSA_DEF_H
#include "libqsa_common.h"

//UDP
int SndBufLen = 1024 * 128;
int RcvBufLen = 1024 * 128;

extern int DebugMode;

short UdpRecvFlag;
pthread_t udpdatarcvid;
pthread_t udpvideorcvid;
int InitUdpSocket(short lPort);
void CloseUdpSocket(void);
int UdpSendBuff(int m_Socket, char *RemoteHost, int RemotePort,
		unsigned char *buf, int nlength);

//UDP音视频接收线程函数
void CreateUdpVideoRcvThread(void);
void UdpVideoRcvThread(void);

//UDP数据接收线程函数
void CreateUdpDataRcvThread(void);
void UdpDataRcvThread(void);

void AddMultiGroup(int m_Socket, char *McastAddr);
void DropMultiGroup(int m_Socket, char *McastAddr);
void RefreshNetSetup(int cType);

extern sem_t audiorec2playsem;
extern sem_t videorec2playsem;

int AudioMuteFlag;
char watchRecvBuf[1024];

//解析请求
//void Recv_NS_Ask_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket);
//解析应答
void Recv_NS_Reply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket);

void Recv_Talk_Call_Task(unsigned char *recv_buf, char *cFromIP);
void Recv_Talk_Line_Use_Task(unsigned char *recv_buf, char *cFromIP);
void Recv_Talk_Call_Answer_Task(unsigned char *recv_buf, char *cFromIP);
void Recv_Talk_Call_Confirm_Task(unsigned char *recv_buf, char *cFromIP);
void Recv_Talk_Open_Lock_Task(unsigned char *recv_buf, char *cFromIP);
void Recv_Talk_Call_Start_Task(unsigned char *recv_buf, char *cFromIP);
void Recv_Talk_Call_End_Task(unsigned char *recv_buf, char *cFromIP);
void Recv_Talk_Call_UpDown_Task(unsigned char *recv_buf, char *cFromIP,
		int length);
void TalkEnd_ClearStatus(void);
void RecvForceIFrame_Func(unsigned char *recv_buf, char *cFromIP);
void ForceIFrame_Func(void);

int init_udp_task(void);
int Init_Udp_Send_Task(void);
int Uninit_Udp_Send_Task(void);
//---------------------------------------------------------------------------
int init_udp_task(void) {
    Init_Udp_Send_Task();
    return _TRUE;
}

int Init_Udp_Send_Task(void) {
	int i;
	pthread_attr_t attr;

	for (i = 0; i < UDPSENDMAX; i++) {
		Multi_Udp_Buff[i].isValid = 0;
		Multi_Udp_Buff[i].SendNum = 0;
		Multi_Udp_Buff[i].DelayTime = 100;
		Multi_Udp_Buff[i].SendDelayTime = 0;
	}

	sem_init(&multi_send_sem, 0, 0);
	multi_send_flag = 1;
	pthread_mutex_init(&Local.udp_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&multi_send_thread, &attr, (void *) multi_send_thread_func,
			NULL);
	pthread_attr_destroy(&attr);
	if (multi_send_thread == 0) {
		printf("don't create UDP send thread. \n");
		return _FALSE;
	} else {
        return _TRUE;
    }
}

void multi_send_thread_func(void) {
	int i, k;
	int HaveDataSend;

    printf("create multi send thread \n");
	while (multi_send_flag == 1) {
		sem_wait(&multi_send_sem);
		HaveDataSend = 1;
		while (HaveDataSend) {
            pthread_mutex_lock (&Local.udp_lock);
			for (i = 0; i < UDPSENDMAX; i++) {
				if (Multi_Udp_Buff[i].isValid == 1) {
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM) {
                        if (Multi_Udp_Buff[i].isValid == 1) {
                            if (Multi_Udp_Buff[i].SendDelayTime == 0) {
                                UdpSendBuff(Multi_Udp_Buff[i].m_Socket,
                                        Multi_Udp_Buff[i].RemoteHost,
                                        Multi_Udp_Buff[i].RemotePort,
                                        Multi_Udp_Buff[i].buf,
                                        Multi_Udp_Buff[i].nlength);
                            }
                        }
                        Multi_Udp_Buff[i].SendDelayTime += 100;
                        if (Multi_Udp_Buff[i].SendDelayTime
								>= Multi_Udp_Buff[i].DelayTime) {
							Multi_Udp_Buff[i].SendDelayTime = 0;
							Multi_Udp_Buff[i].SendNum++;
						}
					}
                    if (Multi_Udp_Buff[i].SendNum >= MAXSENDNUM) {
                        switch (Multi_Udp_Buff[i].buf[6]) {
                            case VIDEOTALK:
                                switch (Multi_Udp_Buff[i].buf[8]) {
                                    case CALL:
                                        if (Multi_Udp_Buff[i].buf[6] == VIDEOTALK) {
                                            for (k = 0; k < UDPSENDMAX; k++) {
                                                if (Multi_Udp_Buff[k].isValid
                                                        == 1) {
                                                    if (Multi_Udp_Buff[k].buf[8]
                                                            == CALL) {
                                                        if (k != i)
                                                            Multi_Udp_Buff[k].isValid =
                                                                0;
                                                    }
                                                }
                                            }
                                            Multi_Udp_Buff[i].isValid = 0;
                                            printf("call fail, %d\n",
                                                    Multi_Udp_Buff[i].buf[6]);
                                        }
                                        break;
                                    case CALLEND:
                                        Multi_Udp_Buff[i].isValid = 0;
                                        Local.OnlineFlag = 0;
                                        Local.CallConfirmFlag = 0;
                                        printf("multi_send_thread_func :: TalkEnd_ClearStatus()\n");
                                        //TalkEnd_ClearStatus();
                                        break;
                                    default:
                                        Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                        printf("communicate fail 1, %d\n",
                                                Multi_Udp_Buff[i].buf[6]);
#endif
                                        break;
                                }
                                break;
                            default:
                                Multi_Udp_Buff[i].isValid = 0;
                                Local.Status = 0;
                                //recv_Call_End(1);
#ifdef _DEBUG
                                printf("communicate fail 3, %d\n",
                                        Multi_Udp_Buff[i].buf[6]);
#endif
                                break;
                        }
                        pthread_mutex_unlock (&Local.udp_lock);
                    }
					if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
							&& (Multi_Udp_Buff[i].buf[8] == CALL)) {
					}
				}
			}

			HaveDataSend = 0;
			for (i = 0; i < UDPSENDMAX; i++)
				if (Multi_Udp_Buff[i].isValid == 1) {
					HaveDataSend = 1;
					break;
				}
            pthread_mutex_unlock (&Local.udp_lock);
			usleep(100 * 1000);
		}
	}
}

int Uninit_Udp_Send_Task(void) {
	multi_send_flag = 0;
	usleep(40 * 1000);
	sem_destroy(&multi_send_sem);
	pthread_mutex_destroy(&Local.udp_lock);

    return _TRUE;
}

int InitUdpSocket(short lPort) {
	struct sockaddr_in s_addr;
	int nZero = 0;
	int m_Socket;
	int nYes;
	int ttl;
    int iLen;

	if ((m_Socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("Create socket error\r\n");
		return _FALSE;
	} else {
		printf("create socket.\n\r");
	}

	memset(&s_addr, 0, sizeof(struct sockaddr_in));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(lPort);
	s_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(LocalIP);//INADDR_ANY;

	iLen = sizeof((char *) &nZero); //  SO_SNDBUF
	nZero = SndBufLen; //128K
	setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, (char*) &nZero, iLen);
	nZero = RcvBufLen; //128K
	setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, (char*) &nZero, iLen);

	ttl = MULTITTL;
	if (setsockopt(m_Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &ttl,
			sizeof(ttl)) == (-1)) {
		printf("set ip_broadcast_ttl error.\n\r");
		return _FALSE;
    }

	nYes = 1;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, (char *) &nYes,
			sizeof((char *) &nYes)) == (-1)) {
		printf("set broadcast error.\n\r");
		return _FALSE;
	}

	if ((bind(m_Socket, (struct sockaddr *) &s_addr, sizeof(s_addr))) == (-1)) {
		printf("bind error");
		exit(errno);
		return _FALSE;
	} else {
        printf("bind address to socket.\n\r");
    }

    if (lPort == LocalDataPort) {
		m_DataSocket = m_Socket;
		CreateUdpDataRcvThread();
	}

	if (lPort == LocalVideoPort) {
		m_VideoSocket = m_Socket;
		CreateUdpVideoRcvThread();
	}
	return _TRUE;
}

void CloseUdpSocket(void) {
	UdpRecvFlag = 0;
	close(m_VideoSocket);
}

#define SMALLESTSIZE  512
int UdpSendBuff(int m_Socket, char * RemoteHost, int RemotePort,
		unsigned char * buf, int nlength) {
	struct sockaddr_in To;
	int nSize;
	To.sin_family = AF_INET;
	To.sin_port = htons(RemotePort);
	To.sin_addr.s_addr = inet_addr(RemoteHost);

#ifdef ISSENDPACKETSIZE
	if (nlength < SMALLESTSIZE)
	nlength = SMALLESTSIZE;
#endif

	nSize = sendto(m_Socket, buf, nlength, 0, (struct sockaddr*) &To,
			sizeof(struct sockaddr));
	if (buf[8] != 0x0B) {
		printf("&&& SEND VIDEO &&& nSize = %d, nlength = %d", nSize, nlength);
		printf("RemoteHost = %s, RemotePort = %d, buf[8] = %02X", RemoteHost, RemotePort, buf[8]);
	}

	return nSize;
}

void CreateUdpVideoRcvThread(void) {
	int ret;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&udpvideorcvid, &attr, (void *) UdpVideoRcvThread,
			NULL);
	pthread_attr_destroy(&attr);
#ifdef _DEBUG
	printf("Create UDP video pthread!\n");
#endif
	if (ret != 0) {
		printf("Create video pthread error!\n");
		exit(1);
	}
}

void AddMultiGroup(int m_Socket, char *McastAddr)
{
	struct ip_mreq mcast;
	mcast.imr_multiaddr.s_addr = inet_addr(McastAddr);
	mcast.imr_interface.s_addr = INADDR_ANY;
	if (setsockopt(m_Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*) &mcast,
			sizeof(mcast)) == -1) {
		printf("set multicast error.\n\r");
		return;
	}
#ifdef _DEBUG
	printf("AddMultiGroup \n\r");
#endif
}

void DropMultiGroup(int m_Socket, char *McastAddr)
{
	struct ip_mreq mcast;
	char IP_Group[20];
	if (Local.IP_Group[0] != 0) {
		sprintf(IP_Group, "%d.%d.%d.%d", Local.IP_Group[0], Local.IP_Group[1],
				Local.IP_Group[2], Local.IP_Group[3]);
		Local.IP_Group[0] = 0;
		Local.IP_Group[1] = 0;
		Local.IP_Group[2] = 0;
		Local.IP_Group[3] = 0;
		//  memset(&mcast, 0, sizeof(struct ip_mreq));
		mcast.imr_multiaddr.s_addr = inet_addr(IP_Group);
		mcast.imr_interface.s_addr = INADDR_ANY;
		if (setsockopt(m_Socket, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*) &mcast,
				sizeof(mcast)) < 0) {
			printf("drop multicast error.\n\r");
			return;
		}
	}
}

void UdpVideoRcvThread(void)
{
	char FromIP[20];
	struct sockaddr_in c_addr;
	socklen_t addr_len;
	int len;
	unsigned char buff[8096];

    printf("This is udp video pthread.");
	UdpRecvFlag = 1;

	addr_len = sizeof(c_addr);
	while (UdpRecvFlag == 1) {
		len = recvfrom(m_VideoSocket, buff, sizeof(buff) - 1, 0,
				(struct sockaddr *) &c_addr, &addr_len);
		if (len < 0) {
			perror("recvfrom");
			continue;
		}
		buff[len] = '\0';

		strcpy(FromIP, inet_ntoa(c_addr.sin_addr));

		if ((buff[0] == UdpPackageHead[0]) && (buff[1] == UdpPackageHead[1])
				&& (buff[2] == UdpPackageHead[2])
				&& (buff[3] == UdpPackageHead[3])
				&& (buff[4] == UdpPackageHead[4])
				&& (buff[5] == UdpPackageHead[5])) {
			switch (buff[6]) {
			case VIDEOTALK:
			case VIDEOTALKTRANS:
				switch (buff[8]) {
				case CALL:
					if (len >= 62) {
						Recv_Talk_Call_Task(buff, FromIP);
					} else {
						if (DebugMode == 1)
							printf("len of call is unusual\n");
					}
					break;
				case LINEUSE:
					if (len >= 57) {
						Recv_Talk_Line_Use_Task(buff, FromIP);
					} else {
						if (DebugMode == 1)
							printf("len of lineuse reply is unusual\n");
					}
					break;
				case CALLANSWER:
					if (len >= 61) {
						Recv_Talk_Call_Answer_Task(buff, FromIP);
					} else {
						if (DebugMode == 1)
							printf("len of call reply is unusual\n");
					}
					break;
				case CALLSTART:
					printf("FromIP is %s\n",FromIP);
					if (len >= 57) {
						Recv_Talk_Call_Start_Task(buff, FromIP);
					} else {
						if (DebugMode == 1)
							printf("len of talk-start is unusual\n");
					}
					break;
				case CALLCONFIRM:
					if (len >= 61) {
//						printf("UdpVideoRcvThread, 4444\n");
						Recv_Talk_Call_Confirm_Task(buff, FromIP);
					} else {
						if (DebugMode == 1)
							printf("len of call-confirm is unusual\n");
					}
					break;
				case REMOTEOPENLOCK:
					if (len >= 57) {
						Recv_Talk_Open_Lock_Task(buff, FromIP);
					} else {
						if (DebugMode == 1)
							printf("len of remote open lock is unusual\n");
					}
					break;
				case CALLEND:
					if (len >= 57) {
						Recv_Talk_Call_End_Task(buff, FromIP);
					} else {
						if (DebugMode == 1)
							printf("len of call end is unusual\n");
					}
					break;
				case FORCEIFRAME:
//					if(len == 57 || len == 58)
					if (len >= 57) {
						RecvForceIFrame_Func(buff, FromIP);
					} else {
						if (DebugMode == 1)
							printf("len of force Iframe is unusual\n");
					}
					break;
				case CALLUP:
				case CALLDOWN:
					Recv_Talk_Call_UpDown_Task(buff, FromIP, len);
					break;
				}
				break;
			}
		}

		if (strcmp((char *)buff, "exit") == 0) {
			printf("recvfrom888888888\n");
			UdpRecvFlag = 0;
		}
	}
}


//-----------------------------------------------------------------------

//解析应答
void Recv_NS_Reply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket)
{
	int i,j,k;
	int CurrOrder = 0;
	int isAddrOK = 0;
	int AddrLen = 12;

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);

	for(i=0; i<UDPSENDMAX; i++)
	  if(Multi_Udp_Buff[i].isValid == 1)
		if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
		  if((Multi_Udp_Buff[i].buf[6] == NSORDER)||(Multi_Udp_Buff[i].buf[6] == NSSERVERORDER))
			if ((Multi_Udp_Buff[i].buf[7] == ASK)&&(recv_buf[32] > 0)) {
				//判断要求解析地址是否匹配
				isAddrOK = 1;

				if (recv_buf[33] == 'S' || recv_buf[33] == 's') {
                    AddrLen = 11;
                }

				for (j=32; j<32+AddrLen; j++) {
					if (recv_buf[33] == 's' && j == 32) {
                        continue;
                    }

					if (Multi_Udp_Buff[i].buf[j] != recv_buf[j+1]) {
						isAddrOK = 0;
						break;
					}
				}

				CurrOrder = Multi_Udp_Buff[i].CurrOrder;
				Multi_Udp_Buff[i].isValid = 0;
				Multi_Udp_Buff[i].SendNum = 0;
				if(isAddrOK == 1)
				  break;         
			}
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);

#ifdef _DEBUG
	printf("isAddrOK:%d,recv_buf[8]:%c,CurrOrder:0x%02X\n",isAddrOK,recv_buf[8],CurrOrder);
#endif

	if(isAddrOK == 1)
    { 
        //收到正确的解析回应
        remote_info.DenNum = recv_buf[32];
        if((remote_info.DenNum >= 1)&&(remote_info.DenNum <= 10))
        {
            for(j=0; j<remote_info.DenNum; j++)
            {
                remote_info.IP[j][0] = recv_buf[53+24*j];
                remote_info.IP[j][1] = recv_buf[54+24*j];
                remote_info.IP[j][2] = recv_buf[55+24*j];
                remote_info.IP[j][3] = recv_buf[56+24*j];
                remote_info.DenIP[0] = remote_info.IP[j][0];
                remote_info.DenIP[1] = remote_info.IP[j][1];
                remote_info.DenIP[2] = remote_info.IP[j][2];
                remote_info.DenIP[3] = remote_info.IP[j][3];

                for(k=0; k<20; k++)
                    remote_info.Addr[j][k] = recv_buf[33+24*j+k];

                remote_info.GroupIP[0] = 236;
                remote_info.GroupIP[1] = local_config.ip[1];
                remote_info.GroupIP[2] = local_config.ip[2];
                remote_info.GroupIP[3] = local_config.ip[3];

                //锁定互斥锁
                pthread_mutex_lock (&Local.udp_lock);

                for(i=0; i<UDPSENDMAX; i++)
                    if(Multi_Udp_Buff[i].isValid == 0)
                    {
                        Multi_Udp_Buff[i].SendNum = 0;
                        Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
                        sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",
                                remote_info.DenIP[0], remote_info.DenIP[1], remote_info.DenIP[2], remote_info.DenIP[3]);
                        isAddrOK = memcmp(local_config.address, remote_info.Addr[0],11);
#ifdef _DEBUG
                        printf("Multi_Udp_Buff[i].RemoteHost is %s\n",Multi_Udp_Buff[i].RemoteHost);
                        printf("解析地址成功,正在呼叫:%d\n",CurrOrder);
#endif
                        //头部
                        memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
                        //命令
                        Multi_Udp_Buff[i].buf[6] = CurrOrder;
                        Multi_Udp_Buff[i].buf[7] = ASK;    //主叫
                        // 子命令
                        Multi_Udp_Buff[i].buf[8] = CALL;

                        memcpy(Multi_Udp_Buff[i].buf+9, local_config.address, 20);
                        memcpy(Multi_Udp_Buff[i].buf+29, local_config.ip, 4);
                        memcpy(Multi_Udp_Buff[i].buf+33, remote_info.Addr[j], 20);
                        memcpy(Multi_Udp_Buff[i].buf+53, remote_info.IP[j], 4);

                        if (remote_info.DenNum == 1) {
                            Multi_Udp_Buff[i].buf[57] = 0; //单播
                        } else {
                            Multi_Udp_Buff[i].buf[57] = 1; //组播
                        }
#ifdef _DEBUG
                        printf("Multi_Udp_Buff[i].buf[57]:%d\n",Multi_Udp_Buff[i].buf[57]);
#endif
                        //组播地址
                        Multi_Udp_Buff[i].buf[58] = remote_info.GroupIP[0];
                        Multi_Udp_Buff[i].buf[59] = remote_info.GroupIP[1];
                        Multi_Udp_Buff[i].buf[60] = remote_info.GroupIP[2];
                        Multi_Udp_Buff[i].buf[61] = remote_info.GroupIP[3];

                        Multi_Udp_Buff[i].nlength = 62;
                        Multi_Udp_Buff[i].DelayTime = 400;
                        Multi_Udp_Buff[i].isValid = 1;
                        sem_post(&multi_send_sem);
                        break;
                    }
                //打开互斥锁
                pthread_mutex_unlock (&Local.udp_lock);
            }
        }
    }
}
//-----------------------------------------------------------------------
void Recv_Talk_Call_Task(unsigned char *recv_buf, char *cFromIP) {
}
//-----------------------------------------------------------------------
void Recv_Talk_Line_Use_Task(unsigned char *recv_buf, char *cFromIP) {
	int i;

	pthread_mutex_lock(&Local.udp_lock);
	if (recv_buf[7] == ASK)
	{
		for (i = 0; i < UDPSENDMAX; i++) {
			if (Multi_Udp_Buff[i].isValid == 1) {
				if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket) {
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM) {
						if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
								|| (Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS)) {
							if (Multi_Udp_Buff[i].buf[7] == ASK) {
								if (Multi_Udp_Buff[i].buf[8] == CALL) {
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0) {
										Multi_Udp_Buff[i].isValid = 0;
										if (remote_info.DenNum == 1) {
											Local.Status = 0;
											//recv_Line_Use(1);
										}

                                        if (DebugMode == 1) {
                                            printf("receive reply of lineuse i = %d,%d.%d.%d.%d\n",
                                                    i, recv_buf[53],
                                                    recv_buf[54], recv_buf[55],
                                                    recv_buf[56]);
                                        }
                                        break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	pthread_mutex_unlock(&Local.udp_lock);
}

//-----------------------------------------------------------------------
void Recv_Talk_Call_Answer_Task(unsigned char *recv_buf, char *cFromIP) {
	int i;
	uint32_t Ip_Int;
	char RemoteIP[20];

	sprintf(RemoteIP, "%d.%d.%d.%d", recv_buf[53], recv_buf[54], recv_buf[55],
			recv_buf[56]);

	pthread_mutex_lock(&Local.udp_lock);
	if (recv_buf[7] == ASK)
	{
		for (i = 0; i < UDPSENDMAX; i++) {
			if (Multi_Udp_Buff[i].isValid == 1) {
				if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket) {
					if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM) {
						if (Multi_Udp_Buff[i].buf[6] == VIDEOTALK) {
							if (Multi_Udp_Buff[i].buf[7] == ASK) {
								if (Multi_Udp_Buff[i].buf[8] == CALL) {
									if (strcmp(Multi_Udp_Buff[i].RemoteHost,
											cFromIP) == 0) {
										if (strcmp(Multi_Udp_Buff[i].RemoteIP,
												RemoteIP) == 0) {
											Multi_Udp_Buff[i].isValid = 0;
											//  printf("remote_info.DenNum = %d, remote_info.isDirect = %d\n", remote_info.DenNum, remote_info.isDirect);
											if (local_config.address[0] == 'S') {
												Ip_Int = inet_addr(cFromIP);
												memcpy(remote_info.DenIP, &Ip_Int,
														4);
											}

											//MsgLAN2CCCallRespond();
											//StartRecVideo();//(CIF_W, CIF_H);
											//cameraInit();
											//StartRecVideo(CIF_W, CIF_H);
											Local.Status = 1;
											//recv_Call_Answer(1);
											Local.CallConfirmFlag = 1;
											Local.Timer1Num = 0;
											Local.TimeOut = 0;
											Local.OnlineNum = 0;
											Local.OnlineFlag = 1;

											printf("Local.Status = %d\n",
													Local.Status);
											printf("receive reply of call, i = %d, %d.%d.%d.%d\n",
													i, recv_buf[53],
													recv_buf[54], recv_buf[55],
													recv_buf[56]);
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	pthread_mutex_unlock(&Local.udp_lock);
}
//-----------------------------------------------------------------------
void Recv_Talk_Call_Start_Task(unsigned char *recv_buf, char *cFromIP) {
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	printf("Local.Status: %d, recv_buf[7]: %d, remote_info.DenNum = %d, remote_info.isDirect = %d\n",
			Local.Status, recv_buf[7], remote_info.DenNum, remote_info.isDirect);

	if (((Local.Status == 1) || (Local.Status == 7) || (Local.Status == 9))
			&& (recv_buf[7] == ASK)) {
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY;
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		memcpy(remote_info.Addr[0], recv_buf + 33, 20);
		memcpy(remote_info.IP[0], recv_buf + 53, 4);
		remote_info.DenNum = 1;
		if (recv_buf[6] == VIDEOTALK) {
			memcpy(remote_info.DenIP, remote_info.IP[0], 4);
		}

		//MsgLAN2CCCallPickUp();

		//StopPlayWavFile();
		//WaitAudioUnuse(2000);
		//StartPlayAudio();

		Local.Status = 5;
		Local.TimeOut = 0;
		//recv_Call_Start(1);

		if (DebugMode == 1) {
			printf("other start talk \n");
		}
	}
}

void Recv_Talk_Call_Confirm_Task(unsigned char *recv_buf, char *cFromIP) {
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	if (((Local.Status == 1) || (Local.Status == 5) || (Local.Status == 7)
			|| (Local.Status == 9)) && (recv_buf[7] == ASK)) {
		memcpy(send_b, recv_buf, 61);
		send_b[7] = REPLY;
		sendlength = 61;
		RemotePort = RemoteVideoPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		Local.CallConfirmFlag = 1;
	} else {
		if (((Local.Status == 2) || (Local.Status == 6) || (Local.Status == 8)
				|| (Local.Status == 10)) && (recv_buf[7] == REPLY)) {
			Local.CallConfirmFlag = 1;
		}
	}
}
//-----------------------------------------------------------------------
void Recv_Talk_Open_Lock_Task(unsigned char *recv_buf, char *cFromIP) {
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	if (((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
			|| (Local.Status == 6)) && (recv_buf[7] == ASK)) {
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY;
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		//recv_Open_Lock(1);

		if (DebugMode == 1) {
			printf("other remote open lock\n");
		}
	}
}

void Recv_Talk_Call_End_Task(unsigned char *recv_buf, char *cFromIP) {
	int i;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	if (((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
			|| (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
			|| (Local.Status == 9) || (Local.Status == 10))
			&& (recv_buf[7] == ASK)) {
        Local.OnlineFlag = 0;
        Local.CallConfirmFlag = 0;

        memcpy(send_b, recv_buf, 57);
        send_b[7] = REPLY;
        sendlength = 57;

        RemotePort = RemoteVideoPort;
        UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		//TalkEnd_ClearStatus();
#ifdef _DEBUG
		printf("other end talk, %s\n", cFromIP);
#endif
	} else {
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0;

		pthread_mutex_lock(&Local.udp_lock);
		if (recv_buf[7] == REPLY) {
			for (i = 0; i < UDPSENDMAX; i++) {
				if (Multi_Udp_Buff[i].isValid == 1) {
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket) {
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM) {
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOTALKTRANS)) {
								if (Multi_Udp_Buff[i].buf[7] == ASK) {
									if (Multi_Udp_Buff[i].buf[8] == CALLEND) {
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0) {
											Multi_Udp_Buff[i].isValid = 0;

											//TalkEnd_ClearStatus();

											if (DebugMode == 1)
												printf("other reply talk end\n");
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
void TalkEnd_ClearStatus(void) {
#ifdef _DEBUG
	printf("Local.Status=%d\n", Local.Status);
#endif
	DropMultiGroup(m_VideoSocket, NULL);
	switch (remote_info.Addr[0][0]) {
	case 'Z':
	case 'W':
		remote_info.Addr[0][5] = '\0';
		break;
	case 'M':
		remote_info.Addr[0][8] = '\0';
		break;
	case 'S':
		remote_info.Addr[0][12] = '\0';
		break;
	}

	switch (Local.Status) {
	case 1:
		break;
	case 5:
		//StopRecVideo();
#ifdef	_SND_RECORD_
		//StopRecAudio();
#endif
#ifdef	_SND_PLAY_
		//StopPlayAudio();
#endif
		break;
	}

	Local.Status = 0;
	Local.OnlineFlag = 0;
	Local.CallConfirmFlag = 0;
	//recv_Call_End(1);

}

void RecvForceIFrame_Func(unsigned char *recv_buf, char *cFromIP) {
	int i;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

	if (recv_buf[7] == ASK) {
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY;
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

	} else {
		pthread_mutex_lock(&Local.udp_lock);
		if (recv_buf[7] == REPLY) {
			for (i = 0; i < UDPSENDMAX; i++) {
				if (Multi_Udp_Buff[i].isValid == 1) {
					if (Multi_Udp_Buff[i].m_Socket == m_VideoSocket) {
						if (Multi_Udp_Buff[i].SendNum < MAXSENDNUM) {
							if ((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)
									|| (Multi_Udp_Buff[i].buf[6]
											== VIDEOTALKTRANS)) {
								if (Multi_Udp_Buff[i].buf[7] == ASK) {
									if (Multi_Udp_Buff[i].buf[8] == FORCEIFRAME) {
										if (strcmp(Multi_Udp_Buff[i].RemoteHost,
												cFromIP) == 0) {
											Multi_Udp_Buff[i].isValid = 0;
											break;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		pthread_mutex_unlock(&Local.udp_lock);
	}
}
void Recv_Talk_Call_UpDown_Task(unsigned char *recv_buf, char *cFromIP,
		int length) {
#if 0
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	short PackIsExist;

	TempAudioNode1 * tmp_audionode;
	int isFull;

	struct talkdata1 talkdata;
	if ((Local.Status == 1) || (Local.Status == 2) || (Local.Status == 5)
			|| (Local.Status == 6) || (Local.Status == 7) || (Local.Status == 8)
			|| (Local.Status == 9) || (Local.Status == 10)) //状态为锟皆斤拷
			{
		switch (recv_buf[61]) {
		case 1:
#if 0
			if ((Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 10)) {
				memcpy(&talkdata, recv_buf + 9, sizeof(talkdata));
				//       printf("talkdata.Frameno = %d, talkdata.timestamp = %d\n", talkdata.Frameno, talkdata.timestamp);
				if (temp_audio_n >= G711NUM) {
					temp_audio_n = G711NUM;
					if (DebugMode == 1) {
						printf("temp_audio is full");
					}
				} else {
					tmp_audionode = (TempAudioNode1 *) find_audionode(
							TempAudioNode_h, talkdata.Frameno,
							talkdata.CurrPackage);
					if (tmp_audionode == NULL) {
						isFull = creat_audionode(TempAudioNode_h, talkdata,
								recv_buf, length);
						PackIsExist = 0;
					} else {
						if (DebugMode == 1) {
							printf("audio pack exist");
						}
						PackIsExist = 1;
					}

					if (PackIsExist == 0) {
						TimeStamp.OldCurrAudio = TimeStamp.CurrAudio;
						TimeStamp.CurrAudio = talkdata.timestamp;

						//   temp_audio_n ++;
						temp_audio_n = length_audionode(TempAudioNode_h);
						if (temp_audio_n >= 4) {
							//VPLAYNUM/2 4帧 128ms
//							printf("&&&&&& audio data recv");
							sem_post(&audiorec2playsem);
						}
					}
				}
			}
#endif           

			break;
		case 2: //I帧  352*288
		case 3: //P帧  352*288
		case 4: //I帧  720*480
		case 5: //P帧  720*480
			printf("video data recv\n");
			break;

		}
	}
#endif
}

void ForceIFrame_Func(void)
{
	int i;
	int RemotePort;

	for (i = 0; i < UDPSENDMAX; i++) {
		if (Multi_Udp_Buff[i].isValid == 0) {
			pthread_mutex_lock(&Local.udp_lock);
			Multi_Udp_Buff[i].SendNum = 5;
			Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
			RemotePort = RemoteVideoPort;
			Multi_Udp_Buff[i].RemotePort = RemotePort;
			Multi_Udp_Buff[i].CurrOrder = 0;
			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",
					remote_info.DenIP[0], remote_info.DenIP[1], remote_info.DenIP[2],
					remote_info.DenIP[3]);
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			if ((Local.Status == 1) || (Local.Status == 2)
					|| (Local.Status == 5) || (Local.Status == 6)
					|| (Local.Status == 7) || (Local.Status == 8)
					|| (Local.Status == 9) || (Local.Status == 10)) {
				Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
			}
			Multi_Udp_Buff[i].buf[7] = ASK;
			Multi_Udp_Buff[i].buf[8] = FORCEIFRAME;

			memcpy(Multi_Udp_Buff[i].buf + 9, local_config.address, 20);
			memcpy(Multi_Udp_Buff[i].buf + 29, local_config.ip, 4);
			memcpy(Multi_Udp_Buff[i].buf + 33, remote_info.Addr[0], 20);
			memcpy(Multi_Udp_Buff[i].buf + 53, remote_info.IP[0], 4);

			Multi_Udp_Buff[i].nlength = 57;
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0;
			Multi_Udp_Buff[i].isValid = 1;

			pthread_mutex_unlock(&Local.udp_lock);

			sem_post(&multi_send_sem);
			break;
		}
	}
}

