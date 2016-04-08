#include     <stdio.h>      /*��׼�����������*/
#include   <time.h>
#include     <stdlib.h>     /*��׼�����ⶨ��*/
#include     <unistd.h>     /*Unix ��׼��������*/
#include     <sys/types.h>  
#include     <sys/stat.h>   
#include     <fcntl.h>      /*�ļ����ƶ���*/
#include     <termios.h>    /*PPSIX �ն˿��ƶ���*/
#include     <errno.h>      /*����Ŷ���*/

#define CommonH
#include "common.h"

#define FALSE  -1
#define TRUE   0
///////////////paul0304//////////////////
extern int audio_play_wav_flag;
/////////////////updatemcu
struct update_mcu McuUpdate;
extern char TmpFromIP[20];
extern char CommTmpBuff[20];
extern char villabuf[10][60];
int OpenDev(char *Dev);  //�򿪴���
int set_speed(int fd, int speed);  //���ò�����
int set_Parity(int fd,int databits,int stopbits,int parity);  //���ò���

int OpenComm(int CommPort,int BautSpeed,int databits,int stopbits,int parity);  //�򿪴���

short CommRecvFlag=1;
pthread_t comm2_rcvid;
pthread_t comm3_rcvid;
pthread_t comm4_rcvid;
void CreateComm2_RcvThread(int fd);
void CreateComm3_RcvThread(int fd);
void CreateComm4_RcvThread(int fd);
void Comm2_RcvThread(int fd);  //Comm2�����̺߳���
void Comm3_RcvThread(int fd);  //Comm3�����̺߳���
void Comm4_RcvThread(int fd);  //Comm4�����̺߳���
int CommSendBuff(int fd,unsigned char buf[1024],int nlength);
void CloseComm();
void UpdateMCU_Func(char valid);
//��������
void OpenLock_Func(void);
void M8Input_Func(unsigned char *inputbuff); //M8����
void SendAck(unsigned char *inputbuff, int nLength);//����Ӧ��
void SendQueryAck(unsigned char *inputbuff, int nLength); //���Ͳ�ѯӦ��
void SendHostOrder(unsigned char norder, char doorno, char *param); //������������
//�洢����
void Save_Setup(void);
void SendUpdateOrder(int commfd,int length, char order, char * param);
void SendDoorStatus(char *buf);
void SendOpenLock(char *buf);
//---------------------------------------------------------------------------
int OpenDev(char *Dev)
{
	int	fd = open( Dev, O_RDWR );         //| O_NOCTTY | O_NDELAY
	if (FALSE == fd)
	{
		perror("Can't Open Serial Port");
		return FALSE;
	}
	else
		return fd;
}
//---------------------------------------------------------------------------
/**
 *@brief  ���ô���ͨ������
 *@param  fd     ���� int  �򿪴��ڵ��ļ����
 *@param  speed  ���� int  �����ٶ�
 *@return  void
 */
int speed_arr[] = { B38400, B19200, B9600, B4800, B2400, B1200, B300,
	B38400, B19200, B9600, B4800, B2400, B1200, B300, };
int name_arr[] = {38400,  19200,  9600,  4800,  2400,  1200,  300, 38400,
	19200,  9600, 4800, 2400, 1200,  300, };
int set_speed(int fd, int speed)
{
	int   i;
	int   status;
	struct termios   Opt;
	tcgetattr(fd, &Opt);
	for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++)
	{
		if  (speed == name_arr[i])
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr[i]);
			cfsetospeed(&Opt, speed_arr[i]);
			status = tcsetattr(fd, TCSANOW, &Opt);
			if  (status != 0)
			{
				perror("tcsetattr fd1");
				return FALSE;
			}
			tcflush(fd,TCIOFLUSH);
		}
	}
	return TRUE;
}
//---------------------------------------------------------------------------
/**
 *@brief   ���ô�������λ��ֹͣλ��Ч��λ
 *@param  fd     ����  int  �򿪵Ĵ����ļ����
 *@param  databits ����  int ����λ   ȡֵ Ϊ 7 ����8
 *@param  stopbits ����  int ֹͣλ   ȡֵΪ 1 ����2
 *@param  parity  ����  int  Ч������ ȡֵΪN,E,O,,S
 */
int set_Parity(int fd,int databits,int stopbits,int parity)
{
	struct termios options;
	if  ( tcgetattr( fd,&options)  !=  0)
	{
		perror("SetupSerial 1");
		return(FALSE);
	}
	options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
	//  options.c_oflag  &= ~OPOST;   /*Output*/

	options.c_cflag &= ~CSIZE;
	switch (databits) /*��������λ��*/
	{
		case 7:
			options.c_cflag |= CS7;
			break;
		case 8:
			options.c_cflag |= CS8;
			break;
		default:
			fprintf(stderr,"Unsupported data size\n"); return (FALSE);
	}
	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB;   /* Clear parity enable */
			options.c_iflag &= ~INPCK;     /* Enable parity checking */ 
			break;  
		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB); /* ����Ϊ��Ч��*/  
			options.c_iflag |= INPCK;             /* Disnable parity checking */ 
			break;  
		case 'e':
		case 'E':
			options.c_cflag |= PARENB;     /* Enable parity */    
			options.c_cflag &= ~PARODD;   /* ת��ΪżЧ��*/     
			options.c_iflag |= INPCK;       /* Disnable parity checking */
			break;
		case 'S':
		case 's':  /*as no parity*/
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;  
		default:
			fprintf(stderr,"Unsupported parity\n");    
			return (FALSE);  
	}  
	/* ����ֹͣλ*/
	switch (stopbits)
	{
		case 1:    
			options.c_cflag &= ~CSTOPB;  
			break;  
		case 2:    
			options.c_cflag |= CSTOPB;  
			break;
		default:    
			fprintf(stderr,"Unsupported stop bits\n");  
			return (FALSE); 
	}
	/* Set input parity option */ 
	if (parity != 'n')
		options.c_iflag |= INPCK;

	//һЩ��������  
	options.c_cflag |= CLOCAL | CREAD;
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);


	tcflush(fd,TCIFLUSH);
	options.c_cc[VTIME] = 10; /* ���ó�ʱ10 seconds*/
	options.c_cc[VMIN] = 1;//0; /* Update the options and do it NOW */
	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		perror("SetupSerial 3");
		return (FALSE);
	}
	return (TRUE);
}
//---------------------------------------------------------------------------
int OpenComm(int CommPort,int BautSpeed,int databits,int stopbits,int parity)
{
	char dev[20]  = "/dev/ttyS"; //���ڶ�
	char   tmp[3];
	int Commfd;
	sprintf(tmp,   "%ld", CommPort-1 );
	strcat(dev,tmp);
	Commfd = OpenDev(dev);
	printf("Comm is %s(%d)\n",dev,Commfd);
	if(Commfd == FALSE)
		return FALSE;
	if(set_speed(Commfd,BautSpeed) == FALSE)
	{
		printf("Set Baut Error\n");
		return FALSE;
	}
	if (set_Parity(Commfd,databits,stopbits,parity) == FALSE)
	{
		printf("Set Parity Error\n");
		return FALSE;
	}
	switch(CommPort)
	{
		case 2:
			CreateComm2_RcvThread(Commfd);
			//CreateComm3_RcvThread(Commfd);      
			break;
		case 3:
			CreateComm3_RcvThread(Commfd);
			break;
		case 4:
			CreateComm4_RcvThread(Commfd);
			break;
	}
	return Commfd;
}
//---------------------------------------------------------------------------
void CreateComm2_RcvThread(int fd)
{
	int i,ret;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	ret=pthread_create(&comm2_rcvid, &attr, (void *)Comm2_RcvThread, (int*)fd);
	pthread_attr_destroy(&attr);
	printf ("Create comm2 pthread!\n");
	if(ret!=0){
		printf ("Create comm2 pthread error!\n");
		exit (1);
	}
}
//---------------------------------------------------------------------------
void CreateComm3_RcvThread(int fd)
{
	int ret;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret=pthread_create(&comm3_rcvid, &attr, (void *)Comm3_RcvThread, (int*)fd);
	pthread_attr_destroy(&attr);
	printf ("Create comm3 pthread!\n");
	if(ret!=0){
		printf ("Create comm3 pthread error!\n");
		exit (1);
	}
}
//---------------------------------------------------------------------------
void CreateComm4_RcvThread(int fd)
{
	int ret;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret=pthread_create(&comm4_rcvid, &attr, (void *)Comm4_RcvThread, (int*)fd);
	pthread_attr_destroy(&attr);
	printf ("Create comm4 pthread!\n");
	if(ret!=0){
		printf ("Create comm4 pthread error!\n");
		exit (1);
	}
}
//---------------------------------------------------------------------------
void Comm2_RcvThread(int fd)  //Comm2�����̺߳���
{
	struct timeval tv;
	uint32_t prev_comm_time;
	uint32_t nowtime;
	int len,validlen;
	int i;
	char buff[128];
	unsigned char validbuff[512];
	unsigned char m_crc;
	char FromIP[20];
	struct commbuf1 commbuf;
	printf("This is comm2 pthread.\n");
	//��һ�δ������ݽ���ʱ��
	gettimeofday(&tv, NULL);
	prev_comm_time = tv.tv_sec *1000 + tv.tv_usec/1000; 
	while (CommRecvFlag == 1)     //ѭ����ȡ����
	{
		//ϵͳ��ʼ����־,��û�г�ʼ�������ȴ�
		while(InitSuccFlag == 0)
			usleep(10000);
		while((len = read(fd, buff, 512))>0)
		{
			//if(len > 8)
			gettimeofday(&tv, NULL);
			nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;
			//����һ�ν��ճ���50ms,���ж�Ϊ��ʱ
			//printf("The Comm2 time is %d /n",nowtime);
			if((nowtime - prev_comm_time) >= 50)
			{
				commbuf.iget = 0;
				commbuf.iput = 0;
				commbuf.n = 0;
			}
			prev_comm_time = nowtime;

			printf("Len %d Comm2\n",len);
			buff[len] = '\0';
			memcpy(commbuf.buffer + commbuf.iput, buff, len);
			commbuf.iput += len;
			if(commbuf.iput >= COMMMAX)
				commbuf.iput = 0;
			commbuf.n += len;
			while(commbuf.n >= 6)
			{
				if (commbuf.buffer[commbuf.iget] ==0xBD)  
				{
					validlen=commbuf.buffer[commbuf.iget +1]+3;
					if (commbuf.n >=validlen)
					{
						memcpy(validbuff, commbuf.buffer + commbuf.iget, validlen);
						commbuf.iget += validlen;
						if (commbuf.iget >= COMMMAX)
							commbuf.iget = 0;
						commbuf.n -= validlen;
						if ((validbuff[0] == 0xBD))
						{
							m_crc = 0;
							for (i=0; i< validlen-1; i++)
								m_crc += validbuff[i]; 
							printf("comm2:m_crc = 0x%X, validbuff[16] = 0x%X\n", m_crc, validbuff[16]);
							if (m_crc == validbuff[validlen-1])
							{
								if((validbuff[2]==0x12))//||(commbuf.buffer[commbuf.iget + 7]==0x00))
									UpdateMCU_Func(validbuff[7]);
								else
								{
									//////////////////////////////////////////////////////////////////////////////////////////
									/*		printf("\nCom2 recieve the data is ");
											for(i=0;i<validlen;i++)
											printf("%X ",validbuff[i]);
											*/
									switch(validbuff[2])
									{
										//printf("\nthe data is 11111111111111");
										case 0x20:
											if((validbuff[7]==0x00)&&(validbuff[8]==0x00)&&(validbuff[9]==0x00)&&(validbuff[10]==0x00))
											{

												printf("\nthe Comm2 data is go to Udp");
												sprintf(FromIP,"%d.%d.%d.%d",LocalCfg.IP_Server[0],LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);	
												validbuff[validbuff[1]+2]=validbuff[validbuff[1]+2]-validbuff[3]-validbuff[4];

												validbuff[3]=((LocalCfg.Addr[3]-'0')<<4)|((LocalCfg.Addr[4]-'0')&0x0F);
												validbuff[4]=((LocalCfg.Addr[5]-'0')<<4)|((LocalCfg.Addr[6]-'0')&0x0F);
												validbuff[validbuff[1]+2]=validbuff[validbuff[1]+2]+validbuff[3]+validbuff[4];
												UdpSendBuff(m_ForbidSocket,FromIP,validbuff,validbuff[1]+3);
											}
											else if((validbuff[7]==0x0A)&&(validbuff[8]==0x0A)&&(validbuff[9]==0x00)&&(validbuff[10]==0x50))
											{
												printf("\nsendopenlock\n");
												SendOpenLock(validbuff);
											}

											else 
											{
												printf("\nthe Comm2 data is go to Comm3");
												CommSendBuff(Comm3fd,validbuff,validbuff[1]+3);
											}
											break;
										case 0x00:
											if((validbuff[3]==0x00)&&(validbuff[4]==0x00)&&(validbuff[5]==0x00)&&(validbuff[6]==0x00))
											{

												printf("\nthe Comm2 data is go to Udp");
												/////////0812
												if((validbuff[9]!=0x0A)&&(validbuff[10]!=0x0A))
												{
													validbuff[validbuff[1]+2]=validbuff[validbuff[1]+2]-validbuff[7]-validbuff[8];

													validbuff[7]=((LocalCfg.Addr[3]-'0')<<4)|((LocalCfg.Addr[4]-'0')&0x0F);
													validbuff[8]=((LocalCfg.Addr[5]-'0')<<4)|((LocalCfg.Addr[6]-'0')&0x0F);
													validbuff[validbuff[1]+2]=validbuff[validbuff[1]+2]+validbuff[7]+validbuff[8];
												}
													sprintf(FromIP,"%d.%d.%d.%d",LocalCfg.IP_Server[0],LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
												UdpSendBuff(m_ForbidSocket,FromIP,validbuff,validbuff[1]+3);
											}
											else
											{
												printf("\nthe Comm2 data is go to Comm3");
												CommSendBuff(Comm3fd,validbuff,validbuff[1]+3);
											}
											break;
									}//switch
								}//else


								//
								//////////////////////////////////////////////////////////////////////////////////////////
								//CommMJ_Func(validbuff); //M8����
							}
							else
								printf("crc error\n");
						}
					}
					else
						break;
				}
				else
				{
					commbuf.iget ++;
					if(commbuf.iget >= COMMMAX)
						commbuf.iget = 0;
					commbuf.n --;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void SendAck(unsigned char *inputbuff, int nLength) //����Ӧ��
{
	unsigned char send_b[30];
	int i;
	memcpy(send_b, inputbuff, nLength);
	send_b[2] = 0x32; //Ӧ��
	send_b[16] = 0;
	printf("%02X,",send_b[0]);
	for(i=1; i<=15; i++)    //crc
	{
		printf("%02X,",send_b[i]);
		send_b[16] += send_b[i];
	}
	if((send_b[16] == 0x7E)||(send_b[16] == 0x0D))
		send_b[16] --;
	send_b[17] = 0x0D;
	printf("%02X,%2X\n\n",send_b[16],send_b[17]);
	///////paul0328/////////////////////////
	CommSendBuff(Comm3fd, send_b, nLength);
	//// CommSendBuff(Comm2fd, send_b, nLength);
}
//---------------------------------------------------------------------------
void SendQueryAck(unsigned char *inputbuff, int nLength) //���Ͳ�ѯӦ��
{
	unsigned char send_b[30];
	char tmp_char[20];
	int i;
	memcpy(send_b, inputbuff, nLength);
	send_b[2] = 0x32; //Ӧ��
	switch(inputbuff[1])
	{
		case 0x40:  //��ѯ����������ַ
			sprintf(tmp_char, "%02X%02X%02X\0", LocalCfg.Mac_Addr[3], LocalCfg.Mac_Addr[4], LocalCfg.Mac_Addr[5]);
			memcpy(send_b + 4, tmp_char, 6);
			break;
		case 0x41:  //��ѯ����IP��ַ
			printf("IP��ַ��%03d%03d%03d%03d\0", LocalCfg.IP[0], LocalCfg.IP[1],
					LocalCfg.IP[2], LocalCfg.IP[3]);
			sprintf(tmp_char, "%03d%03d%03d%03d\0", LocalCfg.IP[0], LocalCfg.IP[1],
					LocalCfg.IP[2], LocalCfg.IP[3]);
			memcpy(send_b + 4, tmp_char, 12);
			break;
		case 0x42:  //��ѯ��������
			sprintf(tmp_char, "%03d%03d%03d%03d\0", LocalCfg.IP_Mask[0], LocalCfg.IP_Mask[1],
					LocalCfg.IP_Mask[2], LocalCfg.IP_Mask[3]);
			memcpy(send_b + 4, tmp_char, 12);
			break;
		case 0x43:  //��ѯ���ص�ַ
			sprintf(tmp_char, "%03d%03d%03d%03d\0", LocalCfg.IP_Gate[0], LocalCfg.IP_Gate[1],
					LocalCfg.IP_Gate[2], LocalCfg.IP_Gate[3]);
			memcpy(send_b + 4, tmp_char, 12);
			break;
		case 0x44:  //��ѯ��������ַ
			sprintf(tmp_char, "%03d%03d%03d%03d\0", LocalCfg.IP_Server[0], LocalCfg.IP_Server[1],
					LocalCfg.IP_Server[2], LocalCfg.IP_Server[3]);
			memcpy(send_b + 4, tmp_char, 12);
			break;    
		case 0x45:  //��ѯ�ſڻ���ַ
			memcpy(send_b + 4, LocalCfg.Addr, 12);
			break;
		case 0x46:  //��ѯ��������
			//////////////////paul
			memcpy(send_b + 4, LocalCfg.EngineerPass, 6);
			break;    
		case 0x47:  //��ѯ��������
			//////////////////paul
			memcpy(send_b + 4, LocalCfg.OpenLockPass, 6);
			break;
		case 0x48:  //��ѯ���뿪�����ܴ򿪻�ر�
			send_b[4] = LocalCfg.PassOpenLock + 0x30;
			break;
		case 0x49:  //��ѯ��ƵCIFʱ����
			send_b[4] = LocalCfg.bit_rate + 0x30;
			break;
		case 0x57:  //�汾��ѯ
			memcpy(send_b + 4, HARDWAREVER, 4);
			memcpy(send_b + 8, SOFTWAREVER, 4);
			break;
	}
	send_b[16] = 0;
//	printf("SendQueryAck:%02x,",send_b[0]);
	for(i=1; i<=15; i++)    //crc
	{
//		printf("%02x,",send_b[i]);
		send_b[16] += send_b[i];
	}
	if((send_b[16] == 0x7E)||(send_b[16] == 0x0D))
		send_b[16] --;
	send_b[17] = 0x0D;

//	printf("%02x,%02x\n",send_b[16],send_b[17]);
	///////////paul0328////////////////
	CommSendBuff(Comm3fd, send_b, nLength);

	//// CommSendBuff(Comm2fd, send_b, nLength);
}
//---------------------------------------------------------------------------
void SendHostOrder(unsigned char norder, char doorno, char *param) //������������
{
	int i,j;
	//����������
	pthread_mutex_lock (&Local.comm_lock);
	//���ҿ��÷��ͻ��岢���
	for(i=0; i<COMMSENDMAX; i++)
	{
		if(Multi_Comm_Buff[i].isValid == 0)
		{
			if(norder == 0x56)    //�����豸
			  Multi_Comm_Buff[i].SendNum = 2;
			else
			  Multi_Comm_Buff[i].SendNum = 0;
			////////////////////paul0328Comm3fd
			Multi_Comm_Buff[i].m_Comm = Comm3fd;

			Multi_Comm_Buff[i].buf[0] = 0x7E;   //��ʼ�ַ�
			Multi_Comm_Buff[i].buf[1] = norder;   //����
			Multi_Comm_Buff[i].buf[2] = 0x31;   // ��������
			Multi_Comm_Buff[i].buf[3] = doorno;   //�ſڻ�����

			if(param == NULL)
			{
				for(j=4; j<=15; j++)
				  Multi_Comm_Buff[i].buf[j] = 0;
			}
			else
			  memcpy(Multi_Comm_Buff[i].buf + 4, param, 12);
			Multi_Comm_Buff[i].buf[16] = 0;
			for(j=1; j<=15; j++)    //crc
			  Multi_Comm_Buff[i].buf[16] += Multi_Comm_Buff[i].buf[j];
			if((Multi_Comm_Buff[i].buf[16] == 0x7E)||(Multi_Comm_Buff[i].buf[16] == 0x0D))
			  Multi_Comm_Buff[i].buf[16] --;
			Multi_Comm_Buff[i].buf[17] = 0x0D;

			Multi_Comm_Buff[i].nlength = 18;
			Multi_Comm_Buff[i].isValid = 1;
			sem_post(&multi_comm_send_sem);
			break;
		}
	}
	//�򿪻�����
	pthread_mutex_unlock (&Local.comm_lock);
}

void M8Input_Func(unsigned char *inputbuff) //M8����
{
	char call_buff[20];
	char mac_text[5];
	int hex_data[6];
	char cFromIP[20];
	int tmprow;
	int i,j;
	char wavFile[100];
	unsigned char send_b[130];
	int PassOk = 1;
	//////////////paul0827
	time_t t;
	struct tm *tm_t;
	////////////////////////
	//10S�޲���, ���ص���������
	PicStatBuf.KeyPressTime = 0;

	switch(inputbuff[1])
	{	///////////lock
		case 0x73:
		case 0x74:///rob
			pthread_mutex_lock (&Local.comm_lock);
			//���ҿ��÷��ͻ��岢���
			for(i=0; i<COMMSENDMAX; i++)
				if(Multi_Comm_Buff[i].isValid == 1)
					if(Multi_Comm_Buff[i].buf[1] == inputbuff[1])
					//if(Multi_Comm_Buff[i].buf[1] == 0x73)
						if((Multi_Comm_Buff[i].buf[2] == 0x31)&&(inputbuff[2]==0x32))
							if(memcmp(Multi_Comm_Buff[i].buf+ 4, inputbuff + 4, 4) == 0)
							{
								Multi_Comm_Buff[i].isValid = 0;
								strcpy(cFromIP,Multi_Comm_Buff[i].buf +20);

								memcpy(send_b,UdpPackageHead,6);
								send_b[6] = 0x54;
								send_b[7] = REPLY;
								send_b[8] = 'S';
								memcpy(send_b + 9, LocalCfg.Addr + 1,10);
								memcpy(send_b + 15, inputbuff + 4, 4);
								memcpy(send_b + 28,LocalCfg.Addr,12);
								send_b[54] = inputbuff[1]%2;
								send_b[55] = inputbuff[8];
								UdpSendBuff(m_DataSocket,cFromIP,send_b,56);
								printf("FromIP is %s,%c\n",cFromIP,send_b[6]);
								break;
							}
			//�򿪻�����
			pthread_mutex_unlock (&Local.comm_lock);
			break;
		case 0x30: //���ñ���������ַ
			//printf("inputbuff[7] = %X, inputbuff[8] = %X, inputbuff[9] = %X\n",inputbuff[7],inputbuff[8],inputbuff[9]);

			SendAck(inputbuff, 18);

			LocalCfg.Mac_Addr[0] = 0x00;
			LocalCfg.Mac_Addr[1] = 0x00;
			LocalCfg.Mac_Addr[2] = 0x00;
			for(i=0; i<6; i+=2)
			{
				mac_text[0] = inputbuff[i+4];
				mac_text[1] = inputbuff[i+4+1];
				mac_text[2] = '\0';
				sscanf(mac_text, "%x", &hex_data[i/2]);
			}
			if((LocalCfg.Mac_Addr[3] != hex_data[0])
					||(LocalCfg.Mac_Addr[4] != hex_data[1])
					||(LocalCfg.Mac_Addr[5] != hex_data[2]))
			{
				LocalCfg.Mac_Addr[3] = hex_data[0];
				LocalCfg.Mac_Addr[4] = hex_data[1];
				LocalCfg.Mac_Addr[5] = hex_data[2];
				Save_Setup();       //�洢����
				RefreshNetSetup(1); //ˢ����������
			}
			break;
		case 0x31: //���ñ���IP��ַ
		case 0x32: //������������
		case 0x33: //�������ص�ַ
		case 0x34: //���÷�������ַ
			SendAck(inputbuff, 18);

			for(i=0; i<12; i+=3)
			{
				mac_text[0] = inputbuff[i+4];
				mac_text[1] = inputbuff[i+5];
				mac_text[2] = inputbuff[i+6];
				mac_text[3] = '\0';
				sscanf(mac_text, "%d", &hex_data[i/3]);
			}
			switch(inputbuff[1])
			{
				case 0x31:
					if((LocalCfg.IP[0] != hex_data[0])
							||(LocalCfg.IP[1] != hex_data[1])
							||(LocalCfg.IP[2] != hex_data[2])
							||(LocalCfg.IP[3] != hex_data[3]))
					{
						LocalCfg.IP[0] = hex_data[0];
						LocalCfg.IP[1] = hex_data[1];
						LocalCfg.IP[2] = hex_data[2];
						LocalCfg.IP[3] = hex_data[3];
						//�㲥��ַ
						for(i=0; i<4; i++)
						{
							if(LocalCfg.IP_Mask[i] != 0)
								LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i] & LocalCfg.IP[i];
							else
								LocalCfg.IP_Broadcast[i] = 0xFF;
						}
						Save_Setup();       //�洢����
						RefreshNetSetup(1); //ˢ����������
					}
					break;
				case 0x32:
					if((LocalCfg.IP_Mask[0] != hex_data[0])
							||(LocalCfg.IP_Mask[1] != hex_data[1])
							||(LocalCfg.IP_Mask[2] != hex_data[2])
							||(LocalCfg.IP_Mask[3] != hex_data[3]))
					{
						LocalCfg.IP_Mask[0] = hex_data[0];
						LocalCfg.IP_Mask[1] = hex_data[1];
						LocalCfg.IP_Mask[2] = hex_data[2];
						LocalCfg.IP_Mask[3] = hex_data[3];
						//�㲥��ַ
						for(i=0; i<4; i++)
						{
							if(LocalCfg.IP_Mask[i] != 0)
								LocalCfg.IP_Broadcast[i] = LocalCfg.IP_Mask[i] & LocalCfg.IP[i];
							else
								LocalCfg.IP_Broadcast[i] = 0xFF;
						}                             
						Save_Setup();       //�洢����
						RefreshNetSetup(1); //ˢ����������
					}
					break;
				case 0x33:
					if((LocalCfg.IP_Gate[0] != hex_data[0])
							||(LocalCfg.IP_Gate[1] != hex_data[1])
							||(LocalCfg.IP_Gate[2] != hex_data[2])
							||(LocalCfg.IP_Gate[3] != hex_data[3]))
					{
						LocalCfg.IP_Gate[0] = hex_data[0];
						LocalCfg.IP_Gate[1] = hex_data[1];
						LocalCfg.IP_Gate[2] = hex_data[2];
						LocalCfg.IP_Gate[3] = hex_data[3];
						Save_Setup();       //�洢����
						RefreshNetSetup(1); //ˢ����������
					}
					break;
				case 0x34:
					if((LocalCfg.IP_Server[0] != hex_data[0])
							||(LocalCfg.IP_Server[1] != hex_data[1])
							||(LocalCfg.IP_Server[2] != hex_data[2])
							||(LocalCfg.IP_Server[3] != hex_data[3]))
					{
						LocalCfg.IP_Server[0] = hex_data[0];
						LocalCfg.IP_Server[1] = hex_data[1];
						LocalCfg.IP_Server[2] = hex_data[2];
						LocalCfg.IP_Server[3] = hex_data[3];
						Save_Setup();       //�洢����
					}
					break;
			}
			break;
		case 0x35: //�����ſڻ���ַ
			SendAck(inputbuff, 18);

			switch(inputbuff[4])
			{
				case 'W': //Χǽ��
					if((LocalCfg.Addr[0] != inputbuff[4])
							||(LocalCfg.Addr[3] != inputbuff[7])
							||(LocalCfg.Addr[4] != inputbuff[8]))
					{
						memcpy(LocalCfg.Addr, NullAddr, 12);
						memcpy(LocalCfg.Addr, inputbuff + 4, 5);
						Save_Setup();       //�洢����
					}
					break;
				case 'M': //��Ԫ�ſڻ�
					if((LocalCfg.Addr[0] != inputbuff[4])
							||(LocalCfg.Addr[1] != inputbuff[5])
							||(LocalCfg.Addr[2] != inputbuff[6])
							||(LocalCfg.Addr[3] != inputbuff[7])
							||(LocalCfg.Addr[4] != inputbuff[8])
							||(LocalCfg.Addr[5] != inputbuff[9])
							||(LocalCfg.Addr[6] != inputbuff[10]))
					{
						memcpy(LocalCfg.Addr, NullAddr, 12);
						memcpy(LocalCfg.Addr, inputbuff + 4, 7);
						Save_Setup();       //�洢����
					}
					break;
			}
			break;
		case 0x36: //���ù�������
			SendAck(inputbuff, 18);

			/////////////////paul
			memcpy(LocalCfg.EngineerPass, inputbuff + 4, 6);
			LocalCfg.EngineerPass[6] = '\0';
			Save_Setup();       //�洢����
			break;
		case 0x37: //���ÿ�������
			SendAck(inputbuff, 18);

			////////////////////////////paul
			memcpy(LocalCfg.OpenLockPass, inputbuff + 4, 6);
			LocalCfg.OpenLockPass[6] = '\0';
			Save_Setup();       //�洢����
			break;
		case 0x38: //�������뿪�����ܴ򿪻�ر�
			SendAck(inputbuff, 18);

			/////////////////////paul
			if((inputbuff[4] == 0x30)||(inputbuff[4] == 0x31))
			{
				LocalCfg.PassOpenLock = inputbuff[4] - 0x30;
				Save_Setup();       //�洢����
			}
			break;
		case 0x39: //������ƵCIFʱ����
			SendAck(inputbuff, 18);
			printf("LocalCfg.bit_rate = inputbuff[4] = %d\n",  inputbuff[4]);

			LocalCfg.bit_rate = inputbuff[4] - 0x30;
			Save_Setup();       //�洢����
			break;
			//////////////////paul0728///////////////////
		case 0x3A:
			SendAck(inputbuff,18);
			printf("ALARM: %02x\n",inputbuff[4]);
			for(i=0;i<18;i++)
			  printf("%02x,",inputbuff[i]);
			printf("\n");
			if(inputbuff[4]==0x31)///BRAKE
			{
				//SendAlarmFunc(1<<(Local.DoorNo-0x31),0x01);
				SendAlarmFunc(Local.DoorNo,0x01);
			}
			else if(inputbuff[4]==0x30)//ROB
			{
				memset(send_b,0x30,20);
				memcpy(send_b,LocalCfg.Addr,11);
				//memcpy(send_b,LocalCfg.Addr,12);
				send_b[0]='S';
				send_b[7]=inputbuff[5];
				send_b[8]=inputbuff[6];
				send_b[9]=inputbuff[7];
				send_b[10]=inputbuff[8];
				send_b[21]='\0';
				//	SendRobAlarm(0x31,send_b);
				SendRobAlarm((Local.DoorNo),send_b);
				//SendRobAlarm(1<<(Local.DoorNo-0x31),send_b);

			}
////////////////////////////paul0827
			else if(inputbuff[4]==0x32)//�Ŵż��
			{
			//	time(&t);
      		//	tm_t=localtime(&t);
				//printf("about door:\n");	
				send_b[0]=0xBD;
				send_b[1]=0x13;
				send_b[2]=0x20;

				send_b[3]=((LocalCfg.Addr[3]-'0')<<4)|((LocalCfg.Addr[4]-'0')&0x0F);
				send_b[4]=((LocalCfg.Addr[5]-'0')<<4)|((LocalCfg.Addr[6]-'0')&0x0F);
				send_b[5]=0x00;
				send_b[6]=((Local.DoorNo-0x31)&0x0F)|(0x60);

			
				send_b[7]=0x00;
				send_b[8]=0x00;
				send_b[9]=0x00;
				send_b[10]=0x00;

				send_b[11]=0x20;
				send_b[12]=0x00;
				if(inputbuff[5]==0x30)//open
				{
					memset(send_b+13,0x0C,4);
				}
				else if(inputbuff[5]==0x31)//close
				{
					memset(send_b+13,0x0D,4);
				}
				else
				{
					return;
				}
				memset(send_b+17,0x00,4);

				send_b[21]=0x00;
				for(i=0;i<0x15;i++)
				{
					send_b[21] += send_b[i];
					//printf("%02x ",send_b[i]);
				}
				//printf("\n");	
				SendDoorStatus(send_b);

				
			}
			else if(inputbuff[4]==0x33)//�Ŵ�alarm
			{
					memset(send_b,0x30,20);
				memcpy(send_b,LocalCfg.Addr,11);
				SendDoorAlarm(Local.DoorNo,NULL);	
			}
			else if(inputbuff[4]==0x34)//POWER ALARM
			{
					memset(send_b,0x30,20);
				memcpy(send_b,LocalCfg.Addr,11);

				SendPowerAlarm(0,NULL);
			}
			break;
		case 0x3B:
			SendAck(inputbuff, 18);
			printf("OPENLOCKINFO\n");
			time(&t);
			Local.call_tm_t = localtime(&t);//��ǰʱ��
			//if(inputbuff[4] == 0x31)
			SendTalkInfo.Status = 0x07;//0x06 call 0x07 pwd
			//else
		//		SendTalkInfo.Status = 0x06;//0x06 call 0x07 pwd
			SendTalkInfo.Duration =0x00;
			SendTalkInfo.Addr[0] = 'S'; 
			memcpy(SendTalkInfo.Addr + 1, LocalCfg.Addr + 1, 6);
			memcpy(SendTalkInfo.Addr + 7, inputbuff + 5, 4);
			sprintf(SendTalkInfo.StartTime,"%04d%02d%02d%02d%02d%02d",
						Local.call_tm_t->tm_year+1900,
				Local.call_tm_t->tm_mon + 1,
				Local.call_tm_t->tm_mday,
				Local.call_tm_t->tm_hour,
				Local.call_tm_t->tm_min,
				Local.call_tm_t->tm_sec);
			SendTalkInfo.StartTime[14] = 0x30;
			SendTalkInfo.StartTime[15] = '\0';

			NeedOpenLock = 1;
			break;
		case 0x40:  //��ѯ����������ַ
		case 0x41:  //��ѯ����IP��ַ
		case 0x42:  //��ѯ��������
		case 0x43:  //��ѯ���ص�ַ
		case 0x44:  //��ѯ��������ַ
		case 0x45:  //��ѯ�ſڻ���ַ
			printf("��ѯ�ſڻ���ַ\n");
		case 0x46:  //��ѯ��������
		case 0x47:  //��ѯ��������
		case 0x48:  //��ѯ���뿪�����ܴ򿪻�ر�
		case 0x49:  //��ѯ��ƵCIFʱ����
			SendQueryAck(inputbuff, 18); //���Ͳ�ѯӦ��
			break;
		case 0x57:  //�汾��ѯ
			SendQueryAck(inputbuff, 18); //���Ͳ�ѯӦ��
			Local.DoorNo = inputbuff[3];
#ifdef _DEBUG
			printf("(doorno:0x%02x)\n", inputbuff[3]);
			printf("M8(doorno:%d)\n", inputbuff[3] - 0x30);
#endif
			LocalCfg.Addr[11]=inputbuff[3]-0x01;

			break;
		case 0x50:  //���Ű�ť����
			SendAck(inputbuff, 18);

			printf("open door\n");
			Local.OpenDoorFlag = 2;   //������־ 0 δ����  1 ������ʱ��  2 ������
			Local.OpenDoorTime = 0;   //ʱ��
			//������
			ioctl(gpio_fd, IO_PUT, 0);
			time(&t);
			tm_t = localtime(&t);
			//zhou101102//����ͨ��ʱ��
			sprintf(wavFile,"%04d%02d%02d%02d%02d%02d",
						tm_t->tm_year+1900,
						tm_t->tm_mon + 1,
						tm_t->tm_mday,
						tm_t->tm_hour,
						tm_t->tm_min,
						tm_t->tm_sec);
			wavFile[14] = 0x30;
			wavFile[15] = '\0';

			strcpy(SendTalkInfo.StartTime,wavFile);
			printf("SendTalkInfo.StartTime is %s\n",SendTalkInfo.StartTime);
			NeedOpenLock = 1;//zhou101102 //�����򿪺󣬷�����Ϣ��UDP��Ϣ
			break;
		case 0x51:  //���뿪��
			SendAck(inputbuff, 18);
			///////////////////paul            
			if((inputbuff[4] == LocalCfg.OpenLockPass[0])&&(inputbuff[5] == LocalCfg.OpenLockPass[1])
					&&(inputbuff[6] == LocalCfg.OpenLockPass[2])&&(inputbuff[7] == LocalCfg.OpenLockPass[3])
					&&(inputbuff[8] == LocalCfg.OpenLockPass[4])&&(inputbuff[9] == LocalCfg.OpenLockPass[5]))
			{          //������ȷ�������Ѵ�
				OpenLock_Func();  //��������
				time(&t);
				tm_t = localtime(&t);
				//zhou101102//����ͨ��ʱ��
				sprintf(wavFile,"%04d%02d%02d%02d%02d%02d",
						tm_t->tm_year+1900,
						tm_t->tm_mon + 1,
						tm_t->tm_mday,
						tm_t->tm_hour,
						tm_t->tm_min,
						tm_t->tm_sec);
				wavFile[14] = 0x30;
				wavFile[15] = '\0';

				strcpy(SendTalkInfo.StartTime,wavFile);
				printf("SendTalkInfo.StartTime is %s\n",SendTalkInfo.StartTime);
				NeedOpenLock = 1;//zhou101102 //�����򿪺󣬷�����Ϣ��UDP��Ϣ

			}
			break;
		case 0x52:  //ID����
			SendAck(inputbuff, 18);
			break;
		case 0x53:  //��״̬����
			SendAck(inputbuff, 18);
			break;
		case 0x54:  //����
			SendAck(inputbuff, 18);
			/////////////////paul0302///////////////////
			//sprintf(wavFile,"%sasong.wav\0","/mnt/mtd/");
			//  printf("A song~!");
			// StartPlayWav(wavFile,1);
			///////////////////////////////////////////////////////////
			//	Local.DoorNo = inputbuff[3];  //�ſڻ�����
			/*for(i=0;i<18;i++)
			  printf("0x%02X ", inputbuff[i]);
			  printf("\n");
			  */ 
#ifdef _LXC_END_WAIT
			if(Local.WaitforEnd == 1)
			{
				SendHostOrder(0x64, Local.DoorNo, NULL); //������������  �Է���æ
				break;
			}
#endif
			switch(LocalCfg.Addr[0])
			{
				case 'W':
					if((inputbuff[4] == 0x30)&&((inputbuff[5] == 0x30)||(inputbuff[5]==0x31))&&(inputbuff[6] == 0x30)
								&&(inputbuff[7] == 0x30)&&(inputbuff[8] == 0x30)&&(inputbuff[9] == 0x30)
								&&(inputbuff[10] == 0x30)&&(inputbuff[11] == 0x30)&&(inputbuff[12] == 0x30)
								&&(inputbuff[13] == 0x30))
					{
						Call_Func(1, call_buff);    //��������
					}
					else
					{
						if((inputbuff[8] == 0x30)&&(inputbuff[9] == 0x30)&&(inputbuff[10] == 0x30)
									&&(inputbuff[11] == 0x30)&&(inputbuff[12] == 0x30)&&(inputbuff[13] == 0x30))
						{            //����
							memcpy(call_buff, inputbuff + 4, 4);
							call_buff[4] = '\0';
							Call_Func(2, call_buff);     //����ס��
						}
						else
						{           //��ͨ
							memcpy(call_buff, inputbuff + 4, 10);
							call_buff[10] = '\0';
							Call_Func(2, call_buff);     //����ס��
							printf("call_buff = %s\n", call_buff);
						}
					}
					break;
				case 'M':
					if((inputbuff[4] == 0x3A))
					{
						memcpy(call_buff, inputbuff + 4, 4);
						Call_Func(1, call_buff);    //��������
					}
					else
					{
						memcpy(call_buff, inputbuff + 4, 4);
						call_buff[4] = '\0';
						Call_Func(2, call_buff);     //����ס��
					}
					break;
				default:
					break;
			}
			break; 
		case 0x55:  //�һ�
			printf("M8 HANGUP %c\n",Remote.Addr[0][0]);
			SendAck(inputbuff, 18);
			TalkEnd_Func();
			break;
		case 0x70:  //ֹͣ����
			printf("M8 ֹͣ��������\n");
			SendAck(inputbuff, 18);
			WatchEnd_Func();
			break;
		case 0x56:  //�����豸
			memcpy(call_buff, inputbuff + 4, 4);
			call_buff[4] = '\0';
			printf("call_buff = %s\n", call_buff);

			SendAck(inputbuff, 18);

			memcpy(Local.FindEquip, inputbuff + 4, 12);
			Local.FindEquip[12] = '\0';
			//Local.DoorNo = inputbuff[3];  //�ſڻ�����
			switch(LocalCfg.Addr[0])
			{
				case 'W':
					if((inputbuff[8] == 0x30)&&(inputbuff[9] == 0x30)&&(inputbuff[10] == 0x30)
							&&(inputbuff[11] == 0x30)&&(inputbuff[12] == 0x30)&&(inputbuff[13] == 0x30))
					{            //����
						memcpy(call_buff, inputbuff + 4, 4);
						call_buff[4] = '\0';
						FindEquip(call_buff);     //����ס��
					}
					else
					{           //��ͨ
						memcpy(call_buff, inputbuff + 4, 10);
						call_buff[10] = '\0';
						FindEquip(call_buff);     //����ס��
					}
					break;
				case 'M':
					memcpy(call_buff, inputbuff + 4, 4);
					call_buff[4] = '\0';
					FindEquip(call_buff);     //����ס��
					break;
			}
			break;
		case 0x58:  //�������
			SendAck(inputbuff, 18);
			system("reboot");
			break;
		case 0x72:
			if(inputbuff[2]==0x32)
			{
				memcpy(send_b,CommTmpBuff,15);
				send_b[12]=inputbuff[4]-0x30;
				send_b[1]=0x0B;
				send_b[2]=0x00;
				send_b[13]=0x00;
				for(i=0;i<13;i++)
				  send_b[13]+=send_b[i];
				UdpSendBuff(m_ForbidSocket,TmpFromIP,send_b,14);
			}
			break;
		case 0x5B:  //����Ӧ��
			time(&t);
			Local.call_tm_t = localtime(&t);//��ǰʱ��
			//if(inputbuff[4] == 0x31)
		//	SendTalkInfo.Status = 0x07;//0x06 call 0x07 pwd
			//else
			SendTalkInfo.Status = 0x06;//0x06 call 0x07 pwd
			SendTalkInfo.Duration =0x00;
		//	SendTalkInfo.Addr[0] = 'S'; 
		//	memcpy(SendTalkInfo.Addr + 1, LocalCfg.Addr + 1, 6);
		//	memcpy(SendTalkInfo.Addr + 7, inputbuff + 5, 4);
			memcpy(SendTalkInfo.Addr , NullAddr, 20);
			memcpy(SendTalkInfo.Addr , Remote.Addr[0], 12);
			
			sprintf(SendTalkInfo.StartTime,"%04d%02d%02d%02d%02d%02d",
					Local.call_tm_t->tm_year+1900,
					Local.call_tm_t->tm_mon + 1,
					Local.call_tm_t->tm_mday,
					Local.call_tm_t->tm_hour,
					Local.call_tm_t->tm_min,
					Local.call_tm_t->tm_sec);
			SendTalkInfo.StartTime[14] = 0x30;
			SendTalkInfo.StartTime[15] = '\0';

			NeedOpenLock = 1;
		case 0x59:  //�����豸�ɹ�Ӧ��
		case 0x5A:  //�����豸ʧ��Ӧ��
		case 0x60:  //�Է�����Ӧ��
		case 0x61:  //�Է�ֹͣ����Ӧ��
		case 0x62:  //ARM��æӦ��
		case 0x63:  //����ʧ��Ӧ��
		case 0x64:  //�Է���æӦ��
		case 0x65:  //�Է���ͨӦ��
		case 0x66:  //���г�ʱӦ��
		case 0x67:  //�Է�����Ӧ��
		case 0x68:  //�Է��һ�Ӧ��
		case 0x69:  //�������Ӧ��
		case 0x6A:  //�������Ӧ��
		case 0x6B:  //IP��ַ��ͻӦ��
		case 0x6C:  //��ӰԤ��
		case 0x6E:  //��Ӱ����Ӧ��
		case 0x6F:  //��������Ӧ��    
			//     printf("inputbuff[4] = %X, inputbuff[5] = %X, inputbuff[6] = %X\n",inputbuff[4],inputbuff[5],inputbuff[6]);
			//����������
			pthread_mutex_lock (&Local.comm_lock);
			if(inputbuff[2] == 0x32)   //Ӧ��
				for(i=0; i<COMMSENDMAX; i++)
					if(Multi_Comm_Buff[i].isValid == 1)
						if(Multi_Comm_Buff[i].SendNum  < MAXSENDNUM)
							if(Multi_Comm_Buff[i].buf[1] == inputbuff[1])  //����
								if(Multi_Comm_Buff[i].buf[2] == 0x31)         //��������
									if(Multi_Comm_Buff[i].buf[3] == inputbuff[3]) //�ſڻ�����
									{
										Multi_Comm_Buff[i].isValid = 0;
										switch(inputbuff[1])
										{
											case 0x59:  //�����豸�ɹ�Ӧ��
#ifdef _DEBUG
												printf("�յ�M8(%d) �����豸�ɹ�Ӧ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x5A:  //�����豸ʧ��Ӧ��
#ifdef _DEBUG
												printf("�յ�M8(%d) �����豸ʧ��Ӧ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x5B:  //����Ӧ��
#ifdef _DEBUG
												printf("�յ�M8(%d) ����Ӧ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x60:  //�Է�����Ӧ��
												//Local.DoorNo = inputbuff[3];  //�ſڻ�����
												//��ʼ¼����Ƶ
												Local.Status = 4;  //״̬Ϊ������
#ifndef _VIDEOZOOMOUT
												StartRecVideo(CIF_W, CIF_H);
#else
												//StartRecVideo(D1_W, D1_H);
												if(Local.OpenCIFVideoFlag != 1)
												  Local.OpenD1VideoFlag = 1;
												else
												  StartRecVideo(CIF_W, CIF_H);
#endif

												Local.CallConfirmFlag = 1; //�������߱�־
												Local.Timer1Num = 0;
												Local.TimeOut = 0;       //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
												Local.OnlineNum = 0;     //����ȷ�����
												Local.OnlineFlag = 1;
#ifdef _DEBUG
												printf("�ſڻ� %d ���ڼ�����...\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x61:  //�Է�ֹͣ����Ӧ��
											//	Local.DoorNo = inputbuff[3];  //�ſڻ�����

#ifdef _DEBUG
												printf("�ſڻ� %d ֹͣ����\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x62:  //ARM��æӦ��
#ifdef _DEBUG
												printf("�յ�M8(%d) ARM��æӦ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x63:  //����ʧ��Ӧ��
#ifdef _DEBUG
												printf("�յ�M8(%d)����ʧ��Ӧ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x64:  //�Է���æӦ��
#ifdef _DEBUG
												printf("�յ�M8(%d) �Է���æӦ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x65:  //�Է���ͨӦ��
												//��ʼ¼����Ƶ
												if(audio_play_wav_flag==0)
												{
													//sprintf(wavFile,"%s6.wav\0","/mnt/mtd/");
													sprintf(wavFile,"%sasong.wav\0","/mnt/");
													StartPlayWav(wavFile,1);
													usleep(150*1000);
												}
												Local.Status = 1;  //״̬Ϊ���жԽ�
												printf("StartRecVideo for %c\n",Remote.Addr[0][0]);
												if((Remote.Addr[0][0]!='D')&&(Remote.Addr[0][0]!='M')&&(Remote.Addr[0][0]!='W'))
												{
#ifndef _VIDEOZOOMOUT
													StartRecVideo(CIF_W, CIF_H);
#else
													if((Local.videozoomout == 1)&&(Local.Status == 1))
													{
														//usleep(200*1000);
														Local.OpenD1VideoFlag = 1;
														//StartRecVideo(D1_W, D1_H);
													}
													else
													  StartRecVideo(CIF_W, CIF_H);
#endif
												}

												Local.CallConfirmFlag = 1; //�������߱�־
												Local.Timer1Num = 0;
												Local.TimeOut = 0;       //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
												Local.OnlineNum = 0;     //����ȷ�����
												Local.OnlineFlag = 1;
#ifdef _DEBUG
												printf("�յ�M8(%d) �Է���ͨӦ��\n", (inputbuff[3] - 0x30));
#endif
												break;
											case 0x66:  //���г�ʱӦ��
												/////////////////////paul0304////////////////////////                
												if(audio_play_wav_flag==1)
													StopPlayWavFile();	
												///////////////////////////////////////////////////////////////////////////////////////////
#ifdef _DEBUG
												printf("�յ�M8(%d) ���г�ʱӦ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x67:  //�Է�����Ӧ��
												//����Ƶ¼�ơ����ţ���Ƶ����
												//   StartPlayVideo(CIF_W, CIF_H);
												StartPlayAudio();
												StartRecAudio();
												Local.Status = 5;  //״̬Ϊ����ͨ��
												Local.TimeOut = 0;       //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
#ifdef _DEBUG
												printf("�յ�M8(%d) �Է�����Ӧ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x68:  //�Է��һ�Ӧ��
												printf("1-�յ�M8(%d) �Է��һ�Ӧ��\n", inputbuff[3] - 0x30);
												if(audio_play_wav_flag==1)
													StopPlayWavFile();	
#ifdef _DEBUG
												printf("�յ�M8(%d) �Է��һ�Ӧ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x69:  //�������Ӧ��
												/////////////////////////paul0619
												Local.DoorNo = inputbuff[3];
#ifdef _DEBUG
												printf("M8(doorno:%d) finish starting\n", inputbuff[3] - 0x30);
#endif
												LocalCfg.Addr[11]=inputbuff[3]-0x01;
												
												
												///WriteCfgFile();
												break;
											case 0x6A:  //�������Ӧ��
#ifdef _DEBUG
												printf("�յ�M8(%d) �������Ӧ��\n", inputbuff[3] - 0x30);
#endif
												break;
											case 0x6B:  //IP��ַ��ͻӦ��
#ifdef _DEBUG
												printf("�յ�M8(%d) IP��ַ��ͻӦ��\n", inputbuff[3] - 0x30);
#endif
												break;
										}
										break;
									}
			//�򿪻�����
			pthread_mutex_unlock (&Local.comm_lock);
			break;
	}

}
//---------------------------------------------------------------------------
//��������
void OpenLock_Func(void)
{
	if(LocalCfg.DelayLockTime == 0)
	{
		Local.OpenDoorFlag = 2;   //������־ 0 δ����  1 ������ʱ��  2 ������
		Local.OpenDoorTime = 0;   //ʱ��
		//������
		ioctl(gpio_fd, IO_PUT, 0);
	}
	else
	{
		Local.OpenDoorFlag = 1;   //������־ 0 δ����  1 ������ʱ��  2 ������
		Local.OpenDoorTime = 0;   //ʱ��
	}
}
//---------------------------------------------------------------------------
//�洢����
void Save_Setup(void)
{
	int i;
	//д���ļ�
	//����������
	pthread_mutex_lock (&Local.save_lock);
	//���ҿ��ô洢���岢���
	for(i=0; i<SAVEMAX; i++)
		if(Save_File_Buff[i].isValid == 0)
		{
			Save_File_Buff[i].Type = 4;      //�洢��������
			Save_File_Buff[i].isValid = 1;
			sem_post(&save_file_sem);     
			break;
		}
	//�򿪻�����
	pthread_mutex_unlock (&Local.save_lock);

}
//---------------------------------------------------------------------------
void Comm3_RcvThread(int fd)  //Comm3�����̺߳���
{
	struct timeval tv;
	uint32_t prev_comm_time;
	uint32_t nowtime;
	int len;
	int i;
	char buff[128];
	char FromIP[20];
	unsigned char validbuff[200];
	char send_b[100];
	unsigned char m_crc;
	int validlen;
	struct commbuf1 commbuf;
	printf("This is comm3 pthread.\n");
	//��һ�δ������ݽ���ʱ��
	gettimeofday(&tv, NULL);
	prev_comm_time = tv.tv_sec *1000 + tv.tv_usec/1000;  
	while (CommRecvFlag == 1)     //ѭ����ȡ����
	{
		while(InitSuccFlag == 0)
			usleep(10000);
		while((len = read(fd,buff,512))>0)
		{
			gettimeofday(&tv,NULL);
			nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;
			//����һ�ν��ճ���50ms,���ж�Ϊ��ʱ
			if((nowtime - prev_comm_time) >= 50)
			{
				commbuf.iget = 0;
				commbuf.iput = 0;
				commbuf.n = 0;
			}
			prev_comm_time = nowtime;

			printf("Len %d Comm3\n",len);
			for(i=0; i<len; i++)
			  printf("0x%02X, ", buff[i]);
			  printf("\n");    
			 
			buff[len] = '\0';
			memcpy(commbuf.buffer + commbuf.iput, buff, len);
			commbuf.iput += len;
			if(commbuf.iput >= COMMMAX)
				commbuf.iput = 0;
			commbuf.n += len;
			while(commbuf.n >= 6)
			{
				if(commbuf.buffer[commbuf.iget] == 0x7E)
				{
					if(commbuf.n >= 18)
					{
						memcpy(validbuff, commbuf.buffer + commbuf.iget, 18);
						commbuf.iget += 18;
						if(commbuf.iget >= COMMMAX)
							commbuf.iget = 0;
						commbuf.n -= 18;
						if((validbuff[0] == 0x7E)&&(validbuff[17] == 0x0D))
						{
							m_crc = 0;
							for(i=1; i<=15; i++)
								m_crc += validbuff[i];
							if((m_crc == 0x7E)||(m_crc == 0x0D))
								m_crc -= 1;
							//printf("comm3:m_crc = 0x%X, validbuff[16] = 0x%X\n", m_crc, validbuff[16]);
							if(m_crc == validbuff[16])
							{
								//for(i=0;i<18;i++)
								//	printf("%02x,",validbuff[i]);
								//printf("\n");
								M8Input_Func(validbuff); //M8����
							}
							else
							  printf("crc error\n"); 
						}
					}
					else
						break;
				}
				else if(commbuf.buffer[commbuf.iget] == 0xBD)
				{
					validlen=commbuf.buffer[commbuf.iget +1]+3;
					if (commbuf.n >=validlen)
					{
						memcpy(validbuff, commbuf.buffer + commbuf.iget, validlen);
						commbuf.iget += validlen;
						if (commbuf.iget >= COMMMAX)
							commbuf.iget = 0;
						commbuf.n -= validlen;
						if ((validbuff[0] == 0xBD))
						{
							m_crc = 0;
							for (i=0; i< validlen-1; i++)
								m_crc += validbuff[i];
							if (m_crc == validbuff[validlen-1])
							{
 

								//    printf("comm3-buffer[2]=%02x\n",validbuff[2]);
								//								if((validbuff[2] ==0x12))//||(commbuf.buffer[commbuf.iget + 7]==0x00))
								//									UpdateMCU_Func(validbuff[7]);
								//								else
								{
									switch(validbuff[2])
									{

										case 0x12:

											UpdateMCU_Func(validbuff[7]);
											break;
										//printf("\nthe data is 11111111111111");
										case 0x20:
											if((validbuff[7]==0x00)&&(validbuff[8]==0x00)&&(validbuff[9]==0x00)&&(validbuff[10]==0x00))
											{

												sprintf(FromIP,"%d.%d.%d.%d",LocalCfg.IP_Server[0],LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);	
												validbuff[validbuff[1]+2]=validbuff[validbuff[1]+2]-validbuff[3]-validbuff[4];

												validbuff[3]=((LocalCfg.Addr[3]-'0')<<4)|((LocalCfg.Addr[4]-'0')&0x0F);
												validbuff[4]=((LocalCfg.Addr[5]-'0')<<4)|((LocalCfg.Addr[6]-'0')&0x0F);
												validbuff[validbuff[1]+2]=validbuff[validbuff[1]+2]+validbuff[3]+validbuff[4];
												for(i=0; i<validlen; i++)
													printf("0x%02x, ", validbuff[i]);
												printf("\n");   
												UdpSendBuff(m_ForbidSocket,FromIP,validbuff,validbuff[1]+3);
											}
											else if((validbuff[7]==0x0A)&&(validbuff[8]==0x0A)&&(validbuff[9]==0x00)&&(validbuff[10]==0x50))
											{
												printf("\nsendopenlock\n");
												SendOpenLock(validbuff);
											}

											else 
											{
												printf("\nthe Comm3 data is go to Comm3");
												CommSendBuff(Comm2fd,validbuff,validbuff[1]+3);
												//CommSendBuff(Comm3fd,validbuff,validbuff[1]+3);
											}
											break;
										case 0x00:
											if((validbuff[3]==0x00)&&(validbuff[4]==0x00)&&(validbuff[5]==0x00)&&(validbuff[6]==0x00))
											{


												printf("\nthe new Comm3 data is go to Udp");
												/////////0812
												if((validbuff[9]!=0x0A)&&(validbuff[10]!=0x0A))
												{
													validbuff[validbuff[1]+2]=validbuff[validbuff[1]+2]-validbuff[7]-validbuff[8];

													validbuff[7]=((LocalCfg.Addr[3]-'0')<<4)|((LocalCfg.Addr[4]-'0')&0x0F);
													validbuff[8]=((LocalCfg.Addr[5]-'0')<<4)|((LocalCfg.Addr[6]-'0')&0x0F);
													validbuff[validbuff[1]+2]=validbuff[validbuff[1]+2]+validbuff[7]+validbuff[8];
												}
												printf("\nSENDING:::::");
												for(i=0; i<validlen; i++)
													printf("0x%02x, ", validbuff[i]);
													printf("\n");
												sprintf(FromIP,"%d.%d.%d.%d",LocalCfg.IP_Server[0],LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
												if((validbuff[10]<=0x07))
												{//villa
													for(i = 0; i<10; i++)
													{
														if(villabuf[i][0] == validbuff[11])
														{
															memcpy(send_b,villabuf[i]+1,54);
															send_b[7] = 0x02;
															memcpy(send_b + 54, validbuff, validbuff[1]+3);
															Local.DoorNo = validbuff[13]+0x31;
															UdpSendBuff(m_DataSocket, FromIP, send_b , 54 + validbuff[1] + 3);
															villabuf[i][0] = 0x00;
															break;

														}
													}

												}
												else
												{///forbid
													printf("\nSENDING::");
													for(i=0; i<validlen; i++)
														printf("0x%02x, ", validbuff[i]);
													printf("\n");

													UdpSendBuff(m_ForbidSocket,FromIP,validbuff,validlen);
													//UdpSendBuff(m_ForbidSocket,FromIP,validbuff,validbuff[1]+3);
												}



											}
											else
											{
												printf("\nSENDING:::");
													for(i=0; i<validlen; i++)
														printf("0x%02x, ", validbuff[i]);
													printf("\n");

													sprintf(FromIP,"%d.%d.%d.%d",LocalCfg.IP_Server[0],LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
													UdpSendBuff(m_ForbidSocket,FromIP,validbuff,validlen);
											}
											break;
										default:
											CommSendBuff(Comm2fd,validbuff,validbuff[1]+3);
											break;

									}//switch

								}

							}
							else
								printf("crc error\n");
						}
					}
					else
						break;

				}
				else
				{
					commbuf.iget ++;
					if(commbuf.iget >= COMMMAX)
						commbuf.iget = 0;
					commbuf.n --;
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void Comm4_RcvThread(int fd)  //Comm4�����̺߳���
{
	struct timeval tv;
	uint32_t prev_comm_time;
	uint32_t nowtime;
	int len,validlen;
	int i;
	unsigned char buff[128];
	////////////////////////////////////////////
	char FromIP[20];
	unsigned char send_b[20];
	////////////////////////////////////////////
	unsigned char validbuff[512];
	unsigned char m_crc;
	struct commbuf1 commbuf;
	printf("This is comm4 pthread.\n");
	//��һ�δ������ݽ���ʱ��
	gettimeofday(&tv, NULL);
	prev_comm_time = tv.tv_sec *1000 + tv.tv_usec/1000;
	while (CommRecvFlag == 1)     //ѭ����ȡ����
	{
		//ϵͳ��ʼ����־,��û��?�ʼ�������ȴ?
		while (InitSuccFlag == 0)
			usleep(10000);
		while ((len = read(fd, buff, 512))>0)
		{
			gettimeofday(&tv, NULL);
			nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;
			//����һ�ν��ճ���50ms,���ж�Ϊ��ʱ
			if ((nowtime - prev_comm_time) >= 50)
			{
				commbuf.iget = 0;
				commbuf.iput = 0;
				commbuf.n = 0;
			}
			prev_comm_time = nowtime;

			printf("Len %d Comm4:",len);
			buff[len] = '\0';
			for(i=0;i<len;i++)
				printf(" %02x ",buff[i]);

			printf("\n");
			memcpy(commbuf.buffer + commbuf.iput, buff, len);
			commbuf.iput += len;
			if (commbuf.iput >= COMMMAX)
				commbuf.iput = 0;
			commbuf.n += len;
			while (commbuf.n >= 6)
			{	
				if (commbuf.buffer[commbuf.iget] ==0xBD)
				{
					validlen=commbuf.buffer[commbuf.iget +1]+3;
					if (commbuf.n >=validlen)
					{
						memcpy(validbuff, commbuf.buffer + commbuf.iget, validlen);
						commbuf.iget += validlen;
						if (commbuf.iget >= COMMMAX)
							commbuf.iget = 0;
						commbuf.n -= validlen;
						if ((validbuff[0] == 0xBD))
						{
							m_crc = 0;
							for (i=0; i< validlen-1; i++)
								m_crc += validbuff[i];
							printf("comm4:m_crc = 0x%X, validbuff[1] = 0x%X\n", m_crc, validbuff[1]);
							if (m_crc == validbuff[validlen-1])
							{
								if((validbuff[2]==0x12))//||(commbuf.buffer[commbuf.iget + 7]==0x00))
									UpdateMCU_Func(validbuff[7]);
								else if(validbuff[2] == 0x06)
								{
									memcpy(send_b,validbuff,9);
									send_b[1] = 0x06;
									send_b[2] = 0x00;
									send_b[7] = 0x00;
									send_b[8] = 0;
									for(i=0;i<send_b[1]+3-1;i++)
									{
										send_b[8] += send_b[i];
									}
									CommSendBuff(Comm4fd, send_b, 9);
								}
								else
								{
									///////////////////////////////////////////////////////////////////////////////////////////
									sprintf(FromIP,"%d.%d.%d.%d",LocalCfg.IP_Server[0],LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
									printf("The Com4 DATA send to UDP\n");
									UdpSendBuff(m_ForbidSocket,FromIP,validbuff,validbuff[1]+3);
									///////////////////////////////////////////////////////////////////////////////////////////
									//CommMJ_Func(validbuff); //M8����
								}
							}
							else
								printf("crc error\n");
						}
					}
					else
						break;
				}
				else
				{
					commbuf.iget ++;
					if (commbuf.iget >= COMMMAX)
						commbuf.iget = 0;
					commbuf.n --;
				}
			}
		}
	}

	/*
	   int len;
	   char buff[128];
	   printf("This is comm4 pthread.\n");  
	   while (CommRecvFlag == 1)     //ѭ����ȡ����
	   {
	   while((len = read(fd, buff, 512))>0)
	   {
	   printf("\nLen %d\n",len);
	   buff[len+1] = '\0';
	   printf( "\n%s", buff);
	   if(strcmp(buff,"exit")==0)
	   {
	   printf("recvfrom888888888");
	   CommRecvFlag=0;
	//   break;
	}
	}
	}*/
}
//---------------------------------------------------------------------------
void CloseComm()
{
	//Comm2���ݽ����߳�
	CommRecvFlag = 0;
	usleep(40*1000);
	//  pthread_cancel(comm2_rcvid);
	pthread_cancel(comm3_rcvid);
	//  pthread_cancel(comm4_rcvid);
	//  close(Comm2fd);
	close(Comm3fd);
	//  close(Comm4fd);
}
//---------------------------------------------------------------------------
////////////PAUL 0328////////CHANGE COMM3FD TO COMM2FD
int CommSendBuff(int fd,unsigned char buf[1024],int nlength)
{
	int nByte;
	int i;
	// if(fd == Comm3fd)
	//����3 RS-485�շ����ƣ��͵�ƽ���գ��ߵ�ƽ����
	ioctl(gpio_fd, IO_PUT, 7);

	nByte = write(fd, buf ,nlength);
    //printf("COMM %d SENDING:",fd);
	//for(i=0;i<nByte;i++)
	//	printf(" %02x ",buf[i]);
	//printf("\n");
	//if(fd == Comm3fd)
	{
		tcdrain(fd);     //�ȴ�Ӳ���������
		//����3 RS-485�շ����ƣ��͵�ƽ���գ��ߵ�ƽ����
		ioctl(gpio_fd, IO_CLEAR, 7);
	}          
	//  printf("send buff\n");
	return nByte;
}
//---------------------------------------------------------------------------
////////////updatemcu
void UpdateMCU_Func(char valid)
{
	char sendbuf[150];
	int i;
	if(Local.updatemcu==0x01)
	{
		pthread_mutex_lock (&Local.comm_lock);		
		for(i=0;i<COMMSENDMAX;i++)
			if(Multi_Comm_Buff[i].isValid == 1)
				Multi_Comm_Buff[i].isValid = 0;
		pthread_mutex_unlock (&Local.comm_lock);
		if(valid==0x00)
		{
			if(McuUpdate.Curr<McuUpdate.Pack)
			{
				sendbuf[0]=0xBD;
				sendbuf[1]=0x8B;
				sendbuf[2]=0x11;

				memset(sendbuf+3,0,4);
				//	memcpy(sendbuf+7,McuUpdate.Towho,4);
				//	��Ԫ
				/*		sendbuf[7]=0x0a;
						sendbuf[8]=0x0a;
						sendbuf[9]=0x00;
						sendbuf[10]=0x00;
						*/
				//	��Ԫ�Ž�

				sendbuf[7]= McuUpdate.Towho[0];
				sendbuf[8]=  McuUpdate.Towho[1];
				sendbuf[9]= McuUpdate.Towho[2];
				sendbuf[10]= McuUpdate.Towho[3];

				//�����Ž�
				/*
				   sendbuf[7]=0x00;
				   sendbuf[8]=0x00;
				   sendbuf[9]=0x10;
				   sendbuf[10]=0x01;
				   */
				//	memcpy(sendbuf+11,McuUpdate.Curr,2);
				sendbuf[11]=McuUpdate.Curr&0xFF;
				sendbuf[12]=McuUpdate.Curr>>8;

				memcpy(sendbuf+13,McuUpdate.McuData+(128*McuUpdate.Curr),128);

				sendbuf[141]=0x00;
				for(i=0;i<141;i++)
					sendbuf[141] += sendbuf[i];
				//				CommSendBuff(McuUpdate.commfd,sendbuf,0x8E);
				//if(McuUpdate.Curr==0)

				//	usleep(300*1000);
				SendUpdateOrder(McuUpdate.commfd,0x8B,0x11,sendbuf+3);
				printf("Updateing %d:\n",McuUpdate.Curr);

			}
			else
			{
				///////////0803
				usleep(300*1000);
				Local.updatemcu = 0x00;
				//inform the center good news
				printf("FINISHUPDATING\n");
			}
			memcpy(sendbuf,UdpPackageHead,6);
			sendbuf[6] = 165;
			sendbuf[7] = 3;
			//memcpy(sendbuf+8,McuData,9);
			sendbuf[8] = McuUpdate.Type;
			sendbuf[9] = (McuUpdate.Length&0xFF);
			sendbuf[10] = (McuUpdate.Length>>8)&0xFF;
			sendbuf[11] = (McuUpdate.Length>>16)&0xFF;
			sendbuf[12] = (McuUpdate.Length>>24)&0xFF;
			//memcpy(sendbuf+9,McuUpdate.Length,4);
			sendbuf[13] = McuUpdate.Pack & 0xFF;
			sendbuf[14] = (McuUpdate.Pack >> 8) & 0xFF;
			//memcpy(sendbuf+13,McuUpdate.Pack,2);
			sendbuf[15] = McuUpdate.Curr & 0xFF;
			sendbuf[16] = (McuUpdate.Curr >> 8) & 0xFF;
			//memcpy(sendbuf+15,McuUpdate.Curr,2);
			//	if(McuUpdate.Curr>0)
			UdpSendBuff(McuUpdate.Socket,McuUpdate.FromIP,sendbuf,18);

			McuUpdate.Curr++;
		}
		else //if(valid == 0x02)
		{
			//inform the center bad news
			Local.updatemcu = 0x00;
			printf("BAD UPDATE\n");
			memcpy(sendbuf,UdpPackageHead,6);
			sendbuf[6] = 165;
			sendbuf[7] = 4;
			//memcpy(sendbuf+8,McuData,9);
			sendbuf[8] = McuUpdate.Type;
			//sendbuf[9] = McuUpdate.Length;
			/*memcpy(sendbuf+9,McuUpdate.Length,4);
			  memcpy(sendbuf+13,McuUpdate.Pack,2);
			  memcpy(sendbuf+15,McuUpdate.Curr,2);
			  */
			sendbuf[9] = (McuUpdate.Length&0xFF);
			sendbuf[10] = (McuUpdate.Length>>8)&0xFF;
			sendbuf[11] = (McuUpdate.Length>>16)&0xFF;
			sendbuf[12] = (McuUpdate.Length>>24)&0xFF;
			//memcpy(sendbuf+9,McuUpdate.Length,4);
			sendbuf[13] = McuUpdate.Pack & 0xFF;
			sendbuf[14] = (McuUpdate.Pack >> 8) & 0xFF;
			//memcpy(sendbuf+13,McuUpdate.Pack,2);
			sendbuf[15] = McuUpdate.Curr & 0xFF;
			sendbuf[16] = (McuUpdate.Curr >> 8) & 0xFF;
			//		if(McuUpdate.Curr>0)
			UdpSendBuff(McuUpdate.Socket,McuUpdate.FromIP,sendbuf,18);

		}
	}
}
void SendUpdateOrder(int commfd,int length, char order, char * param) 
{
	int i,j;
	unsigned char crc;
	pthread_mutex_lock (&Local.comm_lock);
	for(i=0; i<COMMSENDMAX; i++)
		if(Multi_Comm_Buff[i].isValid == 0)
		{
			Multi_Comm_Buff[i].SendNum = 0;
			Multi_Comm_Buff[i].m_Comm = commfd;

			Multi_Comm_Buff[i].buf[0] = 0xBD;   
			Multi_Comm_Buff[i].buf[1] = length;   
			Multi_Comm_Buff[i].buf[2] = order;   
			//Multi_Comm_Buff[i].buf[3] = TerNo;   
			memcpy(Multi_Comm_Buff[i].buf+3,param,length-1);
			//Multi_Comm_Buff[i].buf[] = InfoType;   
			//Multi_Comm_Buff[i].buf[5] = Info;   
			crc = 0;
			for(j=0; j< length + 2; j++)
				crc += Multi_Comm_Buff[i].buf[j];
			Multi_Comm_Buff[i].buf[length + 2] = crc;
			//Multi_Comm_Buff[i].buf[7] = 0xFE;

			Multi_Comm_Buff[i].nlength = length +3;
			Multi_Comm_Buff[i].isValid = 1;
			sem_post(&multi_comm_send_sem);
			break;
		}
	pthread_mutex_unlock (&Local.comm_lock);
}
//////////////////////////////////////////////////////////////
void SendDoorStatus(char *buf)
{
	int i;
	printf("SendDoorStatus\n");
	pthread_mutex_lock (&Local.udp_lock);

	for(i=0;i<UDPSENDMAX;i++)
	{
		if(Multi_Udp_Buff[i].isValid==0)
		{
			memcpy(Multi_Udp_Buff[i].buf,buf,22);
			Multi_Udp_Buff[i].SendNum=0;
			Multi_Udp_Buff[i].m_Socket = m_ForbidSocket;
			Multi_Udp_Buff[i].CurrOrder = 0x04;
			sprintf(Multi_Udp_Buff[i].RemoteHost,"%d.%d.%d.%d",LocalCfg.IP_Server[0],LocalCfg.IP_Server[1],LocalCfg.IP_Server[2],LocalCfg.IP_Server[3]);
			Multi_Udp_Buff[i].nlength=0x16;
			Multi_Udp_Buff[i].isValid=1;
			Multi_Udp_Buff[i].DelayTime = 100;


			sem_post(&multi_send_sem);
			i = UDPSENDMAX;
			break;
		}
	}
    pthread_mutex_unlock (&Local.udp_lock);
}
void SendOpenLock(char *buf)
{
	int i;

	pthread_mutex_lock (&Local.udp_lock);
	for(i=0;i<UDPSENDMAX;i++)
	{
		
		if(Multi_Udp_Buff[i].isValid==0)
		{
			Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_DataSocket;
				strcpy(Multi_Udp_Buff[i].RemoteHost, LFTMULTIADDR);
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				Multi_Udp_Buff[i].buf[6] = LIFT;
				Multi_Udp_Buff[i].buf[7] = ASK;    //Ö÷½Ð
				Multi_Udp_Buff[i].buf[8] = OPENLOCK;

				memcpy(Multi_Udp_Buff[i].buf+9, LocalCfg.Addr, 20);

				memcpy(Multi_Udp_Buff[i].buf+29,buf,15);
				Multi_Udp_Buff[i].nlength = 45;
				Multi_Udp_Buff[i].DelayTime = 100;
				Multi_Udp_Buff[i].isValid = 1;

				sem_post(&multi_send_sem);
				break;
		}
	}
    pthread_mutex_unlock (&Local.udp_lock);
}
