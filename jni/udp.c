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
#include <pthread.h>       //sem_t

#define _LIB_QSA_DEF_H
#include "libqsa_common.h"

//UDP
int SndBufLen = 1024 * 128;
int RcvBufLen = 1024 * 128;

short UdpRecvFlag;
pthread_t udpvideorcvid;

int InitUdpSocket(short lPort);
void CloseUdpSocket(void);
int UdpSendBuff(int m_Socket, char *RemoteHost, int RemotePort,
		unsigned char *buf, int nlength);

//UDP音视频接收线程函数
void CreateUdpVideoRcvThread(void);
void UdpVideoRcvThread(void);

void AddMultiGroup(int m_Socket, char *McastAddr);
void DropMultiGroup(int m_Socket, char *McastAddr);
void RefreshNetSetup(int cType);

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

int init_udp_task(void);
int Init_Udp_Send_Task(void);
int Uninit_Udp_Send_Task(void);
//---------------------------------------------------------------------------
int init_udp_task(void) {
    // Recvice
    if (InitUdpSocket(LocalVideoPort) == 0) {
        LOGD("can't create video rece socket.\n\r");
        return _FALSE;
    }

    // Send
    Init_Udp_Send_Task();

    // Add Multiaddr
	AddMultiGroup(m_VideoSocket, NSMULTIADDR);  

    return _TRUE;
}

int uninit_udp_task(void) {
    CloseUdpSocket();
    Uninit_Udp_Send_Task();
    DropMultiGroup(m_VideoSocket, NSMULTIADDR);
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
		LOGD("don't create UDP send thread. \n");
		return _FALSE;
	} else {
		LOGD("create UDP send thread. \n");
        return _TRUE;
    }
}

void multi_send_thread_func(void) {
	int i;
	int HaveDataSend;
    int Status = 0;

    LOGD("create multi send thread \n");

	while (multi_send_flag == 1) {
		sem_wait(&multi_send_sem);
		HaveDataSend = 1;
        while (HaveDataSend) {
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
                        pthread_mutex_lock (&Local.udp_lock);
                        switch (Multi_Udp_Buff[i].buf[6]) {
                            case VIDEOTALK:
                                switch (Multi_Udp_Buff[i].buf[8]) {
                                    case CALL:
                                        Multi_Udp_Buff[i].isValid = 0;
                                        Status = get_device_status();
                                        cb_opt_function.cb_curr_opt(CB_CALL_FAIL, Status);
                                        LOGD("call fail, %d\n", Multi_Udp_Buff[i].buf[6]);
                                        break;
                                    case CALLEND:
                                        Multi_Udp_Buff[i].isValid = 0;
                                        Local.OnlineFlag = 0;
                                        Local.CallConfirmFlag = 0;
                                        TalkEnd_ClearStatus();
                                        LOGD("multi_send_thread_func :: TalkEnd_ClearStatus()\n");
                                        break;
                                    default:
                                        Multi_Udp_Buff[i].isValid = 0;
                                        break;
                                }
                                break;
                            default:
                                Multi_Udp_Buff[i].isValid = 0;
                                Status = get_device_status();
                                cb_opt_function.cb_curr_opt(CB_ACK_TIMEOUT, Status);
#ifdef _DEBUG
                                LOGD("communicate fail 3, %d\n",
                                        Multi_Udp_Buff[i].buf[6]);
#endif
                                break;
                        }
                        pthread_mutex_unlock (&Local.udp_lock);
                    }
                }
            }

            HaveDataSend = 0;
            for (i = 0; i < UDPSENDMAX; i++) {
                if (Multi_Udp_Buff[i].isValid == 1) {
                    HaveDataSend = 1;
                    break;
                }
            }
            usleep(100*1000);
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
		LOGD("Create socket error\r\n");
		return _FALSE;
	} else {
		LOGD("create socket.\n\r");
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
		LOGD("set ip_broadcast_ttl error.\n\r");
		return _FALSE;
    }

	nYes = 1;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, (char *) &nYes,
			sizeof((char *) &nYes)) == (-1)) {
		LOGD("set broadcast error.\n\r");
		return _FALSE;
	}

	if ((bind(m_Socket, (struct sockaddr *) &s_addr, sizeof(s_addr))) == (-1)) {
		LOGD("bind error\n\r");
		exit(errno);
		return _FALSE;
	} else {
        LOGD("bind address to socket.\n\r");
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

int UdpSendBuff(int m_Socket, char * RemoteHost, int RemotePort,
		unsigned char * buf, int nlength) {
	struct sockaddr_in To;
	int nSize;

	To.sin_family = AF_INET;
	To.sin_port = htons(RemotePort);
	To.sin_addr.s_addr = inet_addr(RemoteHost);

	nSize = sendto(m_Socket, buf, nlength, 0, (struct sockaddr*) &To,
			sizeof(struct sockaddr));
    //LOGD("&&& SEND VIDEO &&& nSize = %d, nlength = %d\n", nSize, nlength);
    //LOGD("&&& SEND VIDEO &&& RemoteHost = %s, RemotePort = %d, buf[8] = %02X\n", RemoteHost, RemotePort, buf[8]);

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
	LOGD("Create UDP video pthread!\n");
#endif
	if (ret != 0) {
		LOGD("Create video pthread error!\n");
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
		LOGD("set multicast error.\n\r");
		return;
	}
#ifdef _DEBUG
	LOGD("AddMultiGroup \n\r");
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
			LOGD("drop multicast error.\n\r");
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

    LOGD("This is udp video pthread.\n");
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
        //LOGD("FromIP:%s\n", FromIP);

		if ((buff[0] == UdpPackageHead[0]) 
                && (buff[1] == UdpPackageHead[1])
				&& (buff[2] == UdpPackageHead[2])
				&& (buff[3] == UdpPackageHead[3])
				&& (buff[4] == UdpPackageHead[4])
                && (buff[5] == UdpPackageHead[5])) {
            switch (buff[6]) {
                case NSORDER:   //主机名解析（子网内广播）
					switch(buff[7])
					{
                        case 2://解析回应
                            if (len >= 57) {
                                Recv_NS_Reply_Func(buff, FromIP, m_VideoSocket);
                            } else {
                                LOGD("解析应答数据长度异常\n");
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case VIDEOTALK:
                case VIDEOTALKTRANS:
                    switch (buff[8]) {
                        case CALL:
                            if (len >= 62) {
                                Recv_Talk_Call_Task(buff, FromIP);
                            } else {
                                LOGD("len of call is unusual\n");
                            }
                            break;
                        case LINEUSE:
                            if (len >= 57) {
                                Recv_Talk_Line_Use_Task(buff, FromIP);
                            } else {
                                LOGD("len of lineuse reply is unusual\n");
                            }
                            break;
                        case CALLANSWER:
                            if (len >= 61) {
                                LOGD("call reply is ok\n");
                                Recv_Talk_Call_Answer_Task(buff, FromIP);
                            } else {
                                LOGD("len of call reply is unusual\n");
                            }
                            break;
                        case CALLSTART:
                            if (len >= 57) {
                                Recv_Talk_Call_Start_Task(buff, FromIP);
                            } else {
                                LOGD("len of talk-start is unusual\n");
                            }
                            break;
                        case CALLCONFIRM:
                            if (len >= 61) {
                                Recv_Talk_Call_Confirm_Task(buff, FromIP);
                            } else {
                                LOGD("len of call-confirm is unusual\n");
                            }
                            break;
                        case REMOTEOPENLOCK:
                            if (len >= 57) {
                                Recv_Talk_Open_Lock_Task(buff, FromIP);
                            } else {
                                LOGD("len of remote open lock is unusual\n");
                            }
                            break;
                        case CALLEND:
                            if (len >= 57) {
                                Recv_Talk_Call_End_Task(buff, FromIP);
                            } else {
                                LOGD("len of call end is unusual\n");
                            }
                            break;
                        case FORCEIFRAME:
                            if (len >= 57) {
                                RecvForceIFrame_Func(buff, FromIP);
                            } else {
                                LOGD("len of force Iframe is unusual\n");
                            }
                            break;
                        case CALLUP:
                        case CALLDOWN:
                            Recv_Talk_Call_UpDown_Task(buff, FromIP, len);
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
        }

		if (strcmp((char *)buff, "exit") == 0) {
			LOGD("recvfrom888888888\n");
			UdpRecvFlag = 0;
		}
	}
}


//-----------------------------------------------------------------------

//解析应答
void Recv_NS_Reply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket)
{
	int i,j,k;
	int isAddrOK = 0;
	int AddrLen = 12;
    char addr[10];
    char ip[20];
    int type = 0;

    LOGD("%s \n", __FUNCTION__);

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);

    for(i=0; i<UDPSENDMAX; i++) {
        if(Multi_Udp_Buff[i].isValid == 1) {
            if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM) {
                if(Multi_Udp_Buff[i].buf[6] == NSORDER) {
                    if ((Multi_Udp_Buff[i].buf[7] == ASK)&&(recv_buf[32] > 0)) {
                        //判断要求解析地址是否匹配
                        isAddrOK = 1;
                        if ((recv_buf[33] == 'S') || (recv_buf[33] == 's')) {
                            AddrLen = 11;
                        }

                        for (j=32; j<32+AddrLen; j++) {
                            if(recv_buf[33] == 's' && j == 32)
                                continue;
                            if (Multi_Udp_Buff[i].buf[j] != recv_buf[j+1]) {
                                isAddrOK = 0;
                                break;
                            }
                        }

                        Multi_Udp_Buff[i].isValid = 0;
                        Multi_Udp_Buff[i].SendNum = 0;
                        if (isAddrOK == 1) {
                            break;         
                        }
                    }
                }
            }
        }
    }

	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);

	if (isAddrOK == 1) {
        //收到正确的解析回应
        remote_info.DenNum = 1;

        remote_info.IP[0][0] = recv_buf[53];
        remote_info.IP[0][1] = recv_buf[54];
        remote_info.IP[0][2] = recv_buf[55];
        remote_info.IP[0][3] = recv_buf[56];
        remote_info.DenIP[0] = remote_info.IP[0][0];
        remote_info.DenIP[1] = remote_info.IP[0][1];
        remote_info.DenIP[2] = remote_info.IP[0][2];
        remote_info.DenIP[3] = remote_info.IP[0][3];

        for (k=0; k<20; k++) {
            remote_info.Addr[0][k] = recv_buf[33+k];
        }

        memcpy(addr, remote_info.Addr[0] + 7, 4);
        addr[4] = '\0';
        for (k=0; k<4; k++) {
            ip[k] = remote_info.IP[0][k];
        }

        sprintf(ip, "%d.%d.%d.%d", remote_info.DenIP[0], remote_info.DenIP[1],  \
                remote_info.DenIP[2], remote_info.DenIP[3]);
        LOGD("ip:  %s\n", ip);

        if ((remote_info.Addr[0][0] == 'S') || (remote_info.Addr[0][0] == 's')) {
            type = 0;
        }

        memcpy(remote_info.GroupIP, &locate_ip, 4);
        remote_info.GroupIP[0] = 236;

        set_device_status(CB_ST_NULL);
        cb_opt_function.cb_devip(addr, ip, type);
    }
}
//-----------------------------------------------------------------------
void Recv_Talk_Call_Task(unsigned char *recv_buf, char *cFromIP) {
}
//-----------------------------------------------------------------------
void Recv_Talk_Line_Use_Task(unsigned char *recv_buf, char *cFromIP) {
	int i;
    int Status;

	pthread_mutex_lock(&Local.udp_lock);
	if (recv_buf[7] == ASK) {
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
                                            Status = CB_ST_NULL;
                                            set_device_status(Status);
                                            cb_opt_function.cb_curr_opt(CB_CALL_BUSY, Status);
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
	char RemoteIP[20];
    int Status;

	sprintf(RemoteIP, "%d.%d.%d.%d", recv_buf[53], recv_buf[54], recv_buf[55],
			recv_buf[56]);

	pthread_mutex_lock(&Local.udp_lock);
	if (recv_buf[7] == ASK) {
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

                                            Status = CB_ST_CALLING;
                                            set_device_status(Status);

											Local.CallConfirmFlag = 1;
											Local.Timer1Num = 0;
											Local.TimeOut = 0;
											Local.OnlineNum = 0;
											Local.OnlineFlag = 1;

                                            cb_opt_function.cb_curr_opt(CB_CALL_OK, Status);
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
    int Status = 0;

    Status = get_device_status();
    LOGD("%s%d: Status = %d\n", __FUNCTION__, __LINE__, Status);
	if ((Status == CB_ST_CALLING) && (recv_buf[7] == ASK)) {
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

        Status = CB_ST_TALKING;
        set_device_status(Status);
		Local.TimeOut = 0;

        cb_opt_function.cb_curr_opt(CB_TALK_OK, Status);
        LOGD("recvice start talk \n");
	}
}

void Recv_Talk_Call_Confirm_Task(unsigned char *recv_buf, char *cFromIP) {
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

    int Status = 0;
    Status = get_device_status();

	if (((Status == CB_ST_CALLING) || (Status == CB_ST_TALKING)) && (recv_buf[7] == ASK)) {
		memcpy(send_b, recv_buf, 61);
		send_b[7] = REPLY;
		sendlength = 61;
		RemotePort = RemoteVideoPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
		Local.CallConfirmFlag = 1;
	} else {
        //本机为被叫方
		if(((Local.Status == CB_ST_CALLED) || (Local.Status == CB_ST_TALKED))
				&& (recv_buf[7] == REPLY)) {
            Local.CallConfirmFlag = 1;
		}
    }
}

void Recv_Talk_Open_Lock_Task(unsigned char *recv_buf, char *cFromIP) {
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

    int Status = 0;
    Status = get_device_status();

	if (((Status == CB_ST_CALLING) || (Status == CB_ST_TALKING)) && (recv_buf[7] == ASK)) {
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY;
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
        cb_opt_function.cb_curr_opt(CB_OPEN_LOCK, Status);

        LOGD("other remote open lock\n");
	}
}

void Recv_Talk_Call_End_Task(unsigned char *recv_buf, char *cFromIP) {
	int i;
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;

    int Status = 0;
    Status = get_device_status();

	if (((Status == CB_ST_CALLING) || (Status == CB_ST_TALKING)) && (recv_buf[7] == ASK)) {
        Local.OnlineFlag = 0;
        Local.CallConfirmFlag = 0;

        memcpy(send_b, recv_buf, 57);
        send_b[7] = REPLY;
        sendlength = 57;

        RemotePort = RemoteVideoPort;
        UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);

		TalkEnd_ClearStatus();
#ifdef _DEBUG
		LOGD("other end talk, %s\n", cFromIP);
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

											TalkEnd_ClearStatus();

                                            LOGD("other reply talk end\n");
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

    int Status;
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

    Status = CB_ST_NULL;
    set_device_status(Status);
	Local.OnlineFlag = 0;
	Local.CallConfirmFlag = 0;

    cb_opt_function.cb_curr_opt(CB_TALK_STOP, Status);
}

void Recv_Talk_Call_UpDown_Task(unsigned char *recv_buf, char *cFromIP,
        int length) {

    struct talkdata1 talkdata;
    int Status;

    Status = get_device_status();
    if (Status == CB_ST_TALKING) {
        if (1 == recv_buf[61]) {
            memcpy(&talkdata, recv_buf + 9, sizeof(struct talkdata1));
            //回调声音数据
            cb_opt_function.cb_audio_data((void *)(recv_buf + 9 + sizeof(struct talkdata1)),
                    talkdata.Datalen, 0);
        }
    }
    return;
}

void ForceIFrame_Func(void) //强制I帧
{
	int i;
	int RemotePort;
    int Status = 0;

	for (i = 0; i < UDPSENDMAX; i++) {
		if (Multi_Udp_Buff[i].isValid == 0) {
			//锁定互斥锁
			pthread_mutex_lock(&Local.udp_lock);
			//只发送一次
			Multi_Udp_Buff[i].SendNum = 5;
			Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
			RemotePort = RemoteVideoPort;
			Multi_Udp_Buff[i].RemotePort = RemotePort;
			Multi_Udp_Buff[i].CurrOrder = 0;
			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",
					remote_info.DenIP[0], remote_info.DenIP[1], remote_info.DenIP[2],
					remote_info.DenIP[3]);
			//头部
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			//命令  ,子网广播解析
            Status = get_device_status();
			if ((Status == CB_ST_CALLING) || (Status == CB_ST_TALKING)) {
				Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
			}
			Multi_Udp_Buff[i].buf[7] = ASK; //主叫
			Multi_Udp_Buff[i].buf[8] = FORCEIFRAME; //FORCEIFRAME

			memcpy(Multi_Udp_Buff[i].buf + 9, local_config.address, 20);
			memcpy(Multi_Udp_Buff[i].buf + 29, local_config.ip, 4);
			memcpy(Multi_Udp_Buff[i].buf + 33, remote_info.Addr[0], 20);
			memcpy(Multi_Udp_Buff[i].buf + 53, remote_info.IP[0], 4);

			Multi_Udp_Buff[i].nlength = 57;
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数
			Multi_Udp_Buff[i].isValid = 1;

			sem_post(&multi_send_sem);
            //打开互斥锁
            pthread_mutex_unlock(&Local.udp_lock);
			break;
        }
    }
}

//对讲强制I帧
void RecvForceIFrame_Func(unsigned char *recv_buf, char *cFromIP) {
	unsigned char send_b[1520];
	int sendlength;
	int RemotePort;
    int Status = get_device_status();;

	//本机被动
	if (recv_buf[7] == ASK) {
		memcpy(send_b, recv_buf, 57);
		send_b[7] = REPLY; //应答
		sendlength = 57;
		RemotePort = RemoteVideoPort;
		UdpSendBuff(m_VideoSocket, cFromIP, RemotePort, send_b, sendlength);
        cb_opt_function.cb_curr_opt(CB_FORCE_IFRAME, Status);
	}
}

