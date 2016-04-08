#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <dirent.h>

#include <assert.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "common.h"

extern struct timeval ref_time;  //基准时间,音频或视频第一帧

//---------------------------------------------------------------------------
int init_main(int argc, char *argv[])
{
	pthread_attr_t attr;
	int i;
	uint32_t Ip_Int;

	ref_time.tv_sec = 0;   //初始时间戳
	ref_time.tv_usec = 0;
	TimeStamp.OldCurrVideo = 0;       //上一次当前视频时间
	TimeStamp.CurrVideo = 0;
	TimeStamp.OldCurrAudio = 0;       //上一次当前音频时间
	TimeStamp.CurrAudio = 0;     

	RemoteDataPort = 8300;
	RemoteVideoPort = 8302;
	strcpy(RemoteHost, "192.168.10.51");
	LocalDataPort = 8300;   //命令及数据UDP端口
	LocalVideoPort = 8302;  //音视频UDP端口

	//读配置文件
	if (LocalCfg.Addr[0] == 'W') {
		Local.AddrLen = 5;  //地址长度  S 12  B 6 M 8 H 6
	}
	if (LocalCfg.Addr[0] == 'M') {
		Local.AddrLen = 7;  //地址长度  S 12  B 6 M 8 H 6
	}  

	Local.ReportSend = 1;  //设备定时报告状态已发送
	Local.RandReportTime = 1;
	Local.ReportTimeNum  = 0;

#ifdef _VIDEOZOOMOUT
	Local.OpenD1VideoFlag = 0;
#endif
    TurnToCenter = 0;

	DeltaLen = 9 + sizeof(struct talkdata1);  //数据包有效数据偏移量 

	strcpy(UdpPackageHead, "QIUSHI");
	memset(NullAddr, 0x30, 20);
	NullAddr[20] = '\0';  

	Local.DoorNo = 0x31;  //门口机编码
	Local.Status = 0;  //状态为空闲
	Local.RecPicSize = 1;  //默认视频大小为352*240
	Local.PlayPicSize = 1;  //默认视频大小为352*240
	Local.videozoomout = 0;

#ifdef _DIFFERENCE_DEV_SUPPORT
	Local.minpacket_flag = 0;
#endif

	Ip_Int = inet_addr("192.168.10.188");
	memcpy(Remote.IP,&Ip_Int,4);
	memcpy(Remote.Addr[0],NullAddr,20);
	memcpy(Remote.Addr[0],"S00010101010",12);      //

	//线程运行标志
	Local.Multi_Send_Run_Flag = 1;

	//系统初始化标志
	InitSuccFlag = 0;
	//按钮初始化,为加快速度，先初始化首页显示需要的按钮

	m_EthSocket = 0;    
	//UDP
	if (InitUdpSocket(LocalDataPort) == 0) {
		LOGD("can't create data socket.\n\r");
		return (0);
	}

	if (InitUdpSocket(LocalVideoPort) == 0) {
		LOGD("can't create video rece socket.\n\r");
		return (0);
	}

	if (InitUdpSocket(8307) == 0) {
		LOGD("can't create video send socket.\n\r");
		return (0);
	}

	//将UDP发送缓冲置为无效
	for (i=0; i<UDPSENDMAX; i++) {
		Multi_Udp_Buff[i].isValid = 0;
		Multi_Udp_Buff[i].SendNum = 0;
		Multi_Udp_Buff[i].DelayTime = 100;
	}

	//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
	//用于UDP和Comm通信
	sem_init(&multi_send_sem, 0, 0);
	multi_send_flag = 1;
	//创建互斥锁
	pthread_mutex_init (&Local.udp_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&multi_send_thread,&attr,(void *)multi_send_thread_func,NULL);
	pthread_attr_destroy(&attr);
	if ( multi_send_thread == 0 ) {
		LOGD("无法创建UDP主动命令数据发送线程\n");
		return (0);
	}

	//初始化ARP Socket  
	InitArpSocket();

	//加入NS组播组
	AddMultiGroup(m_VideoSocket, NSMULTIADDR);  

	//初始化
	gpio_rcv_flag = 1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&gpio_rcv_thread,&attr,(void *)gpio_rcv_thread_func,NULL);
	pthread_attr_destroy(&attr);
	if ( gpio_rcv_thread == 0 ) {
		LOGD("无法创建gpio按键处理线程\n");
		return 0;
	}

	TimeReportStatusFunc();//设备定时报告状态

	LOGD("\nVERSION %s\n\n",SOFTWAREVER);

	return (0);  
}

void multi_send_thread_func(void)
{
	int i,j;
	int isAdded;  
	int HaveDataSend;
	static int send_count = 0;
    unsigned char temp_buf[512];
    int send_flag = 0;

#ifdef _DEBUG
	LOGD("创建主动命令数据发送线程：\n" );
	LOGD("multi_send_flag=%d\n",multi_send_flag);
#endif
	while(multi_send_flag == 1)
	{
		//等待有按键按下的信号
		sem_wait(&multi_send_sem);
		if(Local.Multi_Send_Run_Flag == 0)
		  Local.Multi_Send_Run_Flag = 1;
		else
		{
			HaveDataSend = 1;
			while (HaveDataSend)
			{
				for (i=0; i<UDPSENDMAX; i++) {
                    if (Multi_Udp_Buff[i].isValid == 1) {
                        if (Multi_Udp_Buff[i].SendNum  < MAXSENDNUM) {
                            if ((Multi_Udp_Buff[i].SendNum != 0)&&(Multi_Udp_Buff[i].DelayTime > 100)) {
                                usleep((Multi_Udp_Buff[i].DelayTime - 100)*1000);
                            }
                            //UDP发送
                            UdpSendBuff(Multi_Udp_Buff[i].m_Socket, Multi_Udp_Buff[i].RemoteHost,
                                    Multi_Udp_Buff[i].buf , Multi_Udp_Buff[i].nlength);
                            Multi_Udp_Buff[i].SendNum++;

                            if ((Multi_Udp_Buff[i].buf[6] == NSORDER \
                                        || (Multi_Udp_Buff[i].buf[8] == CALLEND) \
                                        && (Multi_Udp_Buff[i].buf[6] == VIDEOTALK))) {
                                if (Multi_Udp_Buff[i].SendNum == 6) {
                                    if (send_count < 18) {
                                        Multi_Udp_Buff[i].SendNum = 0;
                                        send_count += 6;
                                    } else {
                                        send_count = 0;
                                    }
                                    usleep((50+Multi_Udp_Buff[i].SendNum*10)*1000);
                                } else {
                                    usleep(50*1000);
                                }
                            }
                        }

                        if (Multi_Udp_Buff[i].SendNum  >= MAXSENDNUM) {
                            //锁定互斥锁
                            pthread_mutex_lock (&Local.udp_lock);
                            /* Multi_Udp_Buff[i].isValid = 0; */
                            if (Multi_Udp_Buff[i].m_Socket == ARP_Socket) {
                                Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                LOGD("免费ARP发送完成\n");
#endif
                            } else if (Multi_Udp_Buff[i].m_Socket == m_ForbidSocket) {
                                Multi_Udp_Buff[i].isValid = 0;
                            } else {
#ifdef _DEBUG
                                LOGD("发送UDP数据: 发送缓冲: %d, 命令: %d.\n",i,Multi_Udp_Buff[i].buf[6]);
#endif
                                switch (Multi_Udp_Buff[i].buf[6])
                                {
                                    case NSORDER:
                                        {
                                            if (Multi_Udp_Buff[i].CurrOrder == 255) {
                                                //主机向副机查找
                                                Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                                LOGD("check fail\n");
#endif
                                            } else {
                                                if (Multi_Udp_Buff[i].buf[32] == 'Z') {
                                                    Multi_Udp_Buff[i].SendNum = 0;
                                                    memcpy(temp_buf, Multi_Udp_Buff[i].buf, Multi_Udp_Buff[i].nlength);
                                                    sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_NS[0],
                                                            LocalCfg.IP_NS[1],LocalCfg.IP_NS[2],LocalCfg.IP_NS[3]);
                                                    memcpy(temp_buf + Multi_Udp_Buff[i].nlength, Multi_Udp_Buff[i].RemoteHost, 4);
                                                    temp_buf[7] = REPLY;
                                                    send_flag = 1;
                                                } else {
                                                    //若该命令为子网查找，则下一个命令为向服务器查找
                                                    Multi_Udp_Buff[i].SendNum = 0;
                                                    //更改UDP端口
                                                    Multi_Udp_Buff[i].m_Socket = m_DataSocket;
                                                    sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_NS[0],
                                                            LocalCfg.IP_NS[1],LocalCfg.IP_NS[2],LocalCfg.IP_NS[3]);
                                                    LOGD("IP:%s\n",Multi_Udp_Buff[i].RemoteHost);
                                                    //命令, 服务器查找
                                                    Multi_Udp_Buff[i].buf[6] = NSSERVERORDER;
#ifdef _DEBUG
                                                    LOGD("checking from NS\n");
#endif
                                                }
                                            }
                                            break;
                                        }
                                    case NSSERVERORDER: //服务器查找
                                        {
                                            Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                            LOGD("服务器查找失败\n");
#endif
                                            if(Multi_Udp_Buff[i].CurrOrder == FINDEQUIP)
                                                SendHostOrder(0x5A, Local.DoorNo, Local.FindEquip); //发送主动命令  查找设备失败
                                            if(Multi_Udp_Buff[i].CurrOrder == VIDEOTALK)
                                                SendHostOrder(0x63, Local.DoorNo, NULL); //发送主动命令  呼叫失败
                                            SendTalkInfo.Status = 0x01;//0x06 call 0x07 pwd
                                            SendTalkInfo.Duration =0x00;
                                            SendTalkInfoFlag = 1;
                                            TurnToCenter = 0;
                                            LOGD("%d: TurnToCenter = %d\n", __LINE__, TurnToCenter);
                                            break;
                                        }
                                    case FINDEQUIP:    //查找设备
                                        {
                                            Multi_Udp_Buff[i].isValid = 0;
                                            Local.Status = 0;
#ifdef _DEBUG
                                            LOGD("查找设备失败, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                        }
                                        break;
                                    case VIDEOTALK:    //局域网可视对讲
                                    case VIDEOTALKTRANS:  //局域网可视对讲中转服务
                                        {
                                            switch (Multi_Udp_Buff[i].buf[8]) 
                                            {
                                                case CALL:
                                                    {
                                                        if (Multi_Udp_Buff[i].buf[6] == VIDEOTALK) {
                                                            LOGD("\n%d: Remote.DenNum = %d\n", __LINE__, Remote.DenNum);
                                                            if (Remote.DenNum == 1) {
                                                                //若该命令为直通呼叫，则下一个命令为向服务器请求中转
                                                                Multi_Udp_Buff[i].SendNum = 0;
                                                                //更改UDP端口
                                                                Multi_Udp_Buff[i].m_Socket = m_DataSocket;
                                                                sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
                                                                        LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
                                                                //命令, 服务器中转
                                                                Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
#ifdef _DEBUG
                                                                LOGD("正在向主服务器申请对讲中转\n");
#endif
                                                            }
                                                        } else {
                                                            Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                                            LOGD("呼叫失败, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                                        }
                                                        SendTalkInfo.Status = 0x01;//0x06 call 0x07 pwd
                                                        SendTalkInfo.Duration =0x00;
                                                        SendTalkInfoFlag = 1;
                                                        break;
                                                    }
                                                case CALLEND:  //通话结束
                                                    {
                                                        Multi_Udp_Buff[i].isValid = 0;
                                                        Local.OnlineFlag = 0;
                                                        Local.CallConfirmFlag = 0; //设置在线标志
                                                        //对讲结束，清状态和关闭音视频
                                                        TalkEnd_ClearStatus();
                                                        break;
                                                    }
                                                default: //为其它命令，本次通信结束
                                                    {
                                                        Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                                        LOGD("send fail buf%d, 111 %d\n",i, Multi_Udp_Buff[i].buf[8]);
#endif
                                                        break;
                                                    }
                                            }
                                            break;
                                        }
                                    case VIDEOWATCH:     //局域网监控
                                    case VIDEOWATCHTRANS:  //局域网监控中转服务
                                        {
                                            switch (Multi_Udp_Buff[i].buf[8])
                                            {
                                                case CALL:
                                                    {
                                                        if(Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)
                                                        {
                                                            //若该命令为直通呼叫，则下一个命令为向服务器请求中转
                                                            Multi_Udp_Buff[i].SendNum = 0;
                                                            //更改UDP端口
                                                            Multi_Udp_Buff[i].m_Socket = m_DataSocket;
                                                            sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
                                                                    LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
                                                            //命令, 服务器中转
                                                            Multi_Udp_Buff[i].buf[6] = VIDEOWATCHTRANS;
#ifdef _DEBUG
                                                            LOGD("在向主服务器申请监控中转\n");
#endif
                                                        }
                                                        else
                                                        {
                                                            Multi_Udp_Buff[i].isValid = 0;
                                                            Local.Status = 0;
#ifdef _DEBUG
                                                            LOGD("监视失败, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                                        }
                                                        break;
                                                    }
                                                case CALLEND:  //通话结束
                                                    {
                                                        Multi_Udp_Buff[i].isValid = 0;
                                                        Local.OnlineFlag = 0;
                                                        Local.CallConfirmFlag = 0; //设置在线标志

                                                        switch (Local.Status)
                                                        {
                                                            case 3: //本机监视
                                                                {
                                                                    Local.Status = 0;  //状态为空闲
                                                                    break;
                                                                }
                                                            case 4: //本机被监视
                                                                {
                                                                    Local.Status = 0;  //状态为空闲
                                                                    StopRecVideo();
                                                                    break;
                                                                }
                                                        }
                                                        break;
                                                    }
                                                default: //为其它命令，本次通信结束
                                                    {
                                                        Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                                        LOGD("通信失败, 222%d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                                        break;
                                                    }
                                            }
                                            break;
                                        }
                                    default: //为其它命令，本次通信结束
                                        {
                                            Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                            LOGD("通信失败, 333%d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                            break;
                                        }
                                }
                            }
                            //打开互斥锁
                            pthread_mutex_unlock (&Local.udp_lock);
                        }
                    }
                }

                if (send_flag == 1) {
                    send_flag = 0;
                    RecvNSReply_Func(temp_buf, NULL, m_VideoSendSocket);
                }

                //判断数据是否全部发送完，若是，线程终止
                HaveDataSend = 0;
                for (i=0; i<UDPSENDMAX; i++) {
                    if (Multi_Udp_Buff[i].isValid == 1) {
                        HaveDataSend = 1;
                        break;
                    }
                }
				usleep(50*1000);
			}
		}
	}
}

