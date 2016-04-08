#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>       //sem_t

#define CommonH
#include "common.h"

extern char sPath[80];
extern char NullAddr[21];   //空字符串
//paul2.x
extern char PriMulCast[20];
int tmpip[3];
void Call_Func(int uFlag, char *call_addr);    //呼叫   1  中心  2 住户
void FindEquip(char *call_addr);    //查找设备
void TalkEnd_Func(void);
void WatchEnd_Func(void);
void CallTimeOut_Func(void); //呼叫超时
//---------------------------------------------------------------------------
void Call_Func(int uFlag, char *call_addr)    //呼叫   1  中心  2 住户
{
	int i,j;
	char Addr[20];
	int MaxDen = 1;
	char str[30];
	time_t t;
	time(&t);
	Local.call_tm_t = localtime(&t);
	///Local.Loading = 10;
	//zhou101102//保存通话时间
	sprintf(str,"%04d%02d%02d%02d%02d%02d",
				Local.call_tm_t->tm_year+1900,
				Local.call_tm_t->tm_mon + 1,
				Local.call_tm_t->tm_mday,
				Local.call_tm_t->tm_hour,
				Local.call_tm_t->tm_min,
				Local.call_tm_t->tm_sec);
	str[14] = 0x30;
	str[15] = '\0';

	SendTalkInfoFlag = 0;//zhou101102
	strcpy(SendTalkInfo.StartTime,str);
	printf("SendTalkInfo.StartTime is %s\n",SendTalkInfo.StartTime);

	if((Local.Status == 0)&&((uFlag == 1)||(uFlag == 2)))
	{
		if(uFlag == 2)
		  MaxDen = 4;
		for(i = 0; i < MaxDen; i++)
		  TalkDen.ExistFlag[i] = 0;
		memcpy(Addr,NullAddr,20);
		j=0;
		//锁定互斥锁
		pthread_mutex_lock (&Local.udp_lock);
		//查找可用发送缓冲并填空
		for(i=0; i<UDPSENDMAX; i++)
		  if(Multi_Udp_Buff[i].isValid == 0)
		  {
			  Multi_Udp_Buff[i].SendNum = 0;
			  Multi_Udp_Buff[i].m_Socket = m_VideoSendSocket;
			  Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
			  //   sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",LocalCfg.IP_Broadcast[0],
			  //           LocalCfg.IP_Broadcast[1],LocalCfg.IP_Broadcast[2],LocalCfg.IP_Broadcast[3]);

			  strcpy(Multi_Udp_Buff[i].RemoteHost, NSMULTIADDR);
			  //头部
			  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			  //命令  ,子网多播查找
			  Multi_Udp_Buff[i].buf[6] = NSORDER;
			  Multi_Udp_Buff[i].buf[7] = ASK;    //主叫

			  memcpy(Multi_Udp_Buff[i].buf+8,LocalCfg.Addr,20);
			  memcpy(Multi_Udp_Buff[i].buf+28,LocalCfg.IP,4);
			  memcpy(Remote.Addr[0], NullAddr, 20);
			  switch(uFlag)
			  {
				  case 1: //呼叫中心
					  if(call_addr[1]==0x30)
					  {
						  Addr[0]='Z';
					  }
					  else if(call_addr[1]==0x31)//call the fence
					  {
						  Addr[0]='W';
					  }
					  Addr[3]=call_addr[2];
					  Addr[4]=call_addr[3];
					  memcpy(Remote.Addr[0],Addr,12);
					  j =  MaxDen;
					  break;
				  case 2: //呼叫住户
					  switch(LocalCfg.Addr[0])
					  {
						  case 'M':
							  memcpy(Remote.Addr[0] , LocalCfg.Addr , 20);
							  Remote.Addr[0][0] = 'S';
							  memcpy(Remote.Addr[0] + 7, call_addr, 4);
							  Remote.Addr[0][11] = '0'+j;
							  printf("CALL %d buf%d\n",j,i);
							  ///paul2.x
							  tmpip[0]=(LocalCfg.Addr[5]-'0')*10+(LocalCfg.Addr[6]-'0');
							  tmpip[1]=(call_addr[0]-'0')*10+(call_addr[1]-'0');
							  tmpip[2]=(call_addr[2]-'0')*10+(call_addr[3]-'0');
							  sprintf(PriMulCast,"239.%d.%d.%d",tmpip[0],tmpip[1],tmpip[2]);

							  printf("CALL %s,brocast:%s\n",Remote.Addr[0],Multi_Udp_Buff[i].RemoteHost);
							  break;
						  case 'W':
							  if(strlen(call_addr) == 4) //别墅
							  {
								  Remote.Addr[0][0] = 'B';
								  memcpy(Remote.Addr[0] + 1, call_addr, 4);
								  Remote.Addr[0][11] = '0'+j;
							  }
							  else
							  {
								  Remote.Addr[0][0] = 'S';
								  memcpy(Remote.Addr[0] + 1, call_addr, 10);
								  Remote.Addr[0][11] = '0'+j;
							  }
							  break;
					  }
					  break;
				  case 3:
					  break;
				  default:
					  break;
			  }
			  memcpy(Multi_Udp_Buff[i].buf+32,Remote.Addr[0],20);
			  Remote.IP[0][0] = 0;
			  Remote.IP[0][1] = 0;
			  Remote.IP[0][2] = 0;
			  Remote.IP[0][3] = 0;
			  memcpy(Multi_Udp_Buff[i].buf+52,Remote.IP[0],4);

			  Multi_Udp_Buff[i].nlength = 56;
			  Multi_Udp_Buff[i].DelayTime = 100;
			  Multi_Udp_Buff[i].isValid = 1;
			  //保存呼叫地址//zhou101102
			  memcpy(SendTalkInfo.Addr,Remote.Addr[0],20);
			  SendTalkInfo.Addr[20] = '\0';

#ifdef _DEBUG
			  printf("Looking for the IP via Addr\n");
#endif
			  sem_post(&multi_send_sem);
			  break;
		  }
		//打开互斥锁
		pthread_mutex_unlock (&Local.udp_lock);
    } else {
        LOGD("I'm is Busy!\n");
    }

	return;
}
//---------------------------------------------------------------------------
void FindEquip(char *call_addr)    //查找设备
{
	int i;
	if(Local.Status == 0)
	{
		//锁定互斥锁
		pthread_mutex_lock (&Local.udp_lock);
		//查找可用发送缓冲并填空
		for(i=0; i<UDPSENDMAX; i++)
			if(Multi_Udp_Buff[i].isValid == 0)
			{
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSendSocket;
				Multi_Udp_Buff[i].CurrOrder = FINDEQUIP;
				// sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",LocalCfg.IP_Broadcast[0],
				//         LocalCfg.IP_Broadcast[1],LocalCfg.IP_Broadcast[2],LocalCfg.IP_Broadcast[3]);
				strcpy(Multi_Udp_Buff[i].RemoteHost, NSMULTIADDR);
				//头部
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				//命令  ,子网多播查找
				Multi_Udp_Buff[i].buf[6] = NSORDER;
				Multi_Udp_Buff[i].buf[7] = ASK;    //主叫

				memcpy(Multi_Udp_Buff[i].buf+8,LocalCfg.Addr,20);
				memcpy(Multi_Udp_Buff[i].buf+28,LocalCfg.IP,4);
				memcpy(Remote.Addr[0], NullAddr, 20);

				switch(LocalCfg.Addr[0])
				{
					case 'M':
						Remote.Addr[0][0] = 'S';
						memcpy(Remote.Addr[0] + 1, LocalCfg.Addr + 1, 6);
						memcpy(Remote.Addr[0] + 7, call_addr, 4);
						Remote.Addr[0][11] = '0';
						break;
					case 'W':
						if(strlen(call_addr) == 4) //别墅
						{
							Remote.Addr[0][0] = 'B';
							memcpy(Remote.Addr[0] + 1, call_addr, 4);
							Remote.Addr[0][11] = '0';
						}
						else
						{
							Remote.Addr[0][0] = 'S';
							memcpy(Remote.Addr[0] + 1, call_addr, 10);
							Remote.Addr[0][11] = '0';
						}
						break;
				}

				memcpy(Multi_Udp_Buff[i].buf+32,Remote.Addr[0],20);
				Remote.IP[0][0] = 0;
				Remote.IP[0][1] = 0;
				Remote.IP[0][2] = 0;
				Remote.IP[0][3] = 0;
				memcpy(Multi_Udp_Buff[i].buf+52,Remote.IP[0],4);

				Multi_Udp_Buff[i].nlength = 56;
				Multi_Udp_Buff[i].DelayTime = 100;
				Multi_Udp_Buff[i].isValid = 1;
#ifdef _DEBUG
				printf("正在查找地址\n");
#endif
				sem_post(&multi_send_sem);
				break;
			}
		//打开互斥锁
		pthread_mutex_unlock (&Local.udp_lock);
	}
	else
#ifdef _DEBUG
		printf("本机正忙,无法查找\n");
#endif
}
//---------------------------------------------------------------------------
void TalkEnd_Func(void)
{
	int i,j;
	int MaxDen;
	//////paul0302night
	StopPlayWavFile();  //关闭回铃音  
#ifdef _DEBUG
	printf("%d: Local.Status = %d\n", __LINE__, Local.Status);
#endif
	if((Local.Status == 1)||(Local.Status == 2)||(Local.Status == 5)||(Local.Status == 6)
			||(Local.Status == 7)||(Local.Status == 8)||(Local.Status == 9)||(Local.Status == 10))  //状态为对讲
	{
#ifdef _DEBUG
		printf("Remote.DenNum=%d\n",Remote.DenNum);
#endif
		//锁定互斥锁
		pthread_mutex_lock (&Local.udp_lock);
		j=0;
		for(i=0; i<UDPSENDMAX; i++)
		  if(Multi_Udp_Buff[i].isValid == 0)
		  {
			  //头部
			  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			  //命令  
			  Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
			  Multi_Udp_Buff[i].buf[7] = ASK;    //主叫
			  Multi_Udp_Buff[i].buf[8] = CALLEND;    //CALLEND
			  Multi_Udp_Buff[i].SendNum = 0;
			  Multi_Udp_Buff[i].m_Socket = m_VideoSendSocket;
			  Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
			  //			sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",Remote.IP[j][0],
			  //					Remote.IP[j][1],Remote.IP[j][2],Remote.IP[j][3]);
              /////paul2.x
              if ((Local.Status == 1) && (TurnToCenter == 0) 
                      && ((Remote.Addr[0][0]=='S') || (Remote.Addr[0][0]=='s')))
              {
                  strcpy(Multi_Udp_Buff[i].RemoteHost,PriMulCast);
              }
              else
              {
                  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",Remote.IP[j][0],
                          Remote.IP[j][1],Remote.IP[j][2],Remote.IP[j][3]);
              }

			  printf("\n%d:%d.%d.%d.%d\n", __LINE__, Remote.IP[j][0],
						  Remote.IP[j][1],Remote.IP[j][2],Remote.IP[j][3]);

			  //本机为主叫方
			  if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5)||(Local.Status == 7)||(Local.Status == 9))
			  {
				  memcpy(Multi_Udp_Buff[i].buf+9,LocalCfg.Addr,20);
				  memcpy(Multi_Udp_Buff[i].buf+29,LocalCfg.IP,4);
				  memcpy(Multi_Udp_Buff[i].buf+33,Remote.Addr[j],20);
				  memcpy(Multi_Udp_Buff[i].buf+53,Remote.IP[j],4);
			  }
			  //本机为被叫方
			  if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6)||(Local.Status == 8)||(Local.Status == 10))
			  {
				  memcpy(Multi_Udp_Buff[i].buf+9,Remote.Addr[j],20);
				  memcpy(Multi_Udp_Buff[i].buf+29,Remote.IP[j],4);
				  memcpy(Multi_Udp_Buff[i].buf+33,LocalCfg.Addr,20);
				  memcpy(Multi_Udp_Buff[i].buf+53,LocalCfg.IP,4);
			  }

			  Multi_Udp_Buff[i].nlength = 57;
			  Multi_Udp_Buff[i].DelayTime = 100;
			  Multi_Udp_Buff[i].isValid = 1;
			  sem_post(&multi_send_sem);
			  //                Local.Status = 0;  //状态为空闲
			  break;
		  }
		//打开互斥锁
		pthread_mutex_unlock (&Local.udp_lock);

	}
}
//---------------------------------------------------------------------------
void WatchEnd_Func(void)
{
	int i; 
	if((Local.Status == 3)||(Local.Status == 4))  //状态为监视
	{
		for(i=0; i<UDPSENDMAX; i++)
			if(Multi_Udp_Buff[i].isValid == 0)
			{
				//锁定互斥锁
				pthread_mutex_lock (&Local.udp_lock);
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSendSocket;
				Multi_Udp_Buff[i].CurrOrder = VIDEOWATCH;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",Remote.DenIP[0],
						Remote.DenIP[1],Remote.DenIP[2],Remote.DenIP[3]);
				//头部
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				//命令 
				Multi_Udp_Buff[i].buf[6] = VIDEOWATCH;
				Multi_Udp_Buff[i].buf[7] = ASK;    //主叫
				Multi_Udp_Buff[i].buf[8] = CALLEND;    //CALLEND

				//本机为主叫方
				if((Local.Status == 1)||(Local.Status == 3)||(Local.Status == 5)||(Local.Status == 7)||(Local.Status == 9))
				{
					memcpy(Multi_Udp_Buff[i].buf+9,LocalCfg.Addr,20);
					memcpy(Multi_Udp_Buff[i].buf+29,LocalCfg.IP,4);
					memcpy(Multi_Udp_Buff[i].buf+33,Remote.Addr[0],20);
					memcpy(Multi_Udp_Buff[i].buf+53,Remote.IP[0],4);
				}
				//本机为被叫方
				if((Local.Status == 2)||(Local.Status == 4)||(Local.Status == 6)||(Local.Status == 8)||(Local.Status == 10))
				{
					memcpy(Multi_Udp_Buff[i].buf+9,Remote.Addr[0],20);
					memcpy(Multi_Udp_Buff[i].buf+29,Remote.IP[0],4);
					memcpy(Multi_Udp_Buff[i].buf+33,LocalCfg.Addr,20);
					memcpy(Multi_Udp_Buff[i].buf+53,LocalCfg.IP,4);
				}

				Multi_Udp_Buff[i].nlength = 57;
				Multi_Udp_Buff[i].DelayTime = 100;
				Multi_Udp_Buff[i].isValid = 1;

				//                Local.Status = 0;  //状态为空闲
				//打开互斥锁
				pthread_mutex_unlock (&Local.udp_lock);
				sem_post(&multi_send_sem);
				break;
			}
	}

	StopRecVideo();
	printf("WATCHEND STATUS=0\n");
	Local.Status = 0;  //状态为空闲
#ifdef _DEBUG
	printf("本机结束监视\n");
#endif
}
//---------------------------------------------------------------------------
//呼叫超时
void CallTimeOut_Func(void)
{
	int i;
	TalkEnd_Func();
	Local.OnlineFlag = 0;
	SendHostOrder(0x66, Local.DoorNo, NULL); //发送主动命令  呼叫超时
#ifdef _DEBUG
	printf("呼叫超时\n");
#endif
}
//---------------------------------------------------------------------------
////////////////////////////////////
//zhou101102 发送通话信息给中心机
//---------------------------------------------------------------------------
void SendInfoToCenter_Func(void)  //向中心发送通话信息
{
	int i;
	char str[10];
	int length;
	//	time_t t;
	//	time(&t);
	//	Local.call_tm_t = localtime(&t);//当前时间

//	if(Local.Status == 0 || Local.Status == 21 || Local.Status == 22)
	{
		printf("SendTalkInfo.Status = %d \nSendTalkInfo.Duration = %d \nSendTalkInfo.Addr = %s \nSendTalkInfo.StartTime = %s \n"
					,SendTalkInfo.Status,SendTalkInfo.Duration,SendTalkInfo.Addr,SendTalkInfo.StartTime);
//		printf("Multi_Udp_Buff[i].isValid is %d\n",Multi_Udp_Buff[i].isValid);
//		usleep(1000*1000);
		pthread_mutex_lock (&Local.udp_lock);
//		printf("Testing tag is No.1\n");
		//打开互斥锁
		for(i=0; i<UDPSENDMAX; i++)
		  if(Multi_Udp_Buff[i].isValid == 0)
		  {
//			  printf("Testing tag is No.2\n");
			  ///0803Multi_Udp_Buff[i].SendNum = 0;
			  Multi_Udp_Buff[i].SendNum = 4;//发2遍
			  Multi_Udp_Buff[i].m_Socket = m_DataSocket;
			  //中心IP
			  sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d\0",LocalCfg.IP_Server[0],
						  LocalCfg.IP_Server[1], LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
			  // strcpy(Multi_Udp_Buff[i].RemoteHost, NSMULTIADDR);
			  //printf("Multi_Udp_Buff[i].RemoteHost is %s\n",Multi_Udp_Buff[i].RemoteHost);
			  //包头
			  memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			  //
			  Multi_Udp_Buff[i].buf[6] = SENDTALKINFO;//0x80
			  Multi_Udp_Buff[i].buf[7] = ASK;//请求

			  //主叫地址
			  memset(Multi_Udp_Buff[i].buf + 8, 0x30, 20); //被叫地址编码
			  memcpy(Multi_Udp_Buff[i].buf+8,LocalCfg.Addr,12);
		//	  memcpy(Remote.Addr[0], NullAddr, 20);
		//	  Remote.Addr[0][0]='Z';
			 // memcpy(Remote.Addr[0]+1,CallAddr+1,11);
			  Multi_Udp_Buff[i].buf[28] = 'Z';
			  memcpy(Multi_Udp_Buff[i].buf+29,NullAddr,20);
			  //呼叫状态量 0x01 0x02 0x03 0x04 0x05
		//	  memcpy(Multi_Udp_Buff[i].buf + 48 ,&SendTalkInfo,4);
			  memcpy(Multi_Udp_Buff[i].buf + 48, LocalCfg.Mac_Addr,6);//主叫网卡地址
			  //呼叫类型
			  Multi_Udp_Buff[i].buf[54] = SendTalkInfo.Status;//状态
			  //呼叫地址
			  memset(Multi_Udp_Buff[i].buf + 55, 0x30, 20); //被叫地址编码
			  memcpy(Multi_Udp_Buff[i].buf+55,SendTalkInfo.Addr,12);
			  //开始呼叫时间
			  memcpy(Multi_Udp_Buff[i].buf+75,SendTalkInfo.StartTime,15);
			  // //补‘\0’
			  // Multi_Udp_Buff[i].buf[69] = '\0';
			  //通话时长
			  sprintf(str,"%4d\0",SendTalkInfo.Duration);
			  memset(Multi_Udp_Buff[i].buf + 90, 0x30, 4); //被叫地址编码
			  memcpy(Multi_Udp_Buff[i].buf+90,str,4);

			  Multi_Udp_Buff[i].nlength = 94;
			  Multi_Udp_Buff[i].DelayTime = 100;
			  Multi_Udp_Buff[i].isValid = 1;
#ifdef _DEBUG
			  //				printf("脮媒脭脷陆芒脦枚碌脴脰路\n");
#endif
			  /*
				 memcpy(Label_CCenter.Text, Remote.Addr[0],12);
				 Label_CCenter.Text[12]='\0';
				 ShowLabel(&Label_CCenter, NOREFRESH, cBlack);
				 */
			  sem_post(&multi_send_sem);
			  printf("send the info in %d\n",i);
			  break;
		  }
		//麓貌驴陋禄楼鲁芒脣酶
		pthread_mutex_unlock (&Local.udp_lock);
	}

}
