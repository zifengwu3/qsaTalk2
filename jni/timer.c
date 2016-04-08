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

//键盘扫描
void OnlineCheckFunc(void); //在线确认检测函数
void TimeReportStatusFunc(void); //设备定时报告状态函数

void TalkCtrlFunc(void);  //对讲控制，显示通话时间和判断超时

int timer_thread_flag;
pthread_t timer_thread;
void timer_rcv_thread_task(void);


void timer_rcv_thread_task(void)
{
	int timenum;  //计数

#ifdef _DEBUG
	printf("创建timer接收线程：\n" );
	printf("timer_rcv_flag=%d\n",timer_rcv_flag);
#endif

	timenum = 0;
	while (timer_rcv_flag)
	{
        //连续6秒没有收到在线确认，则认为断线
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

void OnlineCheckFunc(void) //在线确认检测函数
{
	unsigned char send_b[1520];
	int sendlength;
	if(Local.Timer1Num >= TIMERPERSEC*6)
	{
		if(Local.CallConfirmFlag == 0)
		{
			if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
						||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10)) //对讲
			{
				TalkEnd_Func();
				SendHostOrder(0x68, Local.DoorNo, NULL); //发送主动命令  对方挂机
			}
			if((Local.Status == 3)||(Local.Status == 4))  //监视
			{
				WatchEnd_Func();
				SendHostOrder(0x61, Local.DoorNo, NULL); //发送主动命令  对方停止监视
			}
			Local.OnlineFlag = 0;
			//通讯中断
#ifdef _DEBUG
			printf("没有收到在线确认，强制结束\n");
#endif
		}
		else
		  Local.CallConfirmFlag = 0;
		Local.Timer1Num = 0;
	}
	else
	  if((Local.Timer1Num %TIMERPERSEC)==0)
	  {
		  //对讲时被叫方发送在线确认包，每秒一个
		  //监控时主控方发送在线确认包，每秒一个
		  if((Local.Status == 2)||(Local.Status == 6)
					  ||(Local.Status == 8)||(Local.Status == 10)
					  ||(Local.Status == 3))
		  {
			  //头部
			  memcpy(send_b, UdpPackageHead, 6);
			  //命令
			  if((Local.Status == 2)||(Local.Status == 6)||(Local.Status == 8)||(Local.Status == 10))  //对讲
				send_b[6] = VIDEOTALK;
			  if(Local.Status == 3)  //监视
				send_b[6] = VIDEOWATCH;

			  send_b[7]=ASK;        //主叫
			  send_b[8]=CALLCONFIRM;//通话在线确认
			  //子命令
			  if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5)
						  ||(Local.Status == 7)||(Local.Status == 9))  //本机为主叫方
			  {
				  memcpy(send_b+9, LocalCfg.Addr, 20);
				  memcpy(send_b+29, LocalCfg.IP, 4);
				  memcpy(send_b+33, Remote.Addr[0], 20);
				  memcpy(send_b+53, Remote.IP[0], 4);
			  }
			  if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6)
						  ||(Local.Status == 8)||(Local.Status == 10))  //本机为被叫方
			  {
				  memcpy(send_b+9, Remote.Addr[0], 20);
				  memcpy(send_b+29, Remote.IP[0], 4);
				  memcpy(send_b+33, LocalCfg.Addr, 20);
				  memcpy(send_b+53, LocalCfg.IP, 4);
			  }
			  //确认序号
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
void TimeReportStatusFunc(void) //设备定时报告状态函数
{
	int i;
	//锁定互斥锁
	pthread_mutex_lock (&Local.udp_lock);
	//查找可用发送缓冲并填空
	for(i=0; i<UDPSENDMAX; i++)
	  if(Multi_Udp_Buff[i].isValid == 0)
	  {
		  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  Multi_Udp_Buff[i].SendNum = 0;   //脳卯露脿路垄6沤脦
		  Multi_Udp_Buff[i].m_Socket = m_DataSocket;
		  Multi_Udp_Buff[i].CurrOrder = 0;
		  //脥路虏驴
		  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
		  //脙眉脕卯
		  Multi_Udp_Buff[i].buf[6] = REPORTSTATUS;
		  Multi_Udp_Buff[i].buf[7] = ASK;    //脰梅艙脨

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
	//打开互斥锁
	pthread_mutex_unlock (&Local.udp_lock);
}
//---------------------------------------------------------------------------
void TalkCtrlFunc(void)  //对讲控制，显示通话时间和判断超时
{
	if((Local.TimeOut % TIMERPERSEC)==0)
	  switch(Local.Status)
	  {
		  case 1:  //主叫对讲
		  case 2:  //被叫对讲
			  if(Local.TimeOut >= CALLTIMEOUT)
			  {
				  //查看是否在其它组播组内
				  DropMultiGroup(m_VideoSocket, NULL);
				  //呼叫超时
				  CallTimeOut_Func();
				  //呼叫超时
				  SendHostOrder(0x66, Local.DoorNo, NULL); //发送主动命令  呼叫超时
			  }
			  break;
		  case 5:  //主叫通话
		  case 6:  //被叫通话
			  //计时
			  if(Local.TimeOut >= TALKTIMEOUT)
			  {
				  TalkEnd_Func();
				  Local.OnlineFlag = 0;
				  //通话结束
				  SendHostOrder(0x68, Local.DoorNo, NULL); //发送主动命令  对方停止通话
#ifdef _DEBUG
				  printf("通话超时\n");
#endif
			  }
			  break;
		  case 3:  //监视    计时
		  case 4:  //被监视
			  if(Local.TimeOut >= WATCHTIMEOUT)
			  {
				  WatchEnd_Func();
				  Local.OnlineFlag = 0;
				  //监视结束
				  SendHostOrder(0x61, Local.DoorNo, NULL); //发送主动命令  对方停止监视
#ifdef _DEBUG
				  printf("监视超时\n");
#endif
			  }
			  break;
	  }
	Local.TimeOut ++;       //监视超时,  通话超时,  呼叫超时，无人接听
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
	sprintf(TempName,"单元门%c防拆警报",doorno);
	time(&t);
	tm_t=localtime(&t);
	sprintf(TempTime, "%02d-%02d %02d:%02d:%02d", tm_t->tm_mon+1,
				tm_t->tm_mday, tm_t->tm_hour, tm_t->tm_min, tm_t->tm_sec);

	//脣酶露拧禄楼鲁芒脣酶
	pthread_mutex_lock (&Local.udp_lock);
	//虏茅脮脪驴脡脫脙路垄脣脥禄潞鲁氓虏垄脤卯驴脮
	for (i=0; i<UDPSENDMAX; i++)
	  if (Multi_Udp_Buff[i].isValid == 0)
	  {   
		  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",LocalCfg.IP_Server[0],
					  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  //            printf("\nalarmserver: %d.%d.%d.%d\n",LocalCfg.IP_Server[0],
		  //                 LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
		  printf("Multi_Udp_Buff[%d].RemoteHost=%s\n",i,Multi_Udp_Buff[i].RemoteHost);
		  Multi_Udp_Buff[i].SendNum = 4;   //脳卯露脿路垄6沤脦
		  Multi_Udp_Buff[i].m_Socket = m_DataSocket;
		  Multi_Udp_Buff[i].CurrOrder = 0;
		  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
		  //脙眉脕卯
		  Multi_Udp_Buff[i].buf[6] = ALARM;
		  Multi_Udp_Buff[i].buf[7] = ASK;    //脰梅艙脨
		  /////////////////////////////////////paul0620///////////////////////////
		  memset(Multi_Udp_Buff[i].buf + 8, 0x30, 20); //主叫地址编码
		  memcpy(Multi_Udp_Buff[i].buf + 8, LocalCfg.Addr, 12);

		  memset(Multi_Udp_Buff[i].buf + 28, 0x30, 20); //被叫地址编码
		  memcpy(Multi_Udp_Buff[i].buf + 28, "Z0001", 5);
		  memcpy(Multi_Udp_Buff[i].buf + 48, NullAddr, 8);
		  memcpy(Multi_Udp_Buff[i].buf + 48, LocalCfg.Mac_Addr, 6);

		  Multi_Udp_Buff[i].buf[54] = 0;//LocalCfg.DefenceStatus;     //终端状态
		  Multi_Udp_Buff[i].buf[55] = 1;                //报警个数

		  Multi_Udp_Buff[i].buf[56] = AlarmByte+1;        //防区编号

		  memcpy(Multi_Udp_Buff[i].buf+57,TempName,50);
		  memcpy(Multi_Udp_Buff[i].buf+107,TempTime,30);


		  memset(Multi_Udp_Buff[i].buf+137,0,8); //预留的8个字节

		  Multi_Udp_Buff[i].nlength = 145;
		  Multi_Udp_Buff[i].DelayTime = 100;
		  Multi_Udp_Buff[i].isValid = 1;

		  sem_post(&multi_send_sem);
		  break;
	  }
	//沤貌驴陋禄楼鲁芒脣酶
	pthread_mutex_unlock (&Local.udp_lock);
	//////防拆
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
	sprintf(name,"单元门%c防劫",doorno);	
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
		  memset(Multi_Udp_Buff[i].buf+137,0,8); //预留的8个字节

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
	sprintf(name,"单元门%c门磁报警",doorno);	
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
		  memset(Multi_Udp_Buff[i].buf+137,0,8); //预留的8个字节
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
	sprintf(name,"单元门%c电源%d报警",Local.DoorNo,doorno+1);	
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
		  memset(Multi_Udp_Buff[i].buf+137,0,8); //预留的8个字节
		  Multi_Udp_Buff[i].nlength = 145;
		  Multi_Udp_Buff[i].DelayTime = 100;
		  Multi_Udp_Buff[i].isValid = 1;
		  sem_post(&multi_send_sem);
		  break;
	  }
	pthread_mutex_unlock (&Local.udp_lock);
}
#endif
