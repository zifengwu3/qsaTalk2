#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/ipc.h>

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned char u8;

//#include <linux/ethtool.h>
#include <net/if.h>
#include <linux/sockios.h>

#define CommonH
#include "common.h"

//����ɨ��
void OnlineCheckFunc(void); //����ȷ�ϼ�⺯��
void TimeReportStatusFunc(void); //�豸��ʱ����״̬����

void TalkCtrlFunc(void);  //�Խ����ƣ���ʾͨ��ʱ����жϳ�ʱ

int timer_thread_flag;
pthread_t timer_thread;
void timer_rcv_thread_task(void);


void timer_rcv_thread_task(void)
{
	int timenum;  //����

#ifdef _DEBUG
	printf("����timer�����̣߳�\n" );
	printf("timer_rcv_flag=%d\n",timer_rcv_flag);
#endif

	timenum = 0;
	while (timer_rcv_flag)
	{
        //����6��û���յ�����ȷ�ϣ�����Ϊ����
        if (Local.OnlineFlag == 1) {
            OnlineCheckFunc();
            TalkCtrlFunc();
        }

        if (SendTalkInfoFlag) {
            SendInfoToCenter_Func();	
            SendTalkInfoFlag = 0;
        }

        if (NeedOpenLock) {
            SendInfoToCenter_Func();	
            NeedOpenLock= 0;
        }

        if (Local.OpenD1VideoFlag == 1 && Local.Status != 0 && Local.PlayPicSize == 1) {
            StartRecVideo(D1_W, D1_H);
            Local.OpenD1VideoFlag = 0;
        }

        if (LocalCfg.ReportTime != 0 && Local.Status == 0) {
            if (Local.ReportSend == 1) {
                if (Local.ReportTimeNum >= (LocalCfg.ReportTime*TIMERPERSEC)) {
                    Local.RandReportTime = (int)(1.0*LocalCfg.ReportTime*rand()/(RAND_MAX+1.0)) + 1;
                    Local.ReportSend = 0;
                    Local.ReportTimeNum = 0;
                }
            }
            if (Local.ReportSend == 0) {
                if (Local.ReportTimeNum >= (Local.RandReportTime*TIMERPERSEC)) {
                    Local.ReportSend = 1;
                    TimeReportStatusFunc();
                }
            }
            Local.ReportTimeNum ++;
        }

		timenum ++;
		if(timenum > 0xFFFFFF)
		  timenum = 0;     
		usleep((TIMERTIME-10)*1000);
	}
}

void OnlineCheckFunc(void) //����ȷ�ϼ�⺯��
{
	unsigned char send_b[1520];
	int sendlength;
	if(Local.Timer1Num >= TIMERPERSEC*6)
	{
		if(Local.CallConfirmFlag == 0)
		{
			if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
						||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10)) //�Խ�
			{
				TalkEnd_Func();
				SendHostOrder(0x68, Local.DoorNo, NULL); //������������  �Է��һ�
			}
			if((Local.Status == 3)||(Local.Status == 4))  //����
			{
				WatchEnd_Func();
				SendHostOrder(0x61, Local.DoorNo, NULL); //������������  �Է�ֹͣ����
			}
			Local.OnlineFlag = 0;
			//ͨѶ�ж�
#ifdef _DEBUG
			printf("û���յ�����ȷ�ϣ�ǿ�ƽ���\n");
#endif
		}
		else
		  Local.CallConfirmFlag = 0;
		Local.Timer1Num = 0;
	}
	else
	  if((Local.Timer1Num %TIMERPERSEC)==0)
	  {
		  //�Խ�ʱ���з���������ȷ�ϰ���ÿ��һ��
		  //���ʱ���ط���������ȷ�ϰ���ÿ��һ��
		  if((Local.Status == 2)||(Local.Status == 6)
					  ||(Local.Status == 8)||(Local.Status == 10)
					  ||(Local.Status == 3))
		  {
			  //ͷ��
			  memcpy(send_b, UdpPackageHead, 6);
			  //����
			  if((Local.Status == 2)||(Local.Status == 6)||(Local.Status == 8)||(Local.Status == 10))  //�Խ�
				send_b[6] = VIDEOTALK;
			  if(Local.Status == 3)  //����
				send_b[6] = VIDEOWATCH;

			  send_b[7]=ASK;        //����
			  send_b[8]=CALLCONFIRM;//ͨ������ȷ��
			  //������
			  if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5)
						  ||(Local.Status == 7)||(Local.Status == 9))  //����Ϊ���з�
			  {
				  memcpy(send_b+9, LocalCfg.Addr, 20);
				  memcpy(send_b+29, LocalCfg.IP, 4);
				  memcpy(send_b+33, Remote.Addr[0], 20);
				  memcpy(send_b+53, Remote.IP[0], 4);
			  }
			  if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6)
						  ||(Local.Status == 8)||(Local.Status == 10))  //����Ϊ���з�
			  {
				  memcpy(send_b+9, Remote.Addr[0], 20);
				  memcpy(send_b+29, Remote.IP[0], 4);
				  memcpy(send_b+33, LocalCfg.Addr, 20);
				  memcpy(send_b+53, LocalCfg.IP, 4);
			  }
			  //ȷ�����
			  send_b[57] = (Local.OnlineNum & 0xFF000000) >> 24;
			  send_b[58] = (Local.OnlineNum & 0x00FF0000) >> 16;
			  send_b[59] = (Local.OnlineNum & 0x0000FF00) >> 8;
			  send_b[60] = Local.OnlineNum & 0x000000FF;
			  Local.OnlineNum ++;
			  sendlength=61;
			  sprintf(RemoteHost, "%d.%d.%d.%d",
						  Remote.DenIP[0],Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
			  UdpSendBuff(m_VideoSendSocket, RemoteHost, send_b , sendlength);
		  }
	  }
	Local.Timer1Num ++;
}

//---------------------------------------------------------------------------
void TimeReportStatusFunc(void) //�豸��ʱ����״̬����
{
	int i;
	//����������
	pthread_mutex_lock (&Local.udp_lock);
	//���ҿ��÷��ͻ��岢���
	for(i=0; i<UDPSENDMAX; i++)
	  if(Multi_Udp_Buff[i].isValid == 0)
	  {
		  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  Multi_Udp_Buff[i].SendNum = 0;   //×î¶à·¢6ŽÎ
		  Multi_Udp_Buff[i].m_Socket = m_DataSocket;
		  Multi_Udp_Buff[i].CurrOrder = 0;
		  //Í·²¿
		  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
		  //ÃüÁî
		  Multi_Udp_Buff[i].buf[6] = REPORTSTATUS;
		  Multi_Udp_Buff[i].buf[7] = ASK;    //Ö÷œÐ

		  memcpy(Multi_Udp_Buff[i].buf + 8, NullAddr, 20);
		  memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 20);

		  memcpy(Multi_Udp_Buff[i].buf + 28, NullAddr, 20);
		  memcpy(Multi_Udp_Buff[i].buf + 28, "Z0001", 5);
		  memcpy(Multi_Udp_Buff[i].buf + 48, LocalCfg.Mac_Addr, 6);
		  //	Multi_Udp_Buff[i].buf[34] = LocalCfg.DefenceStatus;
		  //	Multi_Udp_Buff[i].buf[35] = LocalCfg.DefenceNum;
		  //	for(k=0; k<(LocalCfg.DefenceNum*6); k++)
		  //		memcpy(Multi_Udp_Buff[i].buf + 36 + 10*k, LocalCfg.DefenceInfo[k], 10);

		  Multi_Udp_Buff[i].nlength = 54;
		  Multi_Udp_Buff[i].DelayTime = 100;
		  Multi_Udp_Buff[i].isValid = 1;
		  sem_post(&multi_send_sem);
		  break;
	  }
	//�򿪻�����
	pthread_mutex_unlock (&Local.udp_lock);
}
//---------------------------------------------------------------------------
void TalkCtrlFunc(void)  //�Խ����ƣ���ʾͨ��ʱ����жϳ�ʱ
{
	if((Local.TimeOut % TIMERPERSEC)==0)
	  switch(Local.Status)
	  {
		  case 1:  //���жԽ�
		  case 2:  //���жԽ�
			  if(Local.TimeOut >= CALLTIMEOUT)
			  {
				  //�鿴�Ƿ��������鲥����
				  DropMultiGroup(m_VideoSocket, NULL);
				  //���г�ʱ
				  CallTimeOut_Func();
				  //���г�ʱ
				  SendHostOrder(0x66, Local.DoorNo, NULL); //������������  ���г�ʱ
			  }
			  break;
		  case 5:  //����ͨ��
		  case 6:  //����ͨ��
			  //��ʱ
			  if(Local.TimeOut >= TALKTIMEOUT)
			  {
				  TalkEnd_Func();
				  Local.OnlineFlag = 0;
				  //ͨ������
				  SendHostOrder(0x68, Local.DoorNo, NULL); //������������  �Է�ֹͣͨ��
#ifdef _DEBUG
				  printf("ͨ����ʱ\n");
#endif
			  }
			  break;
		  case 3:  //����    ��ʱ
		  case 4:  //������
			  if(Local.TimeOut >= WATCHTIMEOUT)
			  {
				  WatchEnd_Func();
				  Local.OnlineFlag = 0;
				  //���ӽ���
				  SendHostOrder(0x61, Local.DoorNo, NULL); //������������  �Է�ֹͣ����
#ifdef _DEBUG
				  printf("���ӳ�ʱ\n");
#endif
			  }
			  break;
	  }
	Local.TimeOut ++;       //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
}
//---------------------------------------------------------------------------
void SendAlarmFunc(unsigned char doorno, unsigned char AlarmByte) 
		//void SendAlarmFunc(unsigned char SpecialByte, unsigned char AlarmByte) 
{
	int i;
	char TempName[50];
	char TempTime[32];
	time_t t;
	struct tm *tm_t;
	sprintf(TempName,"��Ԫ��%c���𾯱�",doorno);
	time(&t);
	tm_t=localtime(&t);
	sprintf(TempTime, "%02d-%02d %02d:%02d:%02d", tm_t->tm_mon+1,
				tm_t->tm_mday, tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);

	//Ëø¶š»¥³âËø
	pthread_mutex_lock (&Local.udp_lock);
	//²éÕÒ¿ÉÓÃ·¢ËÍ»º³å²¢Ìî¿Õ
	for (i=0; i<UDPSENDMAX; i++)
	  if (Multi_Udp_Buff[i].isValid == 0)
	  {   
		  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  //            printf("\nalarmserver: %d.%d.%d.%d\n",LocalCfg.IP_Server[0],
		  //                 LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  printf("Multi_Udp_Buff[%d].RemoteHost=%s\n",i,Multi_Udp_Buff[i].RemoteHost);
		  Multi_Udp_Buff[i].SendNum = 4;   //×î¶à·¢6ŽÎ
		  Multi_Udp_Buff[i].m_Socket = m_DataSocket;
		  Multi_Udp_Buff[i].CurrOrder = 0;
		  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
		  //ÃüÁî
		  Multi_Udp_Buff[i].buf[6] = ALARM;
		  Multi_Udp_Buff[i].buf[7] = ASK;    //Ö÷œÐ
		  /////////////////////////////////////paul0620///////////////////////////
		  memset(Multi_Udp_Buff[i].buf + 8, 0x30, 20); //���е�ַ����
		  memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 12);

		  memset(Multi_Udp_Buff[i].buf + 28, 0x30, 20); //���е�ַ����
		  memcpy(Multi_Udp_Buff[i].buf + 28, "Z0001", 5);
		  memcpy(Multi_Udp_Buff[i].buf + 48, NullAddr, 8);
		  memcpy(Multi_Udp_Buff[i].buf + 48, LocalCfg.Mac_Addr, 6);

		  Multi_Udp_Buff[i].buf[54] = 0;//LocalCfg.DefenceStatus;     //�ն�״̬
		  Multi_Udp_Buff[i].buf[55] = 1;                //��������

		  Multi_Udp_Buff[i].buf[56] = AlarmByte+1;        //�������

		  memcpy(Multi_Udp_Buff[i].buf+57,TempName,50);
		  memcpy(Multi_Udp_Buff[i].buf+107,TempTime,30);


		  memset(Multi_Udp_Buff[i].buf+137,0,8); //Ԥ����8���ֽ�

		  Multi_Udp_Buff[i].nlength = 145;
		  Multi_Udp_Buff[i].DelayTime = 100;
		  Multi_Udp_Buff[i].isValid = 1;

		  sem_post(&multi_send_sem);
		  break;
	  }
	//Žò¿ª»¥³âËø
	pthread_mutex_unlock (&Local.udp_lock);
	//////����
}
void SendRobAlarm(unsigned char doorno, char Addr[21])
{
	int i;
	char name[50];
	char timing[30];
	time_t t;
	struct tm *tm_t;
	time(&t);
	tm_t=localtime(&t);
	printf("SENDING ROB\n");
	sprintf(timing, "%02d-%02d %02d:%02d:%02d", tm_t->tm_mon+1,
				tm_t->tm_mday, tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);
	sprintf(name,"��Ԫ��%c����",doorno);	
	pthread_mutex_lock (&Local.udp_lock);
	for (i=0; i<UDPSENDMAX; i++)
	  if (Multi_Udp_Buff[i].isValid == 0)
	  {
		  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  printf("\nalarmserver: %d.%d.%d.%d\n",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);

		  printf("doorno = %d, Addr = %s\n", doorno,Addr);

		  Multi_Udp_Buff[i].SendNum = 4;   //
		  Multi_Udp_Buff[i].m_Socket = m_DataSocket;
		  Multi_Udp_Buff[i].CurrOrder = 0;
		  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
		  //
		  Multi_Udp_Buff[i].buf[6] = ALARM;
		  Multi_Udp_Buff[i].buf[7] = ASK;    //
		  ///////////////////////////////paul0401 LocalCfg.Addr
		  //				memset(Multi_Udp_Buff[i].buf + 8,0x30,20);

		  memset(Multi_Udp_Buff[i].buf + 28, 0x30, 40); 
		  memcpy(Multi_Udp_Buff[i].buf + 8, Addr, 20);
		  memcpy(Multi_Udp_Buff[i].buf + 28, "Z0001", 5);


		  ////////////////////////////////////////////////paul0425
		  memcpy(Multi_Udp_Buff[i].buf + 48, LocalCfg.Mac_Addr, 6);
		  Multi_Udp_Buff[i].buf[54] = 5;//LocalCfg.DefenceStatus;     //
		  Multi_Udp_Buff[i].buf[55] = 1;//SpecialByte;                //
		  Multi_Udp_Buff[i].buf[56] = 1;// LocalCfg.DefenceNum;        //
		  memcpy(Multi_Udp_Buff[i].buf+57,name,50);
		  memcpy(Multi_Udp_Buff[i].buf+107,timing,30);
		  memset(Multi_Udp_Buff[i].buf+137,0,8); //Ԥ����8���ֽ�

		  /*/////////////////
			Multi_Udp_Buff[i].buf[37] = 0;//AlarmByte;
			Multi_Udp_Buff[i].buf[38] = 0x01;
		  //////////////////
		  Multi_Udp_Buff[i].buf[39] = doorno;
		  Multi_Udp_Buff[i].buf[40] = 0x00;

		  Multi_Udp_Buff[i].nlength = 41;
		  */

		  Multi_Udp_Buff[i].nlength = 145;
		  Multi_Udp_Buff[i].DelayTime = 100;
		  Multi_Udp_Buff[i].isValid = 1;
		  sem_post(&multi_send_sem);
		  break;
	  }

	pthread_mutex_unlock (&Local.udp_lock);

}
void SendDoorAlarm(unsigned char doorno, char Addr[21])
{
	int i;
	char name[50];
	char timing[30];
	time_t t;
	struct tm *tm_t;
	time(&t);
	tm_t=localtime(&t);
	printf("SENDING DOORALARM\n");
	sprintf(timing, "%02d-%02d %02d:%02d:%02d", tm_t->tm_mon+1,
				tm_t->tm_mday, tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);
	sprintf(name,"��Ԫ��%c�Ŵű���",doorno);	
	pthread_mutex_lock (&Local.udp_lock);
	for (i=0; i<UDPSENDMAX; i++)
	  if (Multi_Udp_Buff[i].isValid == 0)
	  {
		  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  printf("\nalarmserver: %d.%d.%d.%d\n",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  Multi_Udp_Buff[i].SendNum = 4;   //
		  Multi_Udp_Buff[i].m_Socket = m_DataSocket;
		  Multi_Udp_Buff[i].CurrOrder = 0;
		  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
		  Multi_Udp_Buff[i].buf[6] = ALARM;
		  Multi_Udp_Buff[i].buf[7] = ASK;    //
		  memset(Multi_Udp_Buff[i].buf + 28, 0x30, 40); 
		  memset(Multi_Udp_Buff[i].buf + 8, 0x30, 20); 
		  memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 11);
		  memcpy(Multi_Udp_Buff[i].buf + 28, "Z0001", 5);
		  memcpy(Multi_Udp_Buff[i].buf + 48, LocalCfg.Mac_Addr, 6);
		  Multi_Udp_Buff[i].buf[54] = 5;//LocalCfg.DefenceStatus;     //
		  Multi_Udp_Buff[i].buf[55] = 1;//SpecialByte;                //
		  Multi_Udp_Buff[i].buf[56] = 1;// LocalCfg.DefenceNum;        //
		  memcpy(Multi_Udp_Buff[i].buf+57,name,50);
		  memcpy(Multi_Udp_Buff[i].buf+107,timing,30);
		  memset(Multi_Udp_Buff[i].buf+137,0,8); //Ԥ����8���ֽ�
		  Multi_Udp_Buff[i].nlength = 145;
		  Multi_Udp_Buff[i].DelayTime = 100;
		  Multi_Udp_Buff[i].isValid = 1;
		  sem_post(&multi_send_sem);
		  break;
	  }
	pthread_mutex_unlock (&Local.udp_lock);
}
#if 1
void SendPowerAlarm(unsigned char doorno, char Addr[21])
{
	int i;
	char name[50];
	char timing[30];
	time_t t;
	struct tm *tm_t;
	time(&t);
	tm_t=localtime(&t);
	printf("SENDING POWERALARM\n");
	sprintf(timing, "%02d-%02d %02d:%02d:%02d", tm_t->tm_mon+1,
				tm_t->tm_mday, tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);
	sprintf(name,"��Ԫ��%c��Դ%d����",Local.DoorNo,doorno+1);	
	pthread_mutex_lock (&Local.udp_lock);
	for (i=0; i<UDPSENDMAX; i++)
	  if (Multi_Udp_Buff[i].isValid == 0)
	  {
		  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  printf("\nalarmserver: %d.%d.%d.%d\n",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  Multi_Udp_Buff[i].SendNum = 4;   //
		  Multi_Udp_Buff[i].m_Socket = m_DataSocket;
		  Multi_Udp_Buff[i].CurrOrder = 0;
		  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
		  Multi_Udp_Buff[i].buf[6] = ALARM;
		  Multi_Udp_Buff[i].buf[7] = ASK;    //
		  memset(Multi_Udp_Buff[i].buf + 28, 0x30, 40); 
		  memset(Multi_Udp_Buff[i].buf + 8, 0x30, 20); 
		  memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 11);
		  memcpy(Multi_Udp_Buff[i].buf + 28, "Z0001", 5);
		  memcpy(Multi_Udp_Buff[i].buf + 48, LocalCfg.Mac_Addr, 6);
		  Multi_Udp_Buff[i].buf[54] = 5;//LocalCfg.DefenceStatus;     //
		  Multi_Udp_Buff[i].buf[55] = 1;//SpecialByte;                //
		  Multi_Udp_Buff[i].buf[56] = 1;// LocalCfg.DefenceNum;        //
		  memcpy(Multi_Udp_Buff[i].buf+57,name,50);
		  memcpy(Multi_Udp_Buff[i].buf+107,timing,30);
		  memset(Multi_Udp_Buff[i].buf+137,0,8); //Ԥ����8���ֽ�
		  Multi_Udp_Buff[i].nlength = 145;
		  Multi_Udp_Buff[i].DelayTime = 100;
		  Multi_Udp_Buff[i].isValid = 1;
		  sem_post(&multi_send_sem);
		  break;
	  }
	pthread_mutex_unlock (&Local.udp_lock);
}
#endif
