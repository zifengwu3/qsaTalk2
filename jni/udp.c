//UDP
#include <stdio.h>
#include <stdlib.h>     /*标准函数库定义*/
#include <string.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>
#include <fcntl.h>

#define CommonH
#define ISSETUPPACKETSIZE   //限制最小包
#include "common.h"
extern int audio_play_wav_flag ;
enum BOOL {
    FALSE = 0,
    TRUE = !FALSE
};
//UDP
int SndBufLen=1024*128;
int RcvBufLen=1024*128;
#define CENTER "Z0000000000000000000"
char villabuf[10][60];
/////paul2.x
extern char PriMulCast[20];
short UdpRecvFlag;
pthread_t udpdatarcvid;
pthread_t udpvideorcvid;
int InitUdpSocket(short lPort);
void CloseUdpSocket(void);
int UdpSendBuff(int m_Socket, char *RemoteHost, unsigned char *buf,int nlength);
void CreateUdpDataRcvThread(void);
void CreateUdpVideoRcvThread(void);
void UdpDataRcvThread(void);  //UDP数据接收线程函数
void UdpVideoRcvThread(void);  //UDP音视频接收线程函数
void AddMultiGroup(int m_Socket, char *McastAddr);  //加入组播组
void DropMultiGroup(int m_Socket, char *McastAddr);  //退出组播组
void DropNsMultiGroup(int m_Socket, char *McastAddr);  //退出NS组播组
void RefreshNetSetup(int cType); //刷新网络设置  0 -- 未启动  1 -- 已启动
void RecvCheckMac_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
void RecvSetParam_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
void RecvTest_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
void RecvReadMac(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
extern sem_t audiorec2playsem;
extern sem_t videorec2playsem;
extern struct tempvideobuf1 tempvideobuf[MP4VNUM];   //视频临时环形缓冲  接收用
int AudioMuteFlag;   //静音标志
//接收线程内部处理函数
//设备定时报告状态
void RecvReportStatus_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
//管理中心查询设备状态
void RecvQueryStatus_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
//写地址设置
void RecvWriteAddress_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
//读地址设置
void RecvReadAddress_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
//写设置信息
void RecvWriteSetup_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
//读设置信息
void RecvReadSetup_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
//查找设备
void RecvFindEquip_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
//查找
void RecvNSAsk_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket);  //查找请求
void RecvNSReply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket);//查找应答
//监视
void RecvWatchCall_Func(unsigned char *recv_buf, char *cFromIP);  //监视呼叫
void RecvWatchLineUse_Func(unsigned char *recv_buf, char *cFromIP);  //监视占线应答
void RecvWatchCallAnswer_Func(unsigned char *recv_buf, char *cFromIP);  //监视呼叫应答
void RecvWatchCallConfirm_Func(unsigned char *recv_buf, char *cFromIP);  //通话在线确认
void RecvWatchCallEnd_Func(unsigned char *recv_buf, char *cFromIP);  //监视呼叫结束
void RecvWatchZoomOut_Func(unsigned char *recv_buf, char *cFromIP);  //放大(720*480)
void RecvWatchZoomIn_Func(unsigned char *recv_buf, char *cFromIP);  //缩小(352*240)
void RecvWatchCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length);  //监视数据
//对讲
void RecvTalkCall_Func(unsigned char *recv_buf, char *cFromIP);  //对讲呼叫
void RecvTalkLineUse_Func(unsigned char *recv_buf, char *cFromIP);  //对讲占线应答
void RecvTalkCallAnswer_Func(unsigned char *recv_buf, char *cFromIP);  //对讲呼叫应答
void RecvTalkCallConfirm_Func(unsigned char *recv_buf, char *cFromIP); //对讲在线确认
void RecvTalkOpenLock_Func(unsigned char *recv_buf, char *cFromIP); //对讲远程开锁
void RecvTalkCallAsk_Func(unsigned char *recv_buf, char *cFromIP);  //对讲开始通话询问
void RecvTalkCallStart_Func(unsigned char *recv_buf, char *cFromIP);  //对讲开始通话
void RecvTalkCallEnd_Func(unsigned char *recv_buf, char *cFromIP);  //对讲呼叫结束
void RecvTalkZoomOut_Func(unsigned char *recv_buf, char *cFromIP);  //放大(720*480)
void RecvTalkZoomIn_Func(unsigned char *recv_buf, char *cFromIP);  //缩小(352*240)
void RecvTalkCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length);  //对讲数据
void TalkEnd_ClearStatus(void); //对讲结束，清状态和关闭音视频
void ForceIFrame_Func(void);  //强制I帧
void SaveToFlash(int savetype);    //向Flash中存储文件
void RecvReadMac_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
void CallEndOther_Func(unsigned char *recv_buf, char *cFromIP);
void Recv_Alarm_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
void RecvOpenLock_Func(unsigned char *recv_buf);
void RecvRSVilla_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
void RecvSetPWD_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket);
void RecvSendTalkInfo_Func(unsigned char *recv_buf,char *cFromIP,int m_Socket);
//---------------------------------------------------------------------------
int InitUdpSocket(short lPort)
{
	struct sockaddr_in s_addr;
	int  nZero=0;
	int  iLen;
	int m_Socket;
	int  nYes;
	int ret;
	int ttl;
	int rc;
	int flags;

	/* 创建 socket , 关键在于这个 SOCK_DGRAM */
	if ((m_Socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Create socket error\r\n");
		exit(errno);
		return 0;
	}
	else
		LOGD("create socket.\n\r");

	if(m_EthSocket == 0)
		m_EthSocket = socket(AF_INET, SOCK_DGRAM, 0);    

	memset(&s_addr, 0, sizeof(struct sockaddr_in));
	/* 设置地址和端口信息 */
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(lPort);
	s_addr.sin_addr.s_addr = INADDR_ANY;//inet_addr(LocalIP);//INADDR_ANY;

	iLen=sizeof(nZero);           //  SO_SNDBUF
	nZero=SndBufLen;       //128K
	setsockopt(m_Socket,SOL_SOCKET,SO_SNDBUF,(char*)&nZero,sizeof((char*)&nZero));
	nZero=RcvBufLen;       //128K
	setsockopt(m_Socket,SOL_SOCKET,SO_RCVBUF,(char*)&nZero,sizeof((char*)&nZero));

	ttl = 5; //设置TTL  
	rc = setsockopt(m_Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&ttl, sizeof(ttl));
	//音视频端口，允许发送和接收广播
	if(lPort == LocalVideoPort)
	{
		nYes = 1;
		if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, (char *)&nYes, sizeof((char *)&nYes))== -1)
		{
			LOGD("set broadcast error.\n\r");
			return 0;
		}
	}

	/* 绑定地址和端口信息 */
	if ((bind(m_Socket, (struct sockaddr *) &s_addr, sizeof(s_addr))) == -1)
	{
		perror("bind error");
		exit(errno);
		return 0;
	}
	else
	  LOGD("bind address to socket.\n\r");
	if(lPort == LocalDataPort)
	{
		m_DataSocket = m_Socket;
		//创建UDP接收线程
		CreateUdpDataRcvThread();
	}
	if(lPort == LocalVideoPort)
	{
		m_VideoSocket = m_Socket;
		//创建UDP接收线程
		CreateUdpVideoRcvThread();
	}

	if(lPort == 8307)
	  m_VideoSendSocket = m_Socket;
	return 1;
}
//---------------------------------------------------------------------------
void CloseUdpSocket(void)
{
	UdpRecvFlag = 0;
	pthread_cancel(udpdatarcvid);
	pthread_cancel(udpvideorcvid);
	close(m_DataSocket);
	close(m_VideoSocket);
}
//---------------------------------------------------------------------------
#define SMALLESTSIZE  512    //UDP最小包大小
int UdpSendBuff(int m_Socket, char *RemoteHost, unsigned char *buf,int nlength)
{
	struct sockaddr_in To;
	int nSize;
	int n;
	To.sin_family=AF_INET;
	if(m_Socket == m_DataSocket)
		To.sin_port=htons(RemoteDataPort);
	if(m_Socket == m_VideoSocket)
		To.sin_port=htons(RemoteVideoPort);
	if(m_Socket == m_VideoSendSocket)
		To.sin_port=htons(RemoteVideoPort);

	To.sin_addr.s_addr = inet_addr(RemoteHost);

#ifdef ISSETUPPACKETSIZE
	//LOGD("6::33 == 0x%02X::%c\n",buf[6],buf[33]);
	//LOGD("6::9 == 0x%02X::%c\n",buf[6],buf[9]);
	if(((buf[6] == 0x96) && (buf[33] == 's')) || ((buf[6] == 0x98) && (buf[9] == 's')))
	{
		if(nlength < SMALLESTSIZE)
		  nlength = SMALLESTSIZE;
	}
#endif

#if 0
	if((buf[6] == 0x96) && ((buf[8] == 0x08) || (buf[8] == 0x07)) && (buf[33] == 'S'))
	{
		if(nlength < 512)
		  nSize = 512;
		else
		  nSize = nlength;
	}
	else
	  nSize = nlength;

	nSize=sendto(m_Socket,buf,nSize,0,(struct sockaddr*)&To,sizeof(struct sockaddr));
#else
	//LOGD("nlength:%d\n",nlength);
	nSize = sendto(m_Socket,buf,nlength,0,(struct sockaddr*)&To,sizeof(struct sockaddr));
#endif

	//nSize=sendto(m_Socket,buf,nlength,0,(struct sockaddr*)&To,sizeof(struct sockaddr));
	//LOGD("\nthe nSize is %d \n",nSize);
	return nSize;
}
void CreateUdpDataRcvThread()
{
	int i,ret;
	pthread_attr_t attr;
	/*  pthread_attr_t attr;
		struct sched_param param;
		int newprio=0;
	//获取线程属性
	pthread_attr_init(&attr);
	pthread_attr_getschedparam(&attr, &param);
	LOGD ("thread priority is %d!\n",param.sched_priority);
	//设置线程优先级
	param.sched_priority=newprio;
	pthread_attr_setschedparam(&attr, &param);
	ret=pthread_create(&tid, &attr, (void *)thread, NULL);  */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret=pthread_create(&udpdatarcvid, &attr, (void *)UdpDataRcvThread, NULL);
	pthread_attr_destroy(&attr);
#ifdef _DEBUG
	LOGD ("Create UDP data pthread!\n");
#endif
	if(ret!=0){
		LOGD ("Create data pthread error!\n");
		exit (1);
	}
}
void CreateUdpVideoRcvThread()
{
	pthread_attr_t attr;
	int i,ret;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret=pthread_create(&udpvideorcvid, &attr, (void *)UdpVideoRcvThread, NULL);
	pthread_attr_destroy(&attr);
#ifdef _DEBUG
	LOGD ("Create UDP video pthread!\n");
#endif
	if(ret!=0){
		LOGD ("Create video pthread error!\n");
		exit (1);
	}
}

void AddMultiGroup(int m_Socket, char *McastAddr)  //加入组播组
{
	// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0
	//  memset(&mcast, 0, sizeof(struct ip_mreq));
	mcast.imr_multiaddr.s_addr = inet_addr(McastAddr);
	mcast.imr_interface.s_addr = INADDR_ANY;
	if( setsockopt(m_Socket,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char*)&mcast,sizeof(mcast)) == -1)
	{
		LOGD("set multicast error.\n\r");
		return;
	}
}

void DropMultiGroup(int m_Socket, char *McastAddr)  //退出组播组
{
	// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0
	char IP_Group[20];
	//查看是否在其它组播组内
	if(Local.IP_Group[0] != 0)
	{
		sLOGD(IP_Group, "%d.%d.%d.%d\0",
				Local.IP_Group[0],Local.IP_Group[1],Local.IP_Group[2],Local.IP_Group[3]);
		Local.IP_Group[0] = 0; //组播地址
		Local.IP_Group[1] = 0;
		Local.IP_Group[2] = 0;
		Local.IP_Group[3] = 0;
		//  memset(&mcast, 0, sizeof(struct ip_mreq));
		mcast.imr_multiaddr.s_addr = inet_addr(IP_Group);
		mcast.imr_interface.s_addr = INADDR_ANY;
		if( setsockopt(m_Socket,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char*)&mcast,sizeof(mcast)) == -1)
		{
			LOGD("drop multicast error.\n\r");
			return;
		}
	}
}

void DropNsMultiGroup(int m_Socket, char *McastAddr)  //退出NS组播组
{
	// Winsock1.0
	struct ip_mreq mcast; // Winsock1.0

	//  memset(&mcast, 0, sizeof(struct ip_mreq));
	mcast.imr_multiaddr.s_addr = inet_addr(McastAddr);
	mcast.imr_interface.s_addr = INADDR_ANY;
	if( setsockopt(m_Socket,IPPROTO_IP,IP_DROP_MEMBERSHIP,(char*)&mcast,sizeof(mcast)) == -1)
	{
		LOGD("drop multicast error.\n\r");
		return;
	}
}

void RefreshNetSetup(int cType) //刷新网络设置  0 -- 未启动  1 -- 已启动
{
	char SystemOrder[100];
	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);
	//退出NS组播组
	if(cType == 1)
	{
		DropNsMultiGroup(m_VideoSocket, NSMULTIADDR);
	}
#if 0
	//设置MAC地址
	system("ifconfig eth0 down");
	sLOGD(SystemOrder, "ifconfig eth0 hw ether %02X:%02X:%02X:%02X:%02X:%02X\0",
			LocalCfg.Mac_Addr[0], LocalCfg.Mac_Addr[1], LocalCfg.Mac_Addr[2],
			LocalCfg.Mac_Addr[3], LocalCfg.Mac_Addr[4], LocalCfg.Mac_Addr[5]);
	system(SystemOrder);
	system("ifconfig eth0 up");
	//设置IP地址和子网掩码
	sLOGD(SystemOrder, "ifconfig eth0 %d.%d.%d.%d netmask %d.%d.%d.%d\0",
			LocalCfg.IP[0], LocalCfg.IP[1], LocalCfg.IP[2], LocalCfg.IP[3],
			LocalCfg.IP_Mask[0], LocalCfg.IP_Mask[1], LocalCfg.IP_Mask[2], LocalCfg.IP_Mask[3]);
	system(SystemOrder);
	//设置网关
	sLOGD(SystemOrder, "route add default gw %d.%d.%d.%d\0",
			LocalCfg.IP_Gate[0], LocalCfg.IP_Gate[1], LocalCfg.IP_Gate[2], LocalCfg.IP_Gate[3]);
	system(SystemOrder);
#endif
	//加入NS组播组
	if(cType == 1)
	{
		AddMultiGroup(m_VideoSocket, NSMULTIADDR);
	}
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);
}

void UdpDataRcvThread(void)  //UDP接收线程函数
{
	/* 循环接收数据 */
	//  int oldframeno=0;
	unsigned char send_b[1520];
	int sendlength;
	char FromIP[20];
	int newframeno;
	int currpackage;
	int i,j;
	int sub;
	short PackIsExist; //数据包已接收标志
	short FrameIsNew;  //数据包是否是新帧的开始
	struct sockaddr_in c_addr;
	socklen_t addr_len;
	int len;
	int tmp;
	unsigned char buff[8096];
	static int errorcount = 0;

	char tmpAddr[21];
	int isAddrOK;
#ifdef _DEBUG
	LOGD("This is udp pthread.\n");
#endif
	UdpRecvFlag = 1;

	addr_len = sizeof(c_addr);
	while (UdpRecvFlag == 1)
	{
		len = recvfrom(m_DataSocket, buff, sizeof(buff) - 1, 0,
				(struct sockaddr *) &c_addr, &addr_len);
		if (len < 0)
		{
			perror("recvfrom");
			errorcount++;
			continue;
		}
		buff[len] = '\0';
		//    LOGD("收到来自%s:%d的消息:%s\n\r",
		//            inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), buff);
		strcpy(FromIP, inet_ntoa(c_addr.sin_addr));

		if((buff[0]==UdpPackageHead[0])&&(buff[1]==UdpPackageHead[1])&&(buff[2]==UdpPackageHead[2])
				&&(buff[3]==UdpPackageHead[3])&&(buff[4]==UdpPackageHead[4])&&(buff[5]==UdpPackageHead[5]))
		{
			switch(buff[6])
			{
				case SENDTALKINFO:
					RecvSendTalkInfo_Func(buff,FromIP,m_DataSocket);
					break;
				case SETPWD:
					RecvSetPWD_Func(buff,FromIP,len,m_DataSocket);
					break;
				case READVILLA:
					//RecvReadVilla_Func(buff,FromIP,len,m_DataSocket);
					//break;
				case SETVILLA:
					RecvRSVilla_Func(buff,FromIP,len,m_DataSocket);
					break;
				case LIFT:
					switch (buff[8])
					{
						case OPENLOCK:
							RecvOpenLock_Func(buff);
							break;
					}
					break;
				case ALARM:   //报警
					if(len >= 56)
					{
						RecvAlarm_Func(buff, FromIP, len, m_DataSocket);
					}
					else
					{
#ifdef _DEBUG
						LOGD("报警应答长度异常\n");
#endif
					}
					break;
				case REPORTSTATUS:   //设备定时报告状态
					if(len >= 40)
					{
						RecvReportStatus_Func(buff, FromIP, len, m_DataSocket);
					}
					else
					{
#ifdef _DEBUG
						LOGD("设备定时报告状态应答长度异常\n");
#endif
					}
					break;
				case QUERYSTATUS:   //管理中心查询设备状态
					if(len == 40)
					{
						RecvQueryStatus_Func(buff, FromIP, len, m_DataSocket);
					}
					else
					{
#ifdef _DEBUG
						LOGD("管理中心查询设备状态长度异常\n");
#endif
					}
					break;
				case WRITEADDRESS:   //写地址设置
					if(len == 72)
					{
						RecvWriteAddress_Func(buff, FromIP, len, m_DataSocket);
					}
					else
					{
#ifdef _DEBUG
						LOGD("写地址设置长度异常\n");
#endif
					}
					break;
				case READADDRESS:   //读地址设置
					if(len == 28)
					{
						RecvReadAddress_Func(buff, FromIP, len, m_DataSocket);
					}
					else
					{
#ifdef _DEBUG
						LOGD("读地址设置长度异常\n");
#endif
					}
					break;
				case WRITESETUP:   //写设置信息
					if(len == 54)
					{
						RecvWriteSetup_Func(buff, FromIP, len, m_DataSocket);
					}
					else
					{
#ifdef _DEBUG
						LOGD("写设置信息长度异常\n");
#endif
					}
					break;
				case READSETUP:   //读设置信息
					if(len == 28)
					{
						RecvReadSetup_Func(buff, FromIP, len, m_DataSocket);
					}
					else
					{
#ifdef _DEBUG
						LOGD("读设置信息长度异常\n");
#endif
					}
					break;
				case CHECKMAC:
					if(len>=39)
						RecvCheckMac_Func(buff,FromIP,len,m_DataSocket);
					break;
				case SETPARAM:
					if(len>=100)
						RecvSetParam_Func(buff,FromIP,len,m_DataSocket);
					break;
				case MAKEOLD:
					if(len>=32)
						RecvTest_Func(buff,FromIP,len,m_DataSocket);
					break;
				case READMAC:
					if(len>=32)
						RecvReadMac_Func(buff,FromIP,len,m_DataSocket);
					break;


					//        case NSOrder:   //主机名查找（子网内广播）
				case NSSERVERORDER:  //主机名查找(NS服务器)
					switch(buff[7])
					{
						case 1://查找
							if(len >= 56)
							{
                                TurnToCenter = 1;
                                RecvNSAsk_Func(buff, FromIP, m_VideoSendSocket);
							}
							else
							{
#ifdef _DEBUG
								LOGD("查找请求数据长度异常\n");
#endif
							}
							break;
						case 2://查找回应
							if(len >= 57)
							{
                                TurnToCenter = 1;
								RecvNSReply_Func(buff, FromIP, m_VideoSendSocket);
							}
							else
							{
#ifdef _DEBUG
								LOGD("查找应答数据长度异常\n");
#endif
							}
							break;
					}
					break;
			}
		}
		if(strcmp(buff,"exit")==0)
		{
			LOGD("recvfrom888888888");
			UdpRecvFlag=0;
			//   break;
		}   
	}
}
//---------------------------------------------------------------------------
//设备定时报告状态
void RecvReportStatus_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	//时钟
	time_t t;

	i = 0;
	isAddrOK = 1;
		time(&t);
			curr_tm_t=localtime(&t);


	//for(j=8; j<8+Local.AddrLen; j++)
	//	if(LocalCfg.Addr[j-8] != recv_buf[j])
	//	{
	//		isAddrOK = 0;
	//		break;
	//	}
	//地址匹配
	if(isAddrOK == 1)
	{
		//锁定互斥锁
		pthread_mutex_lock (&Local.udp_lock);
		if(recv_buf[7] == REPLY)   //应答
			for(i=0; i<UDPSENDMAX; i++)
				if(Multi_Udp_Buff[i].isValid == 1)
					if(Multi_Udp_Buff[i].m_Socket == m_DataSocket)
						if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
							if(Multi_Udp_Buff[i].buf[6] == REPORTSTATUS)
								if(Multi_Udp_Buff[i].buf[7] == ASK)
									if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
	//									if(((recv_buf[29] << 8) + recv_buf[28]) != LocalCfg.ReportTime)
	//									{
	//										LocalCfg.ReportTime = (recv_buf[29] << 8) + recv_buf[28];
	//										Save_Setup();    //向Flash中存储文件
	//									}
										//校准时钟 
										if(((curr_tm_t->tm_year + 1900) != ((recv_buf[54] << 8) + recv_buf[55]))
												||((curr_tm_t->tm_mon+1) != recv_buf[56])
												||(curr_tm_t->tm_mday != recv_buf[57])
												||(curr_tm_t->tm_hour != recv_buf[58])
												||(curr_tm_t->tm_min != recv_buf[59]))
											//	||(curr_tm_t->tm_sec != recv_buf[36]))
										{
											curr_tm_t->tm_year   = (recv_buf[54] << 8) + recv_buf[55] - 1900;
											curr_tm_t->tm_mon   =   recv_buf[56] - 1;
											curr_tm_t->tm_mday   =   recv_buf[57];
											curr_tm_t->tm_hour   =   recv_buf[58];
											curr_tm_t->tm_min   = recv_buf[59];
											curr_tm_t->tm_sec   =   recv_buf[60];
											t=mktime(curr_tm_t);
											stime((long*)&t);
										}
#ifdef _DEBUG
										LOGD("收到设备定时报告状态应答\n");
#endif
										break;
									}
		//打开互斥锁
		pthread_mutex_unlock (&Local.udp_lock);
	}
}
//---------------------------------------------------------------------------
//管理中心查询设备状态
void RecvQueryStatus_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	//时钟
	time_t t;

	i = 0;
	isAddrOK = 1;
	for(j=8; j<8+Local.AddrLen; j++)
		if(LocalCfg.Addr[j-8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//地址匹配
	if(isAddrOK == 1)
	{
		memcpy(send_b, UdpPackageHead, 6);
		send_b[6] = QUERYSTATUS;
		send_b[7] = REPLY;    //应答
		memcpy(send_b + 8, LocalCfg.Addr, 20);
		memcpy(send_b + 28, LocalCfg.Mac_Addr, 6);
		sendlength = 34;
		UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);
	}
}
//---------------------------------------------------------------------------
//写地址设置
void RecvWriteAddress_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for(j=8; j<8+Local.AddrLen; j++)
		if(LocalCfg.Addr[j-8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//地址匹配
	if(isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY;    //应答
		sendlength = length;
		UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);

		//写地址设置
		if(recv_buf[28] & 0x01) //地址编码
			memcpy(LocalCfg.Addr, recv_buf + 30, 20);
		if(recv_buf[28] & 0x02) //网卡地址
		{
			memcpy(LocalCfg.Mac_Addr, recv_buf + 50, 6);
		}
		if(recv_buf[28] & 0x04) //IP地址
		{
			memcpy(LocalCfg.IP, recv_buf + 56, 4);
		}
		if(recv_buf[28] & 0x08) //子网掩码
		{
			memcpy(LocalCfg.IP_Mask, recv_buf + 60, 4);
		}
		if(recv_buf[28] & 0x10) //网关地址
		{
			memcpy(LocalCfg.IP_Gate, recv_buf + 64, 4);
		}
		if(recv_buf[28] & 0x20) //服务器地址
			memcpy(LocalCfg.IP_Server, recv_buf + 68, 4);

		Save_Setup();    //向Flash中存储文件

		RefreshNetSetup(1); //刷新网络设置
	}
}
//---------------------------------------------------------------------------
//读地址设置
void RecvReadAddress_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for(j=8; j<8+Local.AddrLen; j++)
		if(LocalCfg.Addr[j-8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//地址匹配
	if(isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY;    //应答

		send_b[28] = 0;
		send_b[29] = 0;

		//地址编码
		memcpy(send_b + 30, LocalCfg.Addr, 20);
		//网卡地址
		memcpy(send_b + 50, LocalCfg.Mac_Addr, 6);
		//IP地址
		memcpy(send_b + 56, LocalCfg.IP, 4);
		//子网掩码
		memcpy(send_b + 60, LocalCfg.IP_Mask, 4);
		//网关地址
		memcpy(send_b + 64, LocalCfg.IP_Gate, 4);
		//服务器地址
		memcpy(send_b + 68, LocalCfg.IP_Server, 4);

		sendlength = 72;
		UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);
	}
}
//---------------------------------------------------------------------------
//写设置信息
void RecvWriteSetup_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for(j=8; j<8+Local.AddrLen; j++)
		if(LocalCfg.Addr[j-8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//地址匹配
	if(isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY;    //应答
		sendlength = length;
		UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);

		//写设置信息
		if(recv_buf[28] & 0x01) //开锁时间
			LocalCfg.OpenLockTime = recv_buf[30];
		if(recv_buf[28] & 0x02) //延时开锁
			LocalCfg.DelayLockTime = recv_buf[31];
		if(recv_buf[28] & 0x04) //密码开锁
			LocalCfg.PassOpenLock = recv_buf[32];
		if(recv_buf[28] & 0x08) //刷卡开锁
			LocalCfg.CardOpenLock = recv_buf[33];

		if(recv_buf[29] & 0x01) //工程密码
		{
			memcpy(LocalCfg.EngineerPass, recv_buf + 38, 8);
			/////////////paul
			LocalCfg.EngineerPass[6] = '\0';
		}
		if(recv_buf[29] & 0x02) //开锁密码
		{
			memcpy(LocalCfg.OpenLockPass, recv_buf + 46, 8);
			//////////paul
			LocalCfg.OpenLockPass[6] = '\0';
		}

		Save_Setup();    //向Flash中存储文件
	}
}
//---------------------------------------------------------------------------
//读设置信息
void RecvReadSetup_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int newlength;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for(j=8; j<8+Local.AddrLen; j++)
		if(LocalCfg.Addr[j-8] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	//地址匹配
	if(isAddrOK == 1)
	{
		memcpy(send_b, recv_buf, length);
		send_b[7] = REPLY;    //应答

		send_b[28] = 0;
		send_b[29] = 0;

		//开锁时间
		send_b[30] = LocalCfg.OpenLockTime;
		//延时开锁
		send_b[31] = LocalCfg.DelayLockTime;
		//密码开锁
		send_b[32] = LocalCfg.PassOpenLock;
		//刷卡开锁
		send_b[33] = LocalCfg.CardOpenLock;

		//工程密码
		memcpy(send_b + 38, LocalCfg.EngineerPass, 8);
		//开锁密码
		memcpy(send_b + 46, LocalCfg.OpenLockPass, 8);

		sendlength = 54;
		UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);
	}
}
//---------------------------------------------------------------------------
void UdpVideoRcvThread(void)  //UDP接收线程函数
{
	/* 循环接收数据 */
	//  int oldframeno=0;
	unsigned char send_b[1520];
	int sendlength;
	char FromIP[20];
	int newframeno;
	int currpackage;
	int i,j;
	int sub;
	short PackIsExist; //数据包已接收标志
	short FrameIsNew;  //数据包是否是新帧的开始
	struct sockaddr_in c_addr;
	socklen_t addr_len;
	int len;
	int tmp;
	////////////////paul////////////////
	char wavFile[100];
	//////////////////////////////////
	unsigned char buff[8096];
	static int errorcount = 0;

	//  char tmpAddr[21];
	int isAddrOK;
#ifdef _DEBUG
	LOGD("This is udp video pthread.\n");
#endif
	UdpRecvFlag = 1;

	addr_len = sizeof(c_addr);
	while (UdpRecvFlag == 1)
	{
	//	LOGD("m_VideoSocket:%d\n",m_VideoSocket);
	//	LOGD("buff:%s\n",buff);
	//	LOGD("buff_len:%d\n",sizeof(buff));
	//	LOGD("收到来自%s:%d的消息:%s\n\r",
	//				inet_ntoa(c_addr.sin_addr), ntohs(c_addr.sin_port), buff);
	//	usleep(79*1000);
		len = recvfrom(m_VideoSocket, buff, sizeof(buff) - 1, 0,
				(struct sockaddr *) &c_addr, &addr_len);
	//	LOGD("f-len:%d\n",len);
		if (len < 0)
		{
			perror("recvfrom");
			errorcount++;
			//len = 0;
#ifdef _DEBUG
			LOGD("v---------------------------------------------len:%d::::count:%d\n",len,errorcount);
			/*
			LOGD("[buf content]:\n");
			for(i=0; i<56; i++)
			{
				LOGD("0x%02X ",buff[i]);
				if(i%8 == 0 && i != 0)
				  LOGD("\n");
			}
			LOGD("\n");
			*/
#endif
			continue;
			exit(errno);
		}
		buff[len] = '\0';
		strcpy(FromIP, inet_ntoa(c_addr.sin_addr));
		//LOGD("FromIP is %s\nbuff[7]=0x%2x,buff[6]:0x%02x\n",FromIP,buff[7],buff[6]);
		//usleep(79*1000);
		//UdpSendBuff(buff,len+1);
		if((buff[0]==UdpPackageHead[0])&&(buff[1]==UdpPackageHead[1])&&(buff[2]==UdpPackageHead[2])
				&&(buff[3]==UdpPackageHead[3])&&(buff[4]==UdpPackageHead[4])&&(buff[5]==UdpPackageHead[5]))
		{
			switch(buff[6])
			{
				case NSORDER:   //主机名查找（子网内多播）
					//case NSSERVERORDER:  //主机名查找(NS服务器)
					switch(buff[7])
					{
						case 1://查找
							if(len >= 56)
                            {
                                TurnToCenter = 0;
                                RecvNSAsk_Func(buff, FromIP, m_VideoSendSocket);
                            }
							else
							  LOGD("查找请求数据长度异常\n");
							break;
						case 2://查找回应
							if(len >= 57)
							{
                                TurnToCenter = 0;
								RecvNSReply_Func(buff, FromIP, m_VideoSendSocket);
							}
							else
							  LOGD("查找应答数据长度异常\n");
							break;
					}
					break;
				case VIDEOTALK:    //局域网可视对讲
				case VIDEOTALKTRANS:  //局域网可视对讲中转服务
					switch(buff[8])
					{
						case 26:
						case 25:  //某一方接听
							//结束播放视频
							//如为对方结束，需应答
							if(len >= 57)
							  CallEndOther_Func(buff, FromIP);
							else
							  LOGD("wrong length\n");
							break;

						case CALL:        //对方发起对讲
							if(len >= 62)
							  RecvTalkCall_Func(buff, FromIP);
							else
							  LOGD("对讲呼叫数据长度异常\n");
							break;
						case LINEUSE:        //对方正忙
							if(len >= 57)
							  RecvTalkLineUse_Func(buff, FromIP);
							else
							  LOGD("占线应答数据长度异常\n");
							break;
						case CALLANSWER:  //呼叫应答
							if (len >= 62)
                            {
                                RecvTalkCallAnswer_Func(buff, FromIP);
                            }
							else
							  LOGD("对讲呼叫应答数据长度异常\n");
							break;
						case CALLSTART:  //被叫方开始通话
							if(len >= 57)
							  RecvTalkCallStart_Func(buff, FromIP);
							else
							  LOGD("对讲开始通话数据长度异常\n");
							break;
						case CALLCONFIRM:     //通话在线确认
							if(len >= 61)
							  RecvTalkCallConfirm_Func(buff, FromIP);
							else
							  LOGD("对讲通话在线确认数据长度异常\n");
							break;
						case REMOTEOPENLOCK:  //被叫方远程开锁
							if(len >= 57)
							  RecvTalkOpenLock_Func(buff, FromIP);
							else
							  LOGD("远程开锁数据长度异常\n");
							break;
						case CALLEND:  //通话结束
							//结束播放视频
							//如为对方结束，需应答
							if(len >= 57)
							  RecvTalkCallEnd_Func(buff, FromIP);
							else
							  LOGD("结束对讲数据长度异常\n");
							break;
						case FORCEIFRAME:    //强制I帧请求
							Local.ForceIFrame = 1;
							break;
						case ZOOMOUT:    //放大(720*240)
							if(len >= 57)
							  RecvTalkZoomOut_Func(buff, FromIP);
							else
							  LOGD("放大(720*240)数据长度异常");
							break;
						case ZOOMIN:    //缩小(352*240)
							if(len >= 57)
							  RecvTalkZoomIn_Func(buff, FromIP);
							else
							  LOGD("缩小(352*240)数据长度异常");
							break;
						case CALLUP: //通话上行
						case CALLDOWN: //通话下行
							RecvTalkCallUpDown_Func(buff, FromIP, len);
							break;
						default:
							break;
					}
					break;
				case VIDEOWATCH:     //局域网监控
				case VIDEOWATCHTRANS:  //局域网监控中转服务
					switch(buff[8])
					{
						case CALL:        //对方发起监视
							if(len >= 62)
							  RecvWatchCall_Func(buff, FromIP);
							else
							  LOGD("监视呼叫数据长度异常\n");
							break;
						case LINEUSE:        //对方正忙
							if(len >= 57)
							  RecvWatchLineUse_Func(buff, FromIP);
							else
							  LOGD("占线应答数据长度异常\n");
							break;
						case CALLANSWER:  //呼叫应答
							if(len >= 57)
							  RecvWatchCallAnswer_Func(buff, FromIP);
							else
							  LOGD("监视呼叫应答数据长度异常\n");
							break;
						case CALLCONFIRM:     //通话在线确认
							if(len >= 61)
							  RecvWatchCallConfirm_Func(buff, FromIP);
							else
							  LOGD("监视通话在线确认数据长度异常\n");
							break;
						case CALLEND:  //通话结束
							//结束播放视频
							//如为对方结束，需应答
							if(len >= 57)
							  RecvWatchCallEnd_Func(buff, FromIP);
							else
							  LOGD("结束监视数据长度异常\n");
							break;
						case FORCEIFRAME:    //强制I帧请求
							Local.ForceIFrame = 1;
							break;
						case ZOOMOUT:    //放大(720*240)
							if(len >= 57)
							  RecvWatchZoomOut_Func(buff, FromIP);
							else
							  LOGD("放大(720*240)数据长度异常");
							break;
						case ZOOMIN:    //缩小(352*240)
							if(len == 57)
							  RecvWatchZoomIn_Func(buff, FromIP);
							else
							  LOGD("缩小(352*240)数据长度异常");
							break;
						case CALLUP: //通话上行
						case CALLDOWN: //通话下行
							RecvWatchCallUpDown_Func(buff, FromIP, len);
							break;
						default:
							break;
					}
					break;
				/*case FINDEQUIP:       //查找设备
					if(len == 56)
					{
						RecvFindEquip_Func(buff, FromIP, len, m_DataSocket);
					}
					else
					{
#ifdef _DEBUG
						LOGD("查找设备应答数据长度异常\n");
#endif
					}
					break;
					*/
			}
		}


		if(strcmp(buff,"exit")==0)
		{
			LOGD("recvfrom888888888");
			UdpRecvFlag=0;
			//   break;
		}   
	}
}
//-----------------------------------------------------------------------
//查找设备
void RecvFindEquip_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int isAddrOK;
	i = 0;

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);
	if(recv_buf[7] == REPLY)   //应答
		for(i=0; i<UDPSENDMAX; i++)
			if(Multi_Udp_Buff[i].isValid == 1)
			  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
					if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
						if(Multi_Udp_Buff[i].buf[6] == FINDEQUIP)
							if(Multi_Udp_Buff[i].buf[7] == ASK)
								if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
								{
									Multi_Udp_Buff[i].isValid = 0;
									SendHostOrder(0x59, Local.DoorNo, Local.FindEquip); //查找设备成功
#ifdef _DEBUG
									LOGD("收到查找设备成功应答\n");
#endif
									break;
								}
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);
}
//---------------------------------------------------------------------------
//查找请求
void RecvNSAsk_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	unsigned char str[100];

	isAddrOK = 1;
	for(j=8; j<8+Local.AddrLen; j++)
	  if(LocalCfg.Addr[j-8] != recv_buf[j])
	  {
		  isAddrOK = 0;
		  break;
	  }

	//不是本机所发
	if(isAddrOK == 0)
	{
		isAddrOK = 1;
		for(j=32; j<32+Local.AddrLen; j++)
		{
#ifdef _DEBUG
			//LOGD("%c vs %c \n",LocalCfg.Addr[j-32],recv_buf[j]);
#endif
			if(LocalCfg.Addr[j-32] != recv_buf[j])
			{
				isAddrOK = 0;
				break;
			}
		}

		//要求查找的是本机地址
		if(isAddrOK == 1)
		{
#ifdef _DEBUG
			//LOGD("%c vs %c \n",LocalCfg.Addr[11],recv_buf[43]);
#endif
			if(recv_buf[43] != LocalCfg.Addr[11])
			  return;

			Local.DenNum = 0;   //副机数量
			memcpy(send_b, recv_buf, 32);
			send_b[7] = REPLY;    //应答

			send_b[32] = Local.DenNum + 1;   //地址个数

			memcpy(send_b + 33, LocalCfg.Addr, 20);
			memcpy(send_b + 53, LocalCfg.IP, 4);

			for(i=0; i<Local.DenNum; i++)
			{
				memcpy(send_b + 57 + 24*i, Local.DenAddr[i], 20);
				memcpy(send_b + 57 + 20 +24*i, Local.DenIP[i], 4);
			}

			sendlength = 57 + 24*Local.DenNum;

			UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);
		}
	}
}
//-----------------------------------------------------------------------
//查找应答
void RecvNSReply_Func(unsigned char *recv_buf, char *cFromIP, int m_Socket)
{
	int i,j, k;
	int CurrOrder;
	int isAddrOK;
	int DenNo;
	int AddrLen = 8;

	isAddrOK = 0;

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);

	for(i=0; i<UDPSENDMAX; i++)
	  if(Multi_Udp_Buff[i].isValid == 1)
		if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
		  if ((Multi_Udp_Buff[i].buf[6] == NSORDER) || (Multi_Udp_Buff[i].buf[6] == NSSERVERORDER))
			if((Multi_Udp_Buff[i].buf[7] == ASK)&&(recv_buf[32] > 0))
			{
#ifdef _DEBUG
				LOGD("NSR:CLEANBUF %d \n",i);
				LOGD("recv_buf[33]:%c::%d\n",recv_buf[33],recv_buf[33]);
#endif
				//判断本机地址是否匹配
				isAddrOK = 1;

#ifdef ISSETUPPACKETSIZE
				if (recv_buf[33] == 'S' || recv_buf[33] == 's')
#else
				if (recv_buf[33] == 'S')
#endif
                {
					AddrLen = 11;
                }

				for(j=32; j<32+AddrLen; j++)
				{
#ifdef ISSETUPPACKETSIZE
					if(recv_buf[33] == 's' && j == 32)
					  continue;
#endif
					if(Multi_Udp_Buff[i].buf[j] != recv_buf[j+1])
					{
						isAddrOK = 0;
						break;
					}
				}

				if (isAddrOK == 1)
				{
					CurrOrder = Multi_Udp_Buff[i].CurrOrder;
					Multi_Udp_Buff[i].isValid = 0;
					Multi_Udp_Buff[i].SendNum = 0;
					break;
				}
			}
#ifdef _DEBUG
	LOGD("isAddrOK:%d, TurnToCenter = %d\n", isAddrOK, TurnToCenter);
	//usleep(79*1000);
#endif

	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);

	if(isAddrOK == 1)
	{ 
        //收到正确的查找回应
        LOGD("\n%d: CurrOrder = %d\n", __LINE__, CurrOrder);
		switch(CurrOrder)
		{
			case FINDEQUIP:  //查找设备
				Remote.DenNum = recv_buf[32];
				if((Remote.DenNum >= 1)&&(Remote.DenNum <= 10))
				{
					for(j=0; j<Remote.DenNum; j++)
					{
						Remote.IP[j][0] = recv_buf[53+24*j];
						Remote.IP[j][1] = recv_buf[54+24*j];
						Remote.IP[j][2] = recv_buf[55+24*j];
						Remote.IP[j][3] = recv_buf[56+24*j];
						Remote.DenIP[0] = Remote.IP[j][0];
						Remote.DenIP[1] = Remote.IP[j][1];
						Remote.DenIP[2] = Remote.IP[j][2];
						Remote.DenIP[3] = Remote.IP[j][3];
						for(k=0; k<20; k++)
						  Remote.Addr[j][k] = recv_buf[33+24*j+k];
						Remote.GroupIP[0] = 236;
						Remote.GroupIP[1] = LocalCfg.IP[1];
						Remote.GroupIP[2] = LocalCfg.IP[2];
						Remote.GroupIP[3] = LocalCfg.IP[3];

						//锁定互斥锁
						pthread_mutex_lock (&Local.udp_lock);

						for(i=0; i<UDPSENDMAX; i++)
						{
							if(Multi_Udp_Buff[i].isValid == 0)
							{
								Multi_Udp_Buff[i].SendNum = 0;
								Multi_Udp_Buff[i].m_Socket = m_Socket;
								sLOGD(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
											Remote.DenIP[0],Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
#ifdef _DEBUG
								LOGD("Multi_Udp_Buff[%d].RemoteHost is %s\n",i,Multi_Udp_Buff[i].RemoteHost);
								LOGD("查找地址成功,正在呼叫\n");
#endif
								//头部
								memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
								//命令
								Multi_Udp_Buff[i].buf[6] = CurrOrder;
								Multi_Udp_Buff[i].buf[7] = ASK;    //主叫

								memcpy(Multi_Udp_Buff[i].buf+8,LocalCfg.Addr,20);
								memcpy(Multi_Udp_Buff[i].buf+28,LocalCfg.IP,4);
								memcpy(Multi_Udp_Buff[i].buf+32,Remote.Addr[j],20);
								memcpy(Multi_Udp_Buff[i].buf+52,Remote.IP[j],4);

								Multi_Udp_Buff[i].nlength = 56;
								Multi_Udp_Buff[i].DelayTime = 100;
								Multi_Udp_Buff[i].isValid = 1;
								sem_post(&multi_send_sem);
								break;
							}
						}
						//打开互斥锁
						pthread_mutex_unlock (&Local.udp_lock);
						break;
					}
				}
				break;                 
			case VIDEOTALK:  //对讲
				Remote.DenNum = recv_buf[32];
                LOGD("\n%d: Remote.DenNum = %d\n", __LINE__, Remote.DenNum);
				if((Remote.DenNum >= 1)&&(Remote.DenNum <= 10))
				{
					Remote.DenNum = 1; //单播
					for(j=0; j<Remote.DenNum; j++)
					{
						Remote.IP[j][0] = recv_buf[53+24*j];
						Remote.IP[j][1] = recv_buf[54+24*j];
						Remote.IP[j][2] = recv_buf[55+24*j];
						Remote.IP[j][3] = recv_buf[56+24*j];
						Remote.DenIP[0] = Remote.IP[j][0];
						Remote.DenIP[1] = Remote.IP[j][1];
						Remote.DenIP[2] = Remote.IP[j][2];
						Remote.DenIP[3] = Remote.IP[j][3];
						for(k=0; k<20; k++)
						  Remote.Addr[j][k] = recv_buf[33+24*j+k];
						Remote.GroupIP[0] = 236;
						Remote.GroupIP[1] = LocalCfg.IP[1];
						Remote.GroupIP[2] = LocalCfg.IP[2];
						Remote.GroupIP[3] = LocalCfg.IP[3];

						//锁定互斥锁
						pthread_mutex_lock (&Local.udp_lock);

						for(i=0; i<UDPSENDMAX; i++)
						{		
							if(Multi_Udp_Buff[i].isValid == 0)
							{
								Multi_Udp_Buff[i].SendNum = 0;
								Multi_Udp_Buff[i].m_Socket = m_Socket;
								//////////paul2.x	
								sLOGD(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",
											Remote.DenIP[0],Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);

								if ((Remote.Addr[0][0] == 'S' || Remote.Addr[0][0] == 's') && (TurnToCenter == 0))
                                {
                                    strcpy(Multi_Udp_Buff[i].RemoteHost, PriMulCast);
                                }
#ifdef _DEBUG
								LOGD("Multi_Udp_Buff[%d].RemoteHost is %s\n",i,Multi_Udp_Buff[i].RemoteHost);
								LOGD("查找地址成功,正在呼叫\n");
#endif
								//头部
								memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
								//命令
								Multi_Udp_Buff[i].buf[6] = CurrOrder;
								Multi_Udp_Buff[i].buf[7] = ASK;    //主叫
								// 子命令
								Multi_Udp_Buff[i].buf[8] = CALL;

								memcpy(Multi_Udp_Buff[i].buf+9,LocalCfg.Addr,20);
								memcpy(Multi_Udp_Buff[i].buf+29,LocalCfg.IP,4);
								memcpy(Multi_Udp_Buff[i].buf+33,Remote.Addr[j],20);
								memcpy(Multi_Udp_Buff[i].buf+53,Remote.IP[j],4);

								if (Remote.DenNum == 1)
                                {
                                    Multi_Udp_Buff[i].buf[57] = 0; //单播
                                }
								else
                                {
                                    Multi_Udp_Buff[i].buf[57] = 1; //组播
                                }

								//组播地址  
								Multi_Udp_Buff[i].buf[58] = Remote.GroupIP[0];
								Multi_Udp_Buff[i].buf[59] = Remote.GroupIP[1];
								Multi_Udp_Buff[i].buf[60] = Remote.GroupIP[2];
								Multi_Udp_Buff[i].buf[61] = Remote.GroupIP[3];

								Multi_Udp_Buff[i].nlength = 62;
								Multi_Udp_Buff[i].DelayTime = 800;
								Multi_Udp_Buff[i].isValid = 1;
								sem_post(&multi_send_sem);
								break;
							}
						}
						//打开互斥锁
						pthread_mutex_unlock (&Local.udp_lock);
						break;
					}
				}
				break;
			default:
				break;
		}
	}
}
//-----------------------------------------------------------------------
//监视呼叫
void RecvWatchCall_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	uint32_t Ip_Int;
	//本机状态为空闲
	////////////////paul0303////////////////////////////
	////////////////////////////////////////////////////////
	if(Local.Status == 0)
	{
		StopPlayWavFile();
		memcpy(send_b, recv_buf, 57);
		send_b[7]=ASK;    //主叫
		send_b[8]=CALLANSWER;//监视应答
#ifdef _VIDEOZOOMOUT
		//LOGD("recv_buf[8]:0x%02x\n",recv_buf[8]);
		//LOGD("recv_buf[9]:%c\n",recv_buf[9]);
		if(recv_buf[9] == 'S' || recv_buf[9] == 's')
		  send_b[26] = 0x0F;
#endif
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);

		//获取对方地址
		memcpy(Remote.Addr[0], recv_buf+9, 20);
		memcpy(Remote.IP[0], recv_buf+29, 4);

		Ip_Int=inet_addr(cFromIP);
		memcpy(Remote.DenIP, &Ip_Int,4);

#ifdef _DEBUG
		LOGD("Remote.DenIP, %d.%d.%d.%d\0",
				Remote.DenIP[0],Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
#endif
		if((Remote.DenIP[0] == Remote.IP[0][0]) && (Remote.DenIP[1] == Remote.IP[0][1])
				&& (Remote.DenIP[2] == Remote.IP[0][2]) &&(Remote.DenIP[3] == Remote.IP[0][3]))
		{
			Remote.isDirect = 0;
#ifdef _DEBUG
			LOGD("对方发起直通监视呼叫\n");
#endif
		}
		else
		{
			Remote.isDirect = 1;
#ifdef _DEBUG
			LOGD("对方发起中转监视呼叫\n");
#endif
		}
		Local.Status = 4;  //状态为被监视
		//开始录制视频
#ifdef _VIDEOZOOMOUT
		LOGD("recv_buf[26]:0x%02x\n",recv_buf[26]);
		if(recv_buf[9] == 'S' || recv_buf[9] == 's')
		{
			//StartRecVideo(D1_W, D1_H);
			Local.OpenCIFVideoFlag = 0;
		}
		else
		{
			//StartRecVideo(CIF_W, CIF_H);
			Local.OpenCIFVideoFlag = 1;
		}
#else
		//StartRecVideo(CIF_W, CIF_H);
#endif
		Local.CallConfirmFlag = 1; //设置在线标志
		Local.Timer1Num = 0;
		Local.TimeOut = 0;       //监视超时,  通话超时,  呼叫超时，无人接听
		Local.OnlineNum = 0;     //在线确认序号
		Local.OnlineFlag = 1;   
		//doorsLocal.DoorNo = 0x31;
		
		SendHostOrder(0x60, Local.DoorNo, NULL); //发送主动命令  对方监视  暂定为1号
		//SendHostOrder(0x60, 0x31, NULL); //发送主动命令  对方监视  暂定为1号
	}
	//本机为忙
	else
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7]=ASK;    //主叫
		send_b[8]=LINEUSE;//占线应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
#ifdef _DEBUG
		LOGD("对方发起监视呼叫\n");
#endif
	}
}
//-----------------------------------------------------------------------
//监视占线应答
void RecvWatchLineUse_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);
	if(recv_buf[7] == ASK)   //应答
		for(i=0; i<UDPSENDMAX; i++)
			if(Multi_Udp_Buff[i].isValid == 1)
				if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
					if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
						if((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)||(Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
							if(Multi_Udp_Buff[i].buf[7] == ASK)
								if(Multi_Udp_Buff[i].buf[8] == CALL)
									if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										LOGD("WATCHLINEUSE STATUS=0\n");
										Local.Status = 0;  //状态设为空闲
#ifdef _DEBUG
										LOGD("收到监视占线应答\n");
#endif
										break;
									}
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);
}
//-----------------------------------------------------------------------
//监视呼叫应答
void RecvWatchCallAnswer_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);
	if(recv_buf[7] == ASK)   //应答
		for(i=0; i<UDPSENDMAX; i++)
			if(Multi_Udp_Buff[i].isValid == 1)
			  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
					if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
						if((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)||(Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
							if(Multi_Udp_Buff[i].buf[7] == ASK)
								if(Multi_Udp_Buff[i].buf[8] == CALL)
									if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										//开始播放视频
										//StartPlayVideo(CIF_W, CIF_H);

										Local.CallConfirmFlag = 1; //设置在线标志
										Local.Timer1Num = 0;
										Local.TimeOut = 0;       //监视超时,  通话超时,  呼叫超时，无人接听
										Local.OnlineNum = 0;     //在线确认序号
										Local.OnlineFlag = 1;

										Local.Status = 3;  //状态为监视
#ifdef _DEBUG
										LOGD("收到监视呼叫应答\n");
#endif
										break;
									}
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);
}
//-----------------------------------------------------------------------
//监视在线确认
void RecvWatchCallConfirm_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	//本机被动    被监视
	if((Local.Status == 4)&&(recv_buf[7] == ASK))
	{
		memcpy(send_b, recv_buf, 61);
		send_b[7]=REPLY;    //应答
		sendlength=61;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
		Local.CallConfirmFlag = 1;
#ifdef _DEBUG
		//   LOGD("收到监视在线确认\n");
#endif
	}
	else  //本机监视
		if(Local.Status == 3)
		{
			Local.CallConfirmFlag = 1;
#ifdef _DEBUG
			//    LOGD("收到对方应答本机监视在线确认\n");
#endif
		}
}
//-----------------------------------------------------------------------
//监视呼叫结束
void RecvWatchCallEnd_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	//本机被动    被监视
	if((Local.Status == 4)&&(recv_buf[7] == ASK))
	{
		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //设置在线标志
		memcpy(send_b, recv_buf, 57);
		send_b[7]=REPLY;    //应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);

		//    CloseVideoWindow();    //关闭视频窗口
		StopRecVideo();
		LOGD("RECWATCHCALLEND STATUS=0\n");
		Local.Status = 0;  //状态为空闲
		//监视结束
		SendHostOrder(0x61, Local.DoorNo, NULL); //发送主动命令  对方停止监视
#ifdef _DEBUG
		LOGD("对方结束监视\n");
#endif
	}
	else  //本机监视
		if(Local.Status == 4)
		{
			Local.OnlineFlag = 0;
			Local.CallConfirmFlag = 0; //设置在线标志
			//锁定互斥锁
			pthread_mutex_lock (&Local.udp_lock);
			//本机主动
			if(recv_buf[7] == REPLY)
				for(i=0; i<UDPSENDMAX; i++)
					if(Multi_Udp_Buff[i].isValid == 1)
					  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
							if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
								if((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)||(Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
									if(Multi_Udp_Buff[i].buf[7] == ASK)
										if(Multi_Udp_Buff[i].buf[8] == CALLEND)
											if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
											{
												Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
												LOGD("对方应答本机结束监视\n");
#endif
												break;
											}
			//打开互斥锁
			pthread_mutex_unlock (&Local.udp_lock);
		}
}
//-----------------------------------------------------------------------
//放大(720*480)
void RecvWatchZoomOut_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	//本机被动    被监视
	if((Local.Status == 4)&&(recv_buf[7] == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7]=REPLY;    //应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
		StopRecVideo();   //352*240
		StartRecVideo(D1_W, D1_H);  //720*240
#ifdef _DEBUG
		LOGD("对方放大图像\n");
#endif
	}
	else  //本机监视
		if(Local.Status == 3)
		{
			//锁定互斥锁
			pthread_mutex_lock (&Local.udp_lock);
			//本机主动
			if(recv_buf[7] == REPLY)
				for(i=0; i<UDPSENDMAX; i++)
					if(Multi_Udp_Buff[i].isValid == 1)
					  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
							if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
								if((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)||(Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
									if(Multi_Udp_Buff[i].buf[7] == ASK)
										if(Multi_Udp_Buff[i].buf[8] == ZOOMOUT)
											if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
											{
												Multi_Udp_Buff[i].isValid = 0;
												//StopPlayVideo();   //352*240
												//StartPlayVideo(720, 480);  //720*480
#ifdef _DEBUG
												LOGD("对方应答本机放大图像\n");
#endif
												break;
											}
			//打开互斥锁
			pthread_mutex_unlock (&Local.udp_lock);
		}
}
//-----------------------------------------------------------------------
//缩小(352*240)
void RecvWatchZoomIn_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	//本机被动    被监视
	if((Local.Status == 4)&&(recv_buf[7] == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7]=REPLY;    //应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
		StopRecVideo();   //720*480
		StartRecVideo(CIF_W, CIF_H);  //352*288
#ifdef _DEBUG
		LOGD("对方缩小图像\n");
#endif
	}
	else  //本机监视
		if(Local.Status == 3)
		{
			//锁定互斥锁
			pthread_mutex_lock (&Local.udp_lock);
			//本机主动
			if(recv_buf[7] == REPLY)
				for(i=0; i<UDPSENDMAX; i++)
					if(Multi_Udp_Buff[i].isValid == 1)
					  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
							if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
								if((Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)||(Multi_Udp_Buff[i].buf[6] == VIDEOWATCHTRANS))
									if(Multi_Udp_Buff[i].buf[7] == ASK)
										if(Multi_Udp_Buff[i].buf[8] == ZOOMIN)
											if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
											{
												Multi_Udp_Buff[i].isValid = 0;
												//StopPlayVideo();   //720*480
												//StartPlayVideo(CIF_W, CIF_H);  //352*240
#ifdef _DEBUG
												LOGD("对方应答本机缩小图像\n");
#endif
												break;
											}
			//打开互斥锁
			pthread_mutex_unlock (&Local.udp_lock);
		}
}
//-----------------------------------------------------------------------
//监视数据
void RecvWatchCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length)
{

}
//-----------------------------------------------------------------------
//对讲呼叫
void RecvTalkCall_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	uint32_t Ip_Int;
	char str[100];
	char IP_Group[15];
	char wavFile[50];
	//本机状态为空闲
	if(Local.Status == 0)
	{
		memcpy(send_b, recv_buf, 62);
		send_b[7]=ASK;    //主叫
		send_b[8]=CALLANSWER;//对讲应答
		sendlength=62;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);

		//获取对方地址
		memcpy(Remote.Addr[0], recv_buf+9, 20);
		memcpy(Remote.IP[0], recv_buf+29, 4);

		if(recv_buf[57] == 1)
		{
			//查看是否在其它组播组内
			DropMultiGroup(m_VideoSocket, NULL);

			Local.IP_Group[0] = recv_buf[58]; //组播地址
			Local.IP_Group[1] = recv_buf[59];
			Local.IP_Group[2] = recv_buf[60];
			Local.IP_Group[3] = recv_buf[61];
			sLOGD(IP_Group, "%d.%d.%d.%d\0",
					Local.IP_Group[0],Local.IP_Group[1],Local.IP_Group[2],Local.IP_Group[3]);
			AddMultiGroup(m_VideoSocket, IP_Group);
		}

		Ip_Int=inet_addr(cFromIP);
		memcpy(Remote.DenIP, &Ip_Int,4);
#ifdef _DEBUG
		LOGD("Remote.DenIP, %d.%d.%d.%d\0",
				Remote.DenIP[0],Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
#endif
		Local.Status = 2;  //状态为被对讲
		//显示对讲图像窗口（二级）
		// DisplayTalkPicWindow();
		//开始播放视频
		//  StartPlayVideo(CIF_W, CIF_H);
		Local.CallConfirmFlag = 1; //设置在线标志
		Local.Timer1Num = 0;
		Local.TimeOut = 0;       //监视超时,  通话超时,  呼叫超时，无人接听
		Local.OnlineNum = 0;     //在线确认序号
		Local.OnlineFlag = 1;
		////////////////////////
	//	sLOGD(wavFile,"%s2.wav\0","/mnt/mtd/");
		sLOGD(wavFile,"%sasong.wav\0","/mnt/");
		StartPlayWav(wavFile,1);
		///////////////////////////////////
		if((Remote.DenIP[0] == Remote.IP[0][0]) && (Remote.DenIP[1] == Remote.IP[0][1])
				&& (Remote.DenIP[2] == Remote.IP[0][2]) &&(Remote.DenIP[3] == Remote.IP[0][3]))
		{
			Remote.isDirect = 0;
			strcpy(str, Remote.Addr[0]);
			strcat(str, "  直通对讲呼叫");
			//     ShowStatusText(50, 380 , 3, cBlack, 1, 1, str, 0);
#ifdef _DEBUG
			LOGD("%s\n", str);
#endif
		}
		else
		{
			Remote.isDirect = 1;
			strcpy(str, Remote.Addr[0]);
			strcat(str, "  中转对讲呼叫");
			//    ShowStatusText(50, 380 , 3, cBlack, 1, 1, str, 0);
#ifdef _DEBUG
			LOGD("%s\n", str);
#endif
		}    
	}
	//本机为忙
	else
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7]=ASK;    //主叫
		send_b[8]=LINEUSE;//占线应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
#ifdef _DEBUG
		LOGD("对方发起对讲呼叫\n");
#endif
	}

}
//-----------------------------------------------------------------------
//对讲占线应答
void RecvTalkLineUse_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);
	if(recv_buf[7] == ASK)   //应答
		for(i=0; i<UDPSENDMAX; i++)
			if(Multi_Udp_Buff[i].isValid == 1)
				if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
					if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
						if((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)||(Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
							if(Multi_Udp_Buff[i].buf[7] == ASK)
								if(Multi_Udp_Buff[i].buf[8] == CALL)
						//			if(memcmp(recv_buf+33, Remote.Addr[0],11) == 0)
					/////paul2.x    if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										if(Remote.DenNum == 1)
											Local.Status = 0;  //设状态为空闲
										LOGD("TALKLINEUSE%d STATUS=0,FromIP:%s\n",i,cFromIP);
										SendHostOrder(0x64, Local.DoorNo, NULL); //发送主动命令  对方正忙
										//zhou101102
										//SendTalkInfo.Status = 0x02;//对方有应答且占线
										//SendTalkInfo.Duration = 0;//通话时间；
										//	if(Local.Status %2 !=0)
											{
												//if(inputbuff[4] == 0x31)
												SendTalkInfo.Status = 0x02;//0x06 call 0x07 pwd
												//else
												//		SendTalkInfo.Status = 0x06;//0x06 call 0x07 pwd
												SendTalkInfo.Duration =Local.TimeOut;
												if(Remote.Addr[0][0] != 'Z') 
												  memcpy(SendTalkInfo.Addr , Remote.Addr , 12);
												//	memcpy(SendTalkInfo.Addr + 7, inputbuff + 5, 4);

												/*sLOGD(SendTalkInfo.StartTime,"%04d%02d%02d%02d%02d%02d",
												  Local.call_tm_t->tm_year+1900,
												  Local.call_tm_t->tm_mon + 1,
												  Local.call_tm_t->tm_mday,
												  Local.call_tm_t->tm_hour,
												  Local.call_tm_t->tm_min,
												  Local.call_tm_t->tm_sec);
												  SendTalkInfo.StartTime[14] = 0x30;
												  SendTalkInfo.StartTime[15] = '\0';
												*/
												if(Remote.Addr[0][0] != 'Z') 
													SendTalkInfoFlag = 1;
											}


#ifdef _DEBUG
										LOGD("收到对讲占线应答\n");
#endif
										break;
									}
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);
}
//-----------------------------------------------------------------------
//对讲呼叫应答
void RecvTalkCallAnswer_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	uint32_t Ip_Int;
	char wavFile[80];    
	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);
	if(recv_buf[7] == ASK)   //应答
	  for(i=0; i<UDPSENDMAX; i++)
		if(Multi_Udp_Buff[i].isValid == 1)
		  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
			if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
			  if((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)||(Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
				if(Multi_Udp_Buff[i].buf[7] == ASK)
				  if(Multi_Udp_Buff[i].buf[8] == CALL)
				  {
					  Multi_Udp_Buff[i].isValid = 0;
					  memcpy(Remote.Addr[0], recv_buf+33, 20);
					  memcpy(Remote.IP[0], recv_buf+53, 4);
#ifdef _DEBUG
					  LOGD("callanswer(addr:%c) (buf%d) FromIP:%s\n",Remote.Addr[0][0],i,cFromIP);
#endif
					  Ip_Int=inet_addr(cFromIP);
					  memcpy(Remote.DenIP, &Ip_Int,4);
					  if((LocalCfg.Addr[0] == 'S')||(LocalCfg.Addr[0] == 'B')||(LocalCfg.Addr[0] == 'Z')
								  ||(Remote.DenNum == 1))
					  {
						  Ip_Int=inet_addr(cFromIP);
						  memcpy(Remote.DenIP, &Ip_Int,4);
					  }
					  else
					  {
						  if(Remote.DenNum > 1)
						  {
							  Remote.DenIP[0] = Remote.GroupIP[0];
							  Remote.DenIP[1] = Remote.GroupIP[1];
							  Remote.DenIP[2] = Remote.GroupIP[2];
							  Remote.DenIP[3] = Remote.GroupIP[3];
						  }
					  }
#ifdef _VIDEOZOOMOUT
					  LOGD("recv_buf:  0x%2x,LocalCfg.Addr[0]:%c\n",recv_buf[26],LocalCfg.Addr[0]);
					  if((LocalCfg.Addr[0] == 'M') && (recv_buf[26] == 0x0F))
						Local.videozoomout = 1;
					  else
						Local.videozoomout = 0;
					  LOGD("Local.videozoomout:%d\n",Local.videozoomout);
#endif
					  if(Local.Status != 1)
						SendHostOrder(0x65, Local.DoorNo, NULL); //发送主动命令  对方接通
#ifdef _DEBUG
					  LOGD("收到对讲呼叫应答\n");
#endif
					  break;
				  }
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);
}
//-----------------------------------------------------------------------
//对讲开始通话  由被叫方发起，主叫方应答
void RecvTalkCallStart_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j,k;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	uint32_t Ip_Int;
	//本机为主叫方 应答
	if((Local.Status == 1)&&(recv_buf[7] == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7]=REPLY;    //应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);

		//获取被叫方地址
		memcpy(Remote.Addr[0], recv_buf+33, 20);
		memcpy(Remote.IP[0], recv_buf+53, 4);
		Remote.DenNum = 1;

		Ip_Int=inet_addr(cFromIP);
		memcpy(Remote.DenIP, &Ip_Int,4);
		
		//LOGD("Local.DoorNo %d\n",Local.DoorNo);
		///////////////////paul0302night+0303+0311+0312////////////
		StopPlayWavFile();  //关闭回铃音
		/*	
		for(i=0; i<4; i++)
		{
			if(TalkDen.ExistFlag[i] == 1)
			{
				if(strcmp(Remote.Addr[0], TalkDen.Addr[i]) != 0)  
				{
					TalkDen.ExistFlag[i] = 0;
					//新加一条子命令， 25  某一方接听后，挂断其它的
					CallEndOther(i);
					LOGD("CALLENDOTHER %d\n",i);
				} 
			}
		}
		*/
		//选通音频 ,高电平有效
		//doorsLocal.DoorNo = 0x31;
		if((Local.DoorNo >= 0x31)&&(Local.DoorNo <= 0x38))
		{
			////////////////////paul0312////////////////////////////////
			SendHostOrder(0x67, Local.DoorNo, NULL); //发送主动命令  对方接听
			//if((Remote.Addr[0][0]!='M')&&(Remote.Addr[0][0]!='D')&&(Remote.Addr[0][0]!='W'))
			//StartRecVideo(CIF_W,CIF_H);
			if(recv_buf[28]==0x13)
			{
				Local.leftmessage = 0x01;
				SendTalkInfo.Status = 0x04;//0x06 call 0x07 pwd
			}
			else
			{
				Local.leftmessage = 0x00;
				SendTalkInfo.Status = 0x05;//0x06 call 0x07 pwd
			}

#if 0
			StartPlayAudio();
			StartRecAudio();
#endif
			
			Local.Status = 5;
			Local.TimeOut= 0;
			LOGD("Video and Audio Start~ %d\n",Local.leftmessage);
		}
		else
			LOGD("开始通话 Local.DoorNo = 0x%X, 异常\n", Local.DoorNo);

#ifdef _DEBUG
		LOGD("对方开始通话\n");
#endif
		//   ShowStatusText(50, 380 , 3, cBlack, 1, 1, "开始通话", 0);
	}
	else  //本机为被叫方 发起
		if(Local.Status == 2)
		{
			//锁定互斥锁
			pthread_mutex_lock (&Local.udp_lock);
			//本机主动
			if(recv_buf[7] == REPLY)
				for(i=0; i<UDPSENDMAX; i++)
					if(Multi_Udp_Buff[i].isValid == 1)
					  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
							if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
								if((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)||(Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
									if(Multi_Udp_Buff[i].buf[7] == ASK)
										if(Multi_Udp_Buff[i].buf[8] == CALLSTART)
											if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
											{
												//查看是否在其它组播组内
												DropMultiGroup(m_VideoSocket, NULL);

												Multi_Udp_Buff[i].isValid = 0;
												//打开音频录制、播放，视频录制
												StartRecVideo(CIF_W, CIF_H);
												StartRecAudio();
												StartPlayAudio();
												Local.Status = 6;  //状态为被叫通话
												Local.TimeOut = 0;       //监视超时,  通话超时,  呼叫超时，无人接听

#ifdef _DEBUG
												LOGD("对方应答本机开始通话\n");
#endif
												//       ShowStatusText(50, 380 , 3, cBlack, 1, 1, "开始通话", 0);
												break;
											}
			//打开互斥锁
			pthread_mutex_unlock (&Local.udp_lock);
		}
}
//-----------------------------------------------------------------------
//对讲在线确认
void RecvTalkCallConfirm_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	//本机为主叫方
	if(((Local.Status == 1)||(Local.Status == 5)||(Local.Status == 7)||(Local.Status == 9))
			&&(recv_buf[7] == ASK))
	{
		memcpy(send_b, recv_buf, 61);
		send_b[7]=REPLY;    //应答
		sendlength=61;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
		Local.CallConfirmFlag = 1;
#ifdef _DEBUG
		//   LOGD("收到对讲在线确认\n");
#endif
	}
	else  //本机为被叫方
		if(((Local.Status == 2)||(Local.Status == 6)||(Local.Status == 8)||(Local.Status == 10))
				&&(recv_buf[7] == REPLY))
		{
			Local.CallConfirmFlag = 1;
#ifdef _DEBUG
			//    LOGD("收到对方应答本机对讲在线确认\n");
#endif
		}
}
//-----------------------------------------------------------------------
//对讲远程开锁
void RecvTalkOpenLock_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;
	static int times=0;

	//本机被动
	//  #ifdef _ZHUHAIJINZHEN      //珠海进祯  呼叫时可开锁
	if(((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6))
			&&(recv_buf[7] == ASK))
		//  #else
		//  if((/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)||(Local.Status == 6))
		//     &&(recv_buf[7] == ASK))
		//  #endif
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7]=REPLY;    //应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);

		SendHostOrder(0x5B, Local.DoorNo, NULL); //发送主动命令  对方开锁
		times++;
#ifdef _DEBUG
		LOGD("\nRecvTalkOpenLock %d\n\n",times);
#endif
	}
}
//-----------------------------------------------------------------------
//对讲呼叫结束
void RecvTalkCallEnd_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	LOGD("CALLEND:%c%c,%d\n",recv_buf[9],recv_buf[33],Local.Status);
	//本机被动
	if(((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
				||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))
			&&(recv_buf[7] == ASK))
	{
		if(Local.leftmessage ==0x01)
			Local.leftmessage = 0x02;

		Local.OnlineFlag = 0;
		Local.CallConfirmFlag = 0; //设置在线标志

		memcpy(send_b, recv_buf, 57);
		send_b[7]=REPLY;    //应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
		if(Local.Status %2 !=0)
		{
			//if(inputbuff[4] == 0x31)
			if(Local.Status == 0x01)
				SendTalkInfo.Status = 0x03;//0x06 call 0x07 pwd
			SendTalkInfo.Duration =Local.TimeOut;

			SendTalkInfoFlag = 1;
			LOGD("CALLEND SENDING INFO:status %d\n",SendTalkInfo.Status);
		}

#ifdef _LXC_END_WAIT
		Local.WaitforEnd = 1;
		Local.WaitTimeOut = 0;
#endif
		TalkEnd_ClearStatus();
		SendHostOrder(0x68, Local.DoorNo, NULL); //发送主动命令  对方挂机  
		///////////paul2.x

#ifdef _DEBUG
		LOGD("对方结束对讲\n");
#endif
	}
	else  //本机主动
		if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
				||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))
		{
			//		Local.OnlineFlag = 0;
			//		Local.CallConfirmFlag = 0; //设置在线标志

		//锁定互斥锁
		pthread_mutex_lock (&Local.udp_lock);
		//本机主动
		if(recv_buf[7] == REPLY)
			for(i=0; i<UDPSENDMAX; i++)
				if(Multi_Udp_Buff[i].isValid == 1)
				  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
						if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
							if((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)||(Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
								if(Multi_Udp_Buff[i].buf[7] == ASK)
									if(Multi_Udp_Buff[i].buf[8] == CALLEND)
										//if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
										{
											LOGD("Remote.Addr[0]:%X:::%c:|||:recv_buf[33]:%X::%c\n",Remote.Addr[0][0],Remote.Addr[0][0],recv_buf[33],recv_buf[33]);
											if((Local.Status==1)&&(memcmp(Remote.Addr[0],recv_buf+33,11)!=0))
												break;
											if((Local.Status>=5)&&(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) != 0))								
												break;

											Multi_Udp_Buff[i].isValid = 0;
											Local.OnlineFlag = 0;
											Local.CallConfirmFlag = 0; //设置在线标志
											if(Local.leftmessage ==0x01)
												Local.leftmessage = 0x02;
											//     ShowStatusText(50, 380 , 3, cBlack, 1, 1, "本机结束对讲", 0);
											if(Local.Status %2 !=0)
											{
												//if(inputbuff[4] == 0x31)
												if(Local.Status == 0x01)
													SendTalkInfo.Status = 0x03;//0x06 call 0x07 pwd

												SendTalkInfo.Duration =Local.TimeOut;

												/*sLOGD(SendTalkInfo.StartTime,"%04d%02d%02d%02d%02d%02d",
												  Local.call_tm_t->tm_year+1900,
												  Local.call_tm_t->tm_mon + 1,
												  Local.call_tm_t->tm_mday,
												  Local.call_tm_t->tm_hour,
												  Local.call_tm_t->tm_min,
												  Local.call_tm_t->tm_sec);
												  SendTalkInfo.StartTime[14] = 0x30;
												  SendTalkInfo.StartTime[15] = '\0';
												*/
												SendTalkInfoFlag = 1;
												LOGD("CALLEND SENDING INFO:status %d\n",SendTalkInfo.Status);
											}

#ifdef _LXC_END_WAIT
											Local.WaitforEnd = 1;
											Local.WaitTimeOut = 0;
#endif
											TalkEnd_ClearStatus();

#ifdef _DEBUG
											LOGD("对方应答本机结束对讲\n");
#endif
											break;
										}
		//打开互斥锁
		pthread_mutex_unlock (&Local.udp_lock);
	}
}
//-----------------------------------------------------------------------
//对讲结束，清状态和关闭音视频
void TalkEnd_ClearStatus(void)
{
	LOGD("TALKENDCLEARStatus=%d\n", Local.Status);
	//查看是否在其它组播组内
	DropMultiGroup(m_VideoSocket, NULL);
	switch(Local.Status)
	{
		case 1: //本机主叫
			StopRecVideo();
			StopPlayWavFile();
			Local.Status = 0;  //状态为空闲
            TurnToCenter = 0;
			//zhou101102	
			//SendTalkInfo.Status = 0x03;
			//SendTalkInfo.Duration = 0;

			break;
		case 2: //本机被叫
			Local.Status = 0;  //状态为空闲
            TurnToCenter = 0;
			StopPlayWavFile();
			break;
		case 5: //本机主叫通话
			StopRecVideo();
			//   StopPlayVideo();
			StopRecAudio();
			
			StopPlayAudio();

			Local.Status = 0;  //状态为空闲
            TurnToCenter = 0;
			//zhou101102
			LOGD("Local.TimeOut is %d\n,Second is %d\n",Local.TimeOut,(Local.TimeOut/INTRPERSEC)%60);
			//SendTalkInfo.Status = 0x05;
			//SendTalkInfo.Duration = (Local.TimeOut/INTRPERSEC)%60;
			break;
		case 6: //本机被叫通话
			LOGD("stop1-\n");
			StopRecVideo();
			LOGD("stop2-\n");
			//     StopPlayVideo();
			StopRecAudio();
			LOGD("stop3-\n");
			StopPlayAudio();

			Local.Status = 0;  //状态为空闲
            TurnToCenter = 0;
			break;
	}
	

}
//-----------------------------------------------------------------------
//放大(720*480)
void RecvTalkZoomOut_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	//本机被动
	if(((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6))
			&&(recv_buf[7] == ASK)) {
		memcpy(send_b, recv_buf, 57);
		send_b[7]=REPLY;    //应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
		StopRecVideo();   //352*240
		Local.OpenD1VideoFlag = 1;
#ifdef _DEBUG
		LOGD("对方放大图像\n");
#endif
    } else { 
        //本机主动
        if ((Local.Status == 5)||(Local.Status == 6)) {
            //锁定互斥锁
            pthread_mutex_lock (&Local.udp_lock);
            //本机主动
            if(recv_buf[7] == REPLY)
                for(i=0; i<UDPSENDMAX; i++)
                    if(Multi_Udp_Buff[i].isValid == 1)
                        if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
                            if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
                                if((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)||(Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
                                    if(Multi_Udp_Buff[i].buf[7] == ASK)
                                        if(Multi_Udp_Buff[i].buf[8] == ZOOMOUT)
                                            if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
                                            {
                                                Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                                LOGD("对方应答本机放大图像\n");
#endif
                                                break;
                                            }
            //打开互斥锁
            pthread_mutex_unlock (&Local.udp_lock);
        }
    }
}
//-----------------------------------------------------------------------
//缩小(352*288)
void RecvTalkZoomIn_Func(unsigned char *recv_buf, char *cFromIP)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	//本机被动
	if((/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)||(Local.Status == 6))
			&&(recv_buf[7] == ASK))
	{
		memcpy(send_b, recv_buf, 57);
		send_b[7]=REPLY;    //应答
		sendlength=57;
		UdpSendBuff(m_VideoSendSocket, cFromIP, send_b , sendlength);
		StopRecVideo();   //720*480
		StartRecVideo(CIF_W, CIF_H);  //352*288
#ifdef _DEBUG
		LOGD("对方缩小图像\n");
#endif
	}
	else  //本机主动
		if(/*(Local.Status == 1)||(Local.Status == 2)||*/(Local.Status == 5)||(Local.Status == 6))
		{
			//锁定互斥锁
			pthread_mutex_lock (&Local.udp_lock);
			//本机主动
			if(recv_buf[7] == REPLY)
				for(i=0; i<UDPSENDMAX; i++)
					if(Multi_Udp_Buff[i].isValid == 1)
					  if((Multi_Udp_Buff[i].m_Socket == m_VideoSocket) || (Multi_Udp_Buff[i].m_Socket == m_VideoSendSocket))
							if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
								if((Multi_Udp_Buff[i].buf[6] == VIDEOTALK)||(Multi_Udp_Buff[i].buf[6] == VIDEOTALKTRANS))
									if(Multi_Udp_Buff[i].buf[7] == ASK)
										if(Multi_Udp_Buff[i].buf[8] == ZOOMIN)
											if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
											{
												Multi_Udp_Buff[i].isValid = 0;
												//   StopPlayVideo();   //720*480
												//   StartPlayVideo(CIF_W, CIF_H);  //352*288
#ifdef _DEBUG
												LOGD("对方应答本机缩小图像\n");
#endif
												break;
											}
			//打开互斥锁
			pthread_mutex_unlock (&Local.udp_lock);
		}
}

extern int devplayfd;
unsigned char test_audio_buf[AUDIOBLK];
int old_frameno = 0;
//对讲数据
void RecvTalkCallUpDown_Func(unsigned char *recv_buf, char *cFromIP, int length)
{
#if 0
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	short PackIsExist; //数据包已接收标志
	TempAudioNode1 * tmp_audionode;
	int isFull;
	struct talkdata1 talkdata;
	
	isAddrOK = memcmp(recv_buf + 9, LocalCfg.Addr, 11);
	if(isAddrOK != 0)
		isAddrOK = memcmp(recv_buf + 33, LocalCfg.Addr, 11);
	if(isAddrOK != 0)
		return;
	if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
			||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))  //状态为对讲
	{
		switch(recv_buf[61])
		{
			case 1://音频
                {
                    //帧序号
                    //       if(AudioMuteFlag == 0)
                    //写留影数据函数, 数据  长度  类型 1--音频  2--视频I  3--视频P
                    if((Local.Status == 5)||(Local.Status == 6)||(Local.Status == 10))
                    {
                        /* 接收到后直接播放 */
#if 0
                        //音频播放没有打开
                        if (devplayfd <= 0) {
                            break;
                        }
                        memcpy(&talkdata, recv_buf + 9, sizeof(talkdata));
                        if ((old_frameno + 1) != talkdata.Frameno) {
                            LOGD("talkdata.Frameno = %d, old_frameno = %d, length = %d\n", talkdata.Frameno, old_frameno, length);
                        }
                        old_frameno = talkdata.Frameno;
#if 0
                        /* G711格式 */
                        G711Decoder(test_audio_buf, recv_buf + DeltaLen, AUDIOBLK/2,1);
                        Play(test_audio_buf, AUDIOBLK);
#else
                        /* PCM格式 */
                        Play((recv_buf + DeltaLen), AUDIOBLK);
#endif
                        break;
#else
                        memcpy(&talkdata, recv_buf + 9, sizeof(talkdata));
                        if(temp_audio_n >= G711NUM)
                        {
                            temp_audio_n = G711NUM;
#ifdef _DEBUG
                            LOGD("temp_audio is full\n");
#endif
                        }
                        else
                        {
                            tmp_audionode = (TempAudioNode1 *)find_audionode(TempAudioNode_h, talkdata.Frameno, talkdata.CurrPackage);

                            if(tmp_audionode == NULL)
                            {
                                isFull = creat_audionode(TempAudioNode_h, talkdata, recv_buf, length);
                                PackIsExist = 0;
                            }
                            else
                                PackIsExist = 1;

                            if(PackIsExist == 0)
                            {
                                TimeStamp.OldCurrAudio = TimeStamp.CurrAudio; //上一次当前音频时间
                                TimeStamp.CurrAudio = talkdata.timestamp;
                                //  temp_audio_n ++;
                                temp_audio_n = length_audionode(TempAudioNode_h);
                                if(temp_audio_n >= 4) //VPLAYNUM/2 4帧 128ms
                                {
                                    sem_post(&audiorec2playsem);
                                }
                            }
                        }
#endif
                    } 
                }
				break;
			case 2://视频  I帧  352*288
			case 3://视频  P帧  352*288
			case 4://视频  I帧  720*480
			case 5://视频  P帧  720*480
				//LOGD("video data recv\n");
				break;
			default:
				break;
		}
	}
#endif
}
//-----------------------------------------------------------------------
void ForceIFrame_Func(void)  //强制I帧
{
	int i;

	for(i=0; i<UDPSENDMAX; i++)
		if(Multi_Udp_Buff[i].isValid == 0)
		{
			//锁定互斥锁
			pthread_mutex_lock (&Local.udp_lock);
			//只发送一次
			Multi_Udp_Buff[i].SendNum = 5;
			Multi_Udp_Buff[i].m_Socket = m_VideoSendSocket;
			Multi_Udp_Buff[i].CurrOrder = 0;
			sLOGD(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",Remote.DenIP[0],
					Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
			//头部
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			//命令
			if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
					||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))
				Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
			if((Local.Status == 3)||(Local.Status == 4))
				Multi_Udp_Buff[i].buf[6] = VIDEOWATCH;
			Multi_Udp_Buff[i].buf[7] = ASK;    //主叫
			Multi_Udp_Buff[i].buf[8] = FORCEIFRAME;    //FORCEIFRAME

			memcpy(Multi_Udp_Buff[i].buf+9,LocalCfg.Addr,20);
			memcpy(Multi_Udp_Buff[i].buf+29,LocalCfg.IP,4);
			memcpy(Multi_Udp_Buff[i].buf+33,Remote.Addr[0],20);
			memcpy(Multi_Udp_Buff[i].buf+53,Remote.IP[0],4);

			Multi_Udp_Buff[i].nlength = 57;
			Multi_Udp_Buff[i].DelayTime = 100;
			Multi_Udp_Buff[i].isValid = 1;

			//打开互斥锁
			pthread_mutex_unlock (&Local.udp_lock);

			sem_post(&multi_send_sem);
			break;
		}
}

void RecvCheckMac_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	unsigned char send_b[1520];
	int sendlength;
	int isOK;

	isOK = memcmp(recv_buf+8,CENTER,20);
	if((isOK==0)&&(recv_buf[7]==0x01))
	{
		memcpy(send_b,recv_buf,33);
		send_b[7]=0x02;//REPLY;
		if(recv_buf[32]==0x01)
		{
			sendlength = 105;
			memset(send_b+33,0x30,20);
			memcpy(send_b + 33, LocalCfg.Addr, 12);
			//网卡地址
			memcpy(send_b + 53, LocalCfg.Mac_Addr, 6);
			//IP地址
			memcpy(send_b + 59, LocalCfg.IP, 4);
			//子网掩码
			memcpy(send_b + 63, LocalCfg.IP_Mask, 4);
			//网关地址
			memcpy(send_b + 67, LocalCfg.IP_Gate, 4);
			//服务器地址
			memcpy(send_b + 71, LocalCfg.IP_Server, 4);

			//工程密码
			memcpy(send_b + 75, LocalCfg.EngineerPass,10);
			//用户密码
			memcpy(send_b + 85, LocalCfg.OpenLockPass, 10);
			//////////////version0805
			memset(send_b+95,0,10);
			strcpy(send_b + 95,SOFTWAREVER);
			//	UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);
			/*
			   for(i=0;i<10;i++)
			   LOGD("%c,",LocalCfg.OpenLockPass[i]);
			   LOGD("\n");
			   */			

			UdpSendBuff(m_Socket,cFromIP,send_b,sendlength);
		}
		else if(recv_buf[32]==0x02)
		{
			//	sendlength = 34;
			isOK=memcmp(recv_buf+33,LocalCfg.Mac_Addr,6);
			if(isOK==0)
			{
				sendlength = 105;
				memset(send_b+33,0x30,20);
				memcpy(send_b + 33, LocalCfg.Addr, 12);
				//网卡地址
				memcpy(send_b + 53, LocalCfg.Mac_Addr, 6);
				//IP地址
				memcpy(send_b + 59, LocalCfg.IP, 4);
				//子网掩码
				memcpy(send_b + 63, LocalCfg.IP_Mask, 4);
				//网关地址
				memcpy(send_b + 67, LocalCfg.IP_Gate, 4);
				//服务器地址
				memcpy(send_b + 71, LocalCfg.IP_Server, 4);

				//工程密码
				memcpy(send_b + 75, LocalCfg.EngineerPass,10);
				//用户密码
				memcpy(send_b + 85, LocalCfg.OpenLockPass, 10);
				//////////////version0805
				//getchar();
				memset(send_b+95,0,10);
				strcpy(send_b + 95,SOFTWAREVER);
				//	UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);

				UdpSendBuff(m_Socket,cFromIP,send_b,sendlength);
			}
		}
	}
}

void RecvTest_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int isOK;
	char wavFile[30];

	isOK = memcmp(recv_buf+8,CENTER,20);
	if((isOK==0)&&(recv_buf[7]==1))
	{
		StartRecVideo(CIF_W, CIF_H);
		sLOGD(wavFile,"%sasong.wav\0","/mnt/");
		Local.CallConfirmFlag = 1; //设置在线标志
		Local.Timer1Num = 0;
		Local.TimeOut = 0;       //监视超时,  通话超时,  呼叫超时，无人接听
		Local.OnlineNum = 0;     //在线确认序号
		Local.OnlineFlag = 1;   
		Local.Status = 4;
		SendHostOrder(0x60, Local.DoorNo, NULL); //发送主动命令  对方监视  暂定为1号

		StartPlayWav(wavFile,1);
	}

}

//报警
void Recv_Alarm_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int isAddrOK;
	unsigned char AlarmByte;

	i = 0;
	isAddrOK = 1;
	for (j=9; j<15; j++) {
		if (LocalCfg.Addr[j-8] != recv_buf[j]) {
			isAddrOK = 0;
			break;
		}
    }

	//地址匹配
	if (isAddrOK == 1) {
		//锁定互斥锁
		pthread_mutex_lock (&Local.udp_lock);
		if(recv_buf[7] == REPLY) {
			for (i=0; i<UDPSENDMAX; i++) {
				if (Multi_Udp_Buff[i].isValid == 1) {
					if (Multi_Udp_Buff[i].m_Socket == m_DataSocket) {
						if (Multi_Udp_Buff[i].SendNum  < MAXSENDNUM) {
							if (Multi_Udp_Buff[i].buf[6] == ALARM) {
								if (Multi_Udp_Buff[i].buf[7] == ASK) {
									if (strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0) {
                                        LOGD("ALARM REPLY \n");
                                        if (Multi_Udp_Buff[i].buf[56] != recv_buf[56]) {
                                            continue;
                                        }
                                        Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
										LOGD("ALARM REPLY %d\n", AlarmByte);
#endif
										break;
									}
                                }
                            }
                        }
                    }
                }
            }
        }
		//打开互斥锁
		pthread_mutex_unlock (&Local.udp_lock);
	}
}

void RecvOpenLock_Func(unsigned char *recv_buf)
{
	int i,j;
	unsigned char send_b[1520];

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);
	//本机主动
	if(recv_buf[7] == REPLY)
		for(i=0; i<UDPSENDMAX; i++)
			if(Multi_Udp_Buff[i].isValid == 1)
				if(Multi_Udp_Buff[i].m_Socket == m_DataSocket)
					if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
						if((Multi_Udp_Buff[i].buf[6] == LIFT))
							if(Multi_Udp_Buff[i].buf[7] == ASK)
								if(Multi_Udp_Buff[i].buf[8] == OPENLOCK)
								//	if(strcmp(Multi_Udp_Buff[i].RemoteHost, cFromIP) == 0)
									{
										Multi_Udp_Buff[i].isValid = 0;
										memcpy(send_b,recv_buf+29,15);
										send_b[1]=0x06;
										send_b[2]=0x00;
									//	send_b[3]=0x0a;
									//	send_b[4]=0x0a;
									//	send_b[5]=0x00;
									//	send_b[6]=0x60;
										send_b[7]=0x00;
										send_b[8]=0x00;
										for(j=0;j<8;j++)
										{
											LOGD("%02x ",send_b[j]);
											send_b[8]+=send_b[j];
										}
										
										LOGD("%02x\n",send_b[j]);
										CommSendBuff(Comm2fd, send_b,9);
										break;
									}
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);
}

void RecvRSVilla_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	int i,j;
	int isAddrOK;
	unsigned char send_b[1520];
	int sendlength;

	i = 0;
	isAddrOK = 1;
	for(j=28; j<28+Local.AddrLen; j++)
		if(LocalCfg.Addr[j-28] != recv_buf[j])
		{
			isAddrOK = 0;
			break;
		}
	
	LOGD("RECV::");
	for(i=0;i<recv_buf[55]+3;i++)
		LOGD(" %02x ",recv_buf[54+i]);
	LOGD("\n");

	if(isAddrOK == 1)
	{
		memcpy(send_b,recv_buf + 54, length - 54);
		if((send_b[0] == 0xBD)&&((send_b[2] == 0x15)||(send_b[2] == 0x16)))
		{
//			send_b[7] = 0x0A;
//			send_b[8] = 0x0A;
			for(i = 0; i<10; i++)
			{
				if(villabuf[i][0] == 0x00)
				{
					villabuf[i][0] = Local.villacount;
					Local.villacount ++;
					if(!Local.villacount)
						Local.villacount ++;
					send_b[11] = villabuf[i][0];
					if(send_b[2]==0x15)
						send_b[10] = Local.DoorNo - 0x31;
					memcpy(villabuf[i]+1,recv_buf,54);
					break;
				}
			}
			if(i==10)
			{
				memcpy(send_b,recv_buf,length);
				send_b[7] = 0x03;
				UdpSendBuff(m_Socket, cFromIP, send_b , sendlength);
				return;
			}
			if(send_b[2] == 0x15)
			{
				j = 1;
				send_b[11+j] += send_b[10];
			}
			else if(send_b[2] == 0x16)
				j = 5;

			send_b[11+j] += send_b[11];

			CommSendBuff(Comm3fd,send_b,send_b[1]+3);
			for(i=0;i<send_b[1]+3;i++)
				LOGD(" %02x ",send_b[i]);
			LOGD("\n");
		}
	}
}

void RecvSetPWD_Func(unsigned char *recv_buf, char *cFromIP, int length, int m_Socket)
{
	char send_c[20];
	int i,j;
	int isOK;

	isOK =  memcmp(recv_buf+28,LocalCfg.Addr,7);
	if(isOK)
		return;

	LOGD("RECVSETPWD:\n ");	
	send_c[0] = 0x7E;
	if(recv_buf[54] == 0x00)
		send_c[1] = 0x74;//lock
	else
		send_c[1] = 0x73;//rob

	send_c[2] = 0x31;
	send_c[3] = Local.DoorNo;
	//110329send_c[3] = 0x31;

	memcpy(send_c + 4, recv_buf + 15, 4);/////room addr
	memcpy(send_c + 8, recv_buf + 55, 4);/////original pwd
	memcpy(send_c + 12, recv_buf + 59, 4);////new pwd

	send_c[16] = 0;
	for(i=1; i<=15; i++)    //crc
	{
		send_c[16] += send_c[i];
	}
	if((send_c[16] == 0x7E)||(send_c[16] == 0x0D))
		send_c[16] --;
	send_c[17] = 0x0D;
	
//	CommSendBuff(Comm3fd,send_c,18);
///	SendHostOrder();
	pthread_mutex_lock (&Local.comm_lock);
	//查找可用发送缓冲并填空
	for(i=0; i<COMMSENDMAX; i++)
		if(Multi_Comm_Buff[i].isValid == 0)
		{
			Multi_Comm_Buff[i].SendNum = 2;
			Multi_Comm_Buff[i].m_Comm = Comm3fd;
			memcpy(Multi_Comm_Buff[i].buf , send_c, 18);
			strcpy(Multi_Comm_Buff[i].buf +20 , cFromIP);
			Multi_Comm_Buff[i].nlength = 18;
			Multi_Comm_Buff[i].isValid = 1;
			sem_post(&multi_comm_send_sem);
			break;
		}
	//打开互斥锁
	pthread_mutex_unlock (&Local.comm_lock);
	for(j=0;j<18;j++)
	  LOGD("%02x ",send_c[j]);	
}

void RecvSendTalkInfo_Func(unsigned char *recv_buf,char *cFromIP,int m_Socket)
{
	int i,j;
	int isAddrOK;

	i = 0;
	isAddrOK = 1;
//	LOGD("is this -1?\n");
	for(j=8; j<8+Local.AddrLen; j++)
	{
	  if(LocalCfg.Addr[j-8] != recv_buf[j])
	  {
//		  LOGD("is this -2?\n");
		  isAddrOK = 0;
		  break;
	  }
	}

	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);

	if(isAddrOK == 1)
	{
//		LOGD("is this -3?\n");
		if(recv_buf[7] == REPLY)
		  for(i=0; i<UDPSENDMAX; i++)
			if(Multi_Udp_Buff[i].isValid == 1)
			  if(Multi_Udp_Buff[i].m_Socket == m_DataSocket)
				if(Multi_Udp_Buff[i].SendNum  < MAXSENDNUM)
				  if(Multi_Udp_Buff[i].buf[6] == SENDTALKINFO)
					if(Multi_Udp_Buff[i].buf[7] == ASK)
					  //if(strcmp(Multi_Udp_Buff[i].RemoteAddr[0], cFromIP) == 0)
					  {
						  
						  Multi_Udp_Buff[i].isValid = 0;
						  LOGD("recv the  send talk info !\n");
						  break;
					  }

	}
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);

}

