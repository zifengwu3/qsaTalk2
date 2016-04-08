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

extern struct timeval ref_time;  //��׼ʱ��,��Ƶ����Ƶ��һ֡

//---------------------------------------------------------------------------
int init_main(int argc, char *argv[])
{
	pthread_attr_t attr;
	int i;
	uint32_t Ip_Int;

	ref_time.tv_sec = 0;   //��ʼʱ���
	ref_time.tv_usec = 0;
	TimeStamp.OldCurrVideo = 0;       //��һ�ε�ǰ��Ƶʱ��
	TimeStamp.CurrVideo = 0;
	TimeStamp.OldCurrAudio = 0;       //��һ�ε�ǰ��Ƶʱ��
	TimeStamp.CurrAudio = 0;     

	RemoteDataPort = 8300;
	RemoteVideoPort = 8302;
	strcpy(RemoteHost, "192.168.10.51");
	LocalDataPort = 8300;   //�������UDP�˿�
	LocalVideoPort = 8302;  //����ƵUDP�˿�

	//�������ļ�
	if (LocalCfg.Addr[0] == 'W') {
		Local.AddrLen = 5;  //��ַ����  S 12  B 6 M 8 H 6
	}
	if (LocalCfg.Addr[0] == 'M') {
		Local.AddrLen = 7;  //��ַ����  S 12  B 6 M 8 H 6
	}  

	Local.ReportSend = 1;  //�豸��ʱ����״̬�ѷ���
	Local.RandReportTime = 1;
	Local.ReportTimeNum  = 0;

#ifdef _VIDEOZOOMOUT
	Local.OpenD1VideoFlag = 0;
#endif
    TurnToCenter = 0;

	DeltaLen = 9 + sizeof(struct talkdata1);  //���ݰ���Ч����ƫ���� 

	strcpy(UdpPackageHead, "QIUSHI");
	memset(NullAddr, 0x30, 20);
	NullAddr[20] = '\0';  

	Local.DoorNo = 0x31;  //�ſڻ�����
	Local.Status = 0;  //״̬Ϊ����
	Local.RecPicSize = 1;  //Ĭ����Ƶ��СΪ352*240
	Local.PlayPicSize = 1;  //Ĭ����Ƶ��СΪ352*240
	Local.videozoomout = 0;

#ifdef _DIFFERENCE_DEV_SUPPORT
	Local.minpacket_flag = 0;
#endif

	Ip_Int = inet_addr("192.168.10.188");
	memcpy(Remote.IP,&Ip_Int,4);
	memcpy(Remote.Addr[0],NullAddr,20);
	memcpy(Remote.Addr[0],"S00010101010",12);      //

	//�߳����б�־
	Local.Multi_Send_Run_Flag = 1;

	//ϵͳ��ʼ����־
	InitSuccFlag = 0;
	//��ť��ʼ��,Ϊ�ӿ��ٶȣ��ȳ�ʼ����ҳ��ʾ��Ҫ�İ�ť

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

	//��UDP���ͻ�����Ϊ��Ч
	for (i=0; i<UDPSENDMAX; i++) {
		Multi_Udp_Buff[i].isValid = 0;
		Multi_Udp_Buff[i].SendNum = 0;
		Multi_Udp_Buff[i].DelayTime = 100;
	}

	//�����������ݷ����̣߳��ն����������������ʱһ��û�յ���Ӧ�����η���
	//����UDP��Commͨ��
	sem_init(&multi_send_sem, 0, 0);
	multi_send_flag = 1;
	//����������
	pthread_mutex_init (&Local.udp_lock, NULL);
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&multi_send_thread,&attr,(void *)multi_send_thread_func,NULL);
	pthread_attr_destroy(&attr);
	if ( multi_send_thread == 0 ) {
		LOGD("�޷�����UDP�����������ݷ����߳�\n");
		return (0);
	}

	//��ʼ��ARP Socket  
	InitArpSocket();

	//����NS�鲥��
	AddMultiGroup(m_VideoSocket, NSMULTIADDR);  

	//��ʼ��
	gpio_rcv_flag = 1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&gpio_rcv_thread,&attr,(void *)gpio_rcv_thread_func,NULL);
	pthread_attr_destroy(&attr);
	if ( gpio_rcv_thread == 0 ) {
		LOGD("�޷�����gpio���������߳�\n");
		return 0;
	}

	TimeReportStatusFunc();//�豸��ʱ����״̬

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
	LOGD("���������������ݷ����̣߳�\n" );
	LOGD("multi_send_flag=%d\n",multi_send_flag);
#endif
	while(multi_send_flag == 1)
	{
		//�ȴ��а������µ��ź�
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
                            //UDP����
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
                            //����������
                            pthread_mutex_lock (&Local.udp_lock);
                            /* Multi_Udp_Buff[i].isValid = 0; */
                            if (Multi_Udp_Buff[i].m_Socket == ARP_Socket) {
                                Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                LOGD("���ARP�������\n");
#endif
                            } else if (Multi_Udp_Buff[i].m_Socket == m_ForbidSocket) {
                                Multi_Udp_Buff[i].isValid = 0;
                            } else {
#ifdef _DEBUG
                                LOGD("����UDP����: ���ͻ���: %d, ����: %d.\n",i,Multi_Udp_Buff[i].buf[6]);
#endif
                                switch (Multi_Udp_Buff[i].buf[6])
                                {
                                    case NSORDER:
                                        {
                                            if (Multi_Udp_Buff[i].CurrOrder == 255) {
                                                //�����򸱻�����
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
                                                    //��������Ϊ�������ң�����һ������Ϊ�����������
                                                    Multi_Udp_Buff[i].SendNum = 0;
                                                    //����UDP�˿�
                                                    Multi_Udp_Buff[i].m_Socket = m_DataSocket;
                                                    sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_NS[0],
                                                            LocalCfg.IP_NS[1],LocalCfg.IP_NS[2],LocalCfg.IP_NS[3]);
                                                    LOGD("IP:%s\n",Multi_Udp_Buff[i].RemoteHost);
                                                    //����, ����������
                                                    Multi_Udp_Buff[i].buf[6] = NSSERVERORDER;
#ifdef _DEBUG
                                                    LOGD("checking from NS\n");
#endif
                                                }
                                            }
                                            break;
                                        }
                                    case NSSERVERORDER: //����������
                                        {
                                            Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                            LOGD("����������ʧ��\n");
#endif
                                            if(Multi_Udp_Buff[i].CurrOrder == FINDEQUIP)
                                                SendHostOrder(0x5A, Local.DoorNo, Local.FindEquip); //������������  �����豸ʧ��
                                            if(Multi_Udp_Buff[i].CurrOrder == VIDEOTALK)
                                                SendHostOrder(0x63, Local.DoorNo, NULL); //������������  ����ʧ��
                                            SendTalkInfo.Status = 0x01;//0x06 call 0x07 pwd
                                            SendTalkInfo.Duration =0x00;
                                            SendTalkInfoFlag = 1;
                                            TurnToCenter = 0;
                                            LOGD("%d: TurnToCenter = %d\n", __LINE__, TurnToCenter);
                                            break;
                                        }
                                    case FINDEQUIP:    //�����豸
                                        {
                                            Multi_Udp_Buff[i].isValid = 0;
                                            Local.Status = 0;
#ifdef _DEBUG
                                            LOGD("�����豸ʧ��, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                        }
                                        break;
                                    case VIDEOTALK:    //���������ӶԽ�
                                    case VIDEOTALKTRANS:  //���������ӶԽ���ת����
                                        {
                                            switch (Multi_Udp_Buff[i].buf[8]) 
                                            {
                                                case CALL:
                                                    {
                                                        if (Multi_Udp_Buff[i].buf[6] == VIDEOTALK) {
                                                            LOGD("\n%d: Remote.DenNum = %d\n", __LINE__, Remote.DenNum);
                                                            if (Remote.DenNum == 1) {
                                                                //��������Ϊֱͨ���У�����һ������Ϊ�������������ת
                                                                Multi_Udp_Buff[i].SendNum = 0;
                                                                //����UDP�˿�
                                                                Multi_Udp_Buff[i].m_Socket = m_DataSocket;
                                                                sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
                                                                        LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
                                                                //����, ��������ת
                                                                Multi_Udp_Buff[i].buf[6] = VIDEOTALKTRANS;
#ifdef _DEBUG
                                                                LOGD("������������������Խ���ת\n");
#endif
                                                            }
                                                        } else {
                                                            Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                                            LOGD("����ʧ��, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                                        }
                                                        SendTalkInfo.Status = 0x01;//0x06 call 0x07 pwd
                                                        SendTalkInfo.Duration =0x00;
                                                        SendTalkInfoFlag = 1;
                                                        break;
                                                    }
                                                case CALLEND:  //ͨ������
                                                    {
                                                        Multi_Udp_Buff[i].isValid = 0;
                                                        Local.OnlineFlag = 0;
                                                        Local.CallConfirmFlag = 0; //�������߱�־
                                                        //�Խ���������״̬�͹ر�����Ƶ
                                                        TalkEnd_ClearStatus();
                                                        break;
                                                    }
                                                default: //Ϊ�����������ͨ�Ž���
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
                                    case VIDEOWATCH:     //���������
                                    case VIDEOWATCHTRANS:  //�����������ת����
                                        {
                                            switch (Multi_Udp_Buff[i].buf[8])
                                            {
                                                case CALL:
                                                    {
                                                        if(Multi_Udp_Buff[i].buf[6] == VIDEOWATCH)
                                                        {
                                                            //��������Ϊֱͨ���У�����һ������Ϊ�������������ת
                                                            Multi_Udp_Buff[i].SendNum = 0;
                                                            //����UDP�˿�
                                                            Multi_Udp_Buff[i].m_Socket = m_DataSocket;
                                                            sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
                                                                    LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
                                                            //����, ��������ת
                                                            Multi_Udp_Buff[i].buf[6] = VIDEOWATCHTRANS;
#ifdef _DEBUG
                                                            LOGD("��������������������ת\n");
#endif
                                                        }
                                                        else
                                                        {
                                                            Multi_Udp_Buff[i].isValid = 0;
                                                            Local.Status = 0;
#ifdef _DEBUG
                                                            LOGD("����ʧ��, %d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                                        }
                                                        break;
                                                    }
                                                case CALLEND:  //ͨ������
                                                    {
                                                        Multi_Udp_Buff[i].isValid = 0;
                                                        Local.OnlineFlag = 0;
                                                        Local.CallConfirmFlag = 0; //�������߱�־

                                                        switch (Local.Status)
                                                        {
                                                            case 3: //��������
                                                                {
                                                                    Local.Status = 0;  //״̬Ϊ����
                                                                    break;
                                                                }
                                                            case 4: //����������
                                                                {
                                                                    Local.Status = 0;  //״̬Ϊ����
                                                                    StopRecVideo();
                                                                    break;
                                                                }
                                                        }
                                                        break;
                                                    }
                                                default: //Ϊ�����������ͨ�Ž���
                                                    {
                                                        Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                                        LOGD("ͨ��ʧ��, 222%d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                                        break;
                                                    }
                                            }
                                            break;
                                        }
                                    default: //Ϊ�����������ͨ�Ž���
                                        {
                                            Multi_Udp_Buff[i].isValid = 0;
#ifdef _DEBUG
                                            LOGD("ͨ��ʧ��, 333%d\n", Multi_Udp_Buff[i].buf[6]);
#endif
                                            break;
                                        }
                                }
                            }
                            //�򿪻�����
                            pthread_mutex_unlock (&Local.udp_lock);
                        }
                    }
                }

                if (send_flag == 1) {
                    send_flag = 0;
                    RecvNSReply_Func(temp_buf, NULL, m_VideoSendSocket);
                }

                //�ж������Ƿ�ȫ�������꣬���ǣ��߳���ֹ
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

