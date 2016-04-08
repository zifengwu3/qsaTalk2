#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>       //sem_t
#include <semaphore.h>       //sem_t
#include <arpa/inet.h>

#define _DEBUG           //����ģʽ
#define HARDWAREVER "M-HW VER 1.0"    //Ӳ���汾
#define SOFTWAREVER "VER 1.19"    //testר�ð汾 add: m_videosendsocket, ����������,�����Ŵż��͵�Դ���,�������鲥�������ĳ���
#define SERIALNUM "20080318"    //��Ʒ���к�

#define LFTMULTIADDR  "238.9.9.2"//"192.168.10.255"  //NS�鲥��ַ
#define NSMULTIADDR  "238.9.9.1"  //NS�鲥��ַ

#define WAVFILEMAX 10
#define INTRTIME 20       //�߳�20ms
#define INTRPERSEC 50       //ÿ��50���߳�

#define IDCARDMAXNUM  100000  //ID���������
#define CARDPERPACK  350  //ÿ�����ݰ���ID���������
#define IDCARDBRUSHNUM 10000 //�������ˢ������
#define COMMMAX 1024     //���ڻ��������ֵ
#define SCRWIDTH  800
#define SCRHEIGHT  480
#define REFRESH  1
#define NOREFRESH 0
#define SHOW  0
#define HIDE  1
#define UMOUNT  0
#define MOUNT   1
#define HILIGHT  2
#define IMAGEUP  0
#define IMAGEDOWN  1
#define D1_W  720
#define D1_H  480
#define CIF_W    352
#define CIF_H    240

#define LIFT 110
#define SETPWD 0x54///84

#define TIPW     320     //��ʾ�����
#define TIPH     24      //��ʾ���߶�
#define TIPX     320     //��ʾ��X
#define TIPY     24      //��ʾ��Y
#define CLOCKW     260     //ʱ�ӿ��
#define CLOCKH     24      //ʱ�Ӹ߶�
#define CLOCKX     380     //ʱ��X
#define CLOCKY     5       //ʱ��Y

#define CALLCENTERX     50     //��������X
#define CALLCENTERY     380    //��������Y
#define CC_TALKTIMEW     120    //ͨ����ʱW
#define CC_TALKTIMEH     24    //ͨ����ʱH
#define CC_TALKTIMEX     50     //ͨ����ʱX
#define CC_TALKTIMEY     410    //ͨ����ʱY

#define R2RX     50             //����ͨ��X
#define R2RY     380            //����ͨ��Y
#define R2R_TALKTIMEW     120    //ͨ����ʱW
#define R2R_TALKTIMEH     24    //ͨ����ʱH
#define R2R_TALKTIMEX     50     //ͨ����ʱX
#define R2R_TALKTIMEY     410    //ͨ����ʱY

#define WATCHX     50     //����X
#define WATCHY     380    //����Y
#define W_TALKTIMEW     120    //ͨ����ʱW
#define W_TALKTIMEH     24    //ͨ����ʱH
#define W_TALKTIMEX     50     //ͨ����ʱX
#define W_TALKTIMEY     410    //ͨ����ʱY

#define CALLX     50     //����X
#define CALLY     380    //����Y
#define C_TALKTIMEW     120    //ͨ����ʱW
#define C_TALKTIMEH     24    //ͨ����ʱH
#define C_TALKTIMEX     50     //ͨ����ʱX
#define C_TALKTIMEY     410    //ͨ����ʱY

#define COMMSRVX     50     //��ҵ����X
#define COMMSRVY     380    //��ҵ����Y

#define COMMSRV_HH   32     //��ҵ�������H
#define COMMSRV_HW   180     //��ҵ�������W
#define COMMSRV_HX   60     //��ҵ�������X
#define COMMSRV_HY   80     //��ҵ�������Y

#define SETUP1X     50     //���ô���1X
#define SETUP1Y     380    //���ô���1Y

#define INFOBOXW     390    //��Ϣ��ʾ��W
#define INFOBOXH     330    //��Ϣ��ʾ��H
#define INFOBOXX     300     //��Ϣ��ʾ��X
#define INFOBOXY     50-4    //��Ϣ��ʾ��Y

#define INFOBUTTONW     590    //��Ϣ��ť����W
#define INFOBUTTONH     40    //��Ϣ��ť����H
#define INFOBUTTONX     100     //��Ϣ��ť����X
#define INFOBUTTONY     380    //��Ϣ��ť����Y

#define INFONUMW     70    //��Ϣ����Ϣ��δ����ϢW
#define INFONUMH     24    //��Ϣ����Ϣ��δ����ϢH
#define INFONUMX     224     //��Ϣ����Ϣ��δ����ϢX
#define INFONUMY     60    //��Ϣ����Ϣ��δ����ϢY

#define INFONUM2X     230     //��Ϣ����Ϣ��δ����ϢX
#define INFONUM2Y     60+48    //��Ϣ����Ϣ��δ����ϢY

#define INFONUM3X     230     //��Ϣ����Ϣ��δ����ϢX
#define INFONUM3Y     60+48*2    //��Ϣ����Ϣ��δ����ϢY

#define INFONUM4X     230     //��Ϣ����Ϣ��δ����ϢX
#define INFONUM4Y     60+48*3    //��Ϣ����Ϣ��δ����ϢY

#define INFOROWW     370    //������Ϣ�������͸�����ʾ
#define INFOROWH     32
#define INFOROWX     300
#define INFOROWY     60

#define LWORDNUMW     230    //��Ϣ����Ϣ��δ����ϢW
#define LWORDNUMH     24    //��Ϣ����Ϣ��δ����ϢH
#define LWORDNUMX     100     //��Ϣ����Ϣ��δ����ϢX
#define LWORDNUMY     60    //��Ϣ����Ϣ��δ����ϢY

#define LWORDBOXW     420    //������ʾ��W
#define LWORDBOXH     230    //������ʾ��H
#define LWORDBOXX     140     //������ʾ��X
#define LWORDBOXY     96    //������ʾ��Y

#define USERW     96    //ʹ��������
#define USERH     30
#define USERX     40
#define USERY     50

#define WATCHTIMEOUT  32*INTRPERSEC//30*20    //�����ʱ��
#define CALLTIMEOUT  30*INTRPERSEC     //�����ʱ��
#define TALKTIMEOUT  130*INTRPERSEC//30*20     //ͨ���ʱ��
#define PREPARETIMEOUT  10*INTRPERSEC     //��Ӱ����Ԥ���ʱ��
#define RECORDTIMEOUT  30*INTRPERSEC     //��Ӱ�����ʱ��

//���� ��������
#define ALARM  1
#define SENDMESSAGE   3   //������Ϣ
#define REPORTSTATUS  4   //�豸��ʱ����״̬
#define QUERYSTATUS   5   //�������Ĳ�ѯ�豸״̬
#define SENDCOMMSRV   7   //������ҵ����
#define WRITEADDRESS   40   //д��ַ����
#define READADDRESS    41   //����ַ����
#define WRITECOMMMENU   42   //д��ҵ����˵�
#define READCOMMMENU   43   //����ҵ����˵�
#define WRITEHELP   50   //д������Ϣ
#define READHELP    51   //��������Ϣ
#define WRITESETUP     52   //д������Ϣ
#define READSETUP      53   //��������Ϣ
#define WRITEIDCARD    54   //дID����Ϣ
#define READIDCARD     55   //��ID����Ϣ
#define BRUSHIDCARD     56   //ˢID����Ϣ�������������
//�Խ�
#define VIDEOTALK      150 //���������ӶԽ�
#define VIDEOTALKTRANS 151 //���������ӶԽ���ת����
#define VIDEOWATCH     152 //���������
#define VIDEOWATCHTRANS   153 //�����������ת����
#define NSORDER        154 //���������ң������ڶಥ��
#define NSSERVERORDER  155 //����������(NS������)
#define FINDEQUIP      170 //�����豸
/////////////paul0723
#define CHECKMAC	161
#define SETPARAM 	162
#define MAKEOLD		163
#define READMAC		164
#define MCUUPDATE	165

#define READVILLA   170
#define SETVILLA    171
/////////////////////////////
//0x80 ����ͨ����Ϣ�����Ļ�
//0x81 ���Ϳ�����Ϣ�����Ļ�//zhou101123
#define SENDTALKINFO   0x80 //ͨ����Ϣ���� �ɵ�55λ
////////////////////////
#define ASK              1     //�������� ����
#define REPLY            2     //�������� Ӧ��

#define CALL             1     //����
#define LINEUSE          2     //ռ��
#define QUERYFAIL        3      //ͨ��ʧ��
#define CALLANSWER       4     //����Ӧ��
#define CALLSTART        6     //��ʼͨ��

#define CALLUP           7     //ͨ������1�����з�->���з���
#define CALLDOWN         8     //ͨ������2�����з�->���з���
#define CALLCONFIRM      9     //ͨ������ȷ�ϣ����շ����ͣ��Ա㷢�ͷ�ȷ�����ߣ�
#define REMOTEOPENLOCK   10     //Զ�̿���
#define FORCEIFRAME      11     //ǿ��I֡����
#define ZOOMOUT          15     //�Ŵ�(720*480)
#define ZOOMIN           16     //��С(352*288)
#define OPENLOCK         17
#define CALLEND          30     //ͨ������

#define UDPSENDMAX  50  //UDP��η��ͻ������ֵ
#define MAXCOMMSENDNUM  3
#define MAXSENDNUM  6  //����ʹ���
//��ťѹ��ʱ��
#define DELAYTIME  300

#define NMAX 512*64  //AUDIOBLK*64  //��Ƶ���λ�������С
#define G711NUM  64*512/AUDIOBLK       //��Ƶ���ջ��������� δ����   10

#define VIDEOMAX 720*576
#define VNUM  3         //��Ƶ�ɼ���������С
#define VPLAYNUM  10         //��Ƶ���Ż�������С         6
#define MP4VNUM  10         //��Ƶ���ջ��������� δ����   10
#define PACKDATALEN  1200   //���ݰ���С
#define MAXPACKNUM  100     //֡������ݰ�����

struct TimeStamp1{
	unsigned int OldCurrVideo;     //��һ�ε�ǰ��Ƶʱ��
	unsigned int CurrVideo;
	unsigned int OldCurrAudio;     //��һ�ε�ǰ��Ƶʱ��
	unsigned int CurrAudio;
};

struct WaveFileHeader
{
	char chRIFF[4];
	uint32_t dwRIFFLen;
	char chWAVE[4];

	char chFMT[4];
	uint32_t dwFMTLen;
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
	uint16_t wBitsPerSample;

	char chFACT[4];
	uint32_t dwFACTLen;

	char chDATA[4];
	uint32_t dwDATALen;
};

struct Local1{
	int Status;
	//״̬ 0 ���� 1 ���жԽ�  2 ���жԽ�  3 ����  4 ������  5 ����ͨ��
	//6 ����ͨ�� 
	int DoorNo;
	char FindEquip[20];
	int CallConfirmFlag; //���߱�־
	int Timer1Num;  //��ʱ��1����
	int OnlineFlag; //��������ȷ��
	int OnlineNum;  //����ȷ�����
	int TimeOut;    //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
	int RecPicSize;  //��Ƶ��С  1  352*288   2  720*480
	int PlayPicSize;  //��Ƶ��С  1  352*288   2  720*480
	pthread_mutex_t save_lock;//������
	pthread_mutex_t udp_lock;//������
	pthread_mutex_t comm_lock;//������
	int PassLen;            //���볤��
	int ForceIFrame;    //1 ǿ��I֡
	char ClockText[20];  //��ǰʱ��
	int InputMax;       //������󳤶�  Χǽ��  10λ  ��Ԫ�ſڻ�  4λ
	unsigned char IP_Group[4];  //�鲥��ַ

	struct tm *call_tm_t;
	int AddrLen;          //��ַ����  S 12  B 6 M 8 H 6 
	int villacount;
	int isHost;           //'0' ���� '1' ���� '2' ... 
	int DenNum;             //Ŀ������  ����
	unsigned char DenIP[10][4];    //����IP
	char DenAddr[10][21];         //����Addr

	int NetStatus;   //����״̬ 1 �Ͽ�  0 ��ͨ
	int OldNetSpeed;  //�����ٶ� 

	int ReportSend;  //�豸��ʱ����״̬�ѷ���
	int RandReportTime; //�豸��ʱ����״̬���ʱ��
	int ReportTimeNum;  //��ʱ                   

	int OpenDoorFlag; //������־ 0 δ����  1 ������ʱ��  2 ������
	int OpenDoorTime; //ʱ��
	//��GPIO�߳��в�ѯ���߳��Ƿ�����
	int Multi_Send_Run_Flag;

	int nowvideoframeno;   //��ǰ��Ƶ֡���
	int nowaudioframeno;   //��ǰ��Ƶ֡���
	char updatemcu;
	char leftmessage;
	char reportinfo;
	//zhou110809
	int videozoomout;
	int OpenD1VideoFlag;//��D1��Ƶ���
	int OpenCIFVideoFlag;//��CIF��Ƶ���
	//zhou110907
	int WaitforEnd;//���У����ӽ���״̬�����ִ�Լ2S��ʱ�䣬��ʱ���ڣ������������ж���æ����
	int WaitTimeOut; //�ȴ���ʱʱ��
#ifdef ISSETUPPACKETSIZE
	unsigned char minpacket_flag;
#endif
};
struct LocalCfg1{
	char Addr[20];             //��ַ����
	unsigned char Mac_Addr[6]; //������ַ
	unsigned char IP[4];       //IP��ַ
	unsigned char IP_Mask[4];  //��������
	unsigned char IP_Gate[4];  //���ص�ַ
	unsigned char IP_NS[4];    //NS�����Ʋ��ң���������ַ
	unsigned char IP_Server[4];  //����������ַ����NS��������Ϊͬһ����
	unsigned char IP_Broadcast[4];  //�㲥��ַ

	int ReportTime;      //�豸��ʱ����״̬ʱ��

	unsigned char OpenLockTime;   //����ʱ��  0 200ms  1 500ms  2 5s  3 10s
	unsigned char DelayLockTime;   //��ʱ����  0 0s  1 3s  2 5s  3 10s
	unsigned char PassOpenLock;   //���뿪��
	unsigned char CardOpenLock;   //ˢ������

	unsigned char bit_rate;        //��ƵCIFʱ����    0-128k    1-256k   2-384k    3-512k   4-768  5-1024 

	char EngineerPass[10];           //��������
	char OpenLockPass[10];             //��������
};
//ID����Ϣ
struct IDCardNo1{
	int Num;        //����
	unsigned char SN[IDCARDMAXNUM*4]; //ID����  ���10����
	uint32_t serialnum;     //���
};
//дID����Ϣ
struct RecvIDCardNo1{
	int isNewWriteFlag;  //��д��־
	int Num;             //������
	int PackNum;         //������
	int PackIsRecved[IDCARDMAXNUM/CARDPERPACK + 1];  //���ѽ��ձ�־
	unsigned char SN[IDCARDMAXNUM*4]; //ID����  ���10����
	uint32_t serialnum;     //���
};
//ˢID����Ϣ
struct BrushIDCard1{
	int Num;        //����
	unsigned char Info[IDCARDBRUSHNUM*11]; //ID����  ������Ϣ���1����
	//ǰ4�ֽڿ��ţ���7�ֽ�ʱ��
};
//���ڽ��ջ�����
struct commbuf1
{
	int iput; // ���λ������ĵ�ǰ����λ��
	int iget; // �������ĵ�ǰȡ��λ��
	int n; // ���λ������е�Ԫ��������
	unsigned char buffer[COMMMAX];
};
//״̬��ʾ��Ϣ����
//Type
//          11 -- ��������
//          12 -- ����ͨ��
//          13 -- ����
//          16 -- �Խ�ͼ�񴰿�
struct PicStatBuf1{
	int Flag;                      //��ʱ����ʾ��Ϣ��־
	int Type;                      //����
	int Time;
	int MaxTime;                   //�ʱ��

	int KeyPressTime;
};
struct Remote1{
	int DenNum;             //Ŀ������  ����+����
	unsigned char DenIP[4]; //�Է�IP����Ƶ������IP
	unsigned char GroupIP[4]; //GroupIP
	unsigned char IP[10][4];    //�Է�IP
	int Added[10];                //�Ѽ�����
	char Addr[10][21];         //�Է�Addr
	int isDirect;       //�Ƿ�ֱͨ  0 ֱͨ  1 ��ת
};

//�洢�ļ���FLASH���� ���ݽṹ ���ڴ洢FLASH�ٶȽ��� ���߳�������
struct Save_File_Buff1{
	int isValid; //�Ƿ���Ч
	int Type;    //�洢����   4����������
	int InfoType;   //��Ϣ����
	int InfoNo;     //��Ϣλ��
	char cFromIP[15];   //���ڶ�ID��Ӧ��
};

//UDP�����������ݷ��ͽṹ
struct Multi_Udp_Buff1{
	int isValid; //�Ƿ���Ч
	int SendNum; //��ǰ���ʹ���
	int CurrOrder;//��ǰ����״̬,VIDEOTALK VIDEOTALKTRANS VIDEOWATCH VIDEOWATCHTRANS
	//��Ҫ���������ʱ���絥��������Ϊ0
	int m_Socket;
	char RemoteHost[20];
	unsigned char buf[1500];
	int DelayTime;  //�ȴ�ʱ��
	int nlength;
};
//COMM�����������ݷ��ͽṹ
struct Multi_Comm_Buff1{
	int isValid; //�Ƿ���Ч
	int SendNum; //��ǰ���ʹ���
	int m_Comm;
	unsigned char buf[1500];
	int nlength;
};
//ͨ�����ݽṹ
struct talkdata1
{
	char HostAddr[20];       //���з���ַ
	unsigned char HostIP[4]; //���з�IP��ַ
	char AssiAddr[20];       //���з���ַ
	unsigned char AssiIP[4]; //���з�IP��ַ
	unsigned int timestamp;  //ʱ���
	unsigned short DataType;          //��������
	unsigned short Frameno;           //֡���
	unsigned int Framelen;            //֡���ݳ���
	unsigned short TotalPackage;      //�ܰ���
	unsigned short CurrPackage;       //��ǰ����
	unsigned short Datalen;           //���ݳ���
	unsigned short PackLen;       //���ݰ���С
}__attribute__ ((packed));
//��дID�����ݽṹ
struct iddata1
{
	char Addr[20];       //��ַ����
	unsigned int serialnum;  //���
	unsigned int  Num;          //������
	unsigned int  CurrNum;           //��ǰ���ݰ�������
	unsigned int  TotalPackage;      //�ܰ���
	unsigned int  CurrPackage;       //��ǰ����
}__attribute__ ((packed));
//��Ϣ���ݽṹ
struct infodata1
{
	char Addr[20];       //��ַ����
	unsigned short Type; //����
	unsigned int  Sn;         //���
	unsigned short Length;   //���ݳ���
}__attribute__ ((packed));
///////////////////////////////////////zhou101102
struct SendTalkInfo1
{
	int Status;//״̬
	int Duration;//ͨ��ʱ��
	char Addr[21];//�Է���ַ
	char StartTime[16];//��ʼ����ʱ��
};
#ifndef CommonH
#define CommonH
struct TalkDen1 TalkDen; 
struct TimeStamp1 TimeStamp;  //����ʱ���벥��ʱ�䣬ͬ����

//////////////////////////
int NeedSendPhone;//�Ƿ��������͸��绰ģ��//zhou101014
//////////////////////////zhou101102
struct SendTalkInfo1 SendTalkInfo;
int SendTalkInfoFlag;
int NeedOpenLock; //0Ϊ�գ����ϴ� 1Ϊ�ϴ�//zhou101102 ���������ϴ�����Ϣ ����OpenLock_FuncΪUDP�����߳�
int DeltaLen;  //���ݰ���Ч����ƫ����

struct tm *curr_tm_t;

//ϵͳ��ʼ����־
int InitSuccFlag;
//����״̬����
struct Local1 Local;
struct LocalCfg1 LocalCfg;
int TurnToCenter;
int Cif_Bit_rate[6] = {128, 256, 384, 512, 768, 1024};
//ID����Ϣ
struct IDCardNo1 IDCardNo;
//ˢID����Ϣ
struct BrushIDCard1 BrushIDCard;
//дID����Ϣ
struct RecvIDCardNo1 RecvIDCardNo;

//Զ�˵�ַ
struct Remote1 Remote;
char NullAddr[21];   //���ַ���
//���ARP
int ARP_Socket;
//�����������
int m_EthSocket;
//UDP
int m_DataSocket;
int m_VideoSocket;
int LocalDataPort;   //�������UDP�˿�
int LocalVideoPort;  //����ƵUDP�˿�
int RemoteDataPort;
int RemoteVideoPort;
int m_VideoSendSocket;
char RemoteHost[20];
char sPath[80];
char wavPath[80];
char UdpPackageHead[15];
//״̬��ʾ��Ϣ����
struct PicStatBuf1 PicStatBuf;

//�����������ݷ����̣߳��ն����������������ʱһ��û�յ���Ӧ�����η���
//����UDP��Commͨ��
int multi_send_flag;
pthread_t multi_send_thread;
void multi_send_thread_func(void);
sem_t multi_send_sem;
struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10��UDP�������ͻ���

//watchdog
int watchdog_fd;

int OpenLockTime[4];       //����ʱ��
char OpenLockTimeText[4][20];  //�ı�
int DelayLockTime[4];       //��ʱ����ʱ��
char DelayLockTimeText[4][20];       //�ı�
#else
//////////////////////////
extern int NeedSendPhone;//�Ƿ��������͸��绰ģ��//zhou101014
//////////////////////////zhou101102
extern struct SendTalkInfo1 SendTalkInfo;
extern int SendTalkInfoFlag;
extern int NeedOpenLock; //0Ϊ�գ����ϴ� 1Ϊ�ϴ�//zhou101102 ���������ϴ�����Ϣ ����OpenLock_FuncΪUDP�����߳�
extern struct TalkDen1 TalkDen;
extern struct TimeStamp1 TimeStamp;  //����ʱ���벥��ʱ�䣬ͬ����

extern int DeltaLen;  //���ݰ���Ч����ƫ����

extern struct tm *curr_tm_t;

//ϵͳ��ʼ����־
extern int InitSuccFlag;
//����״̬����
extern struct Local1 Local;
extern struct LocalCfg1 LocalCfg;
extern int TurnToCenter;
extern int Cif_Bit_rate[6];
//ID����Ϣ
extern struct IDCardNo1 IDCardNo;
//ˢID����Ϣ
extern struct BrushIDCard1 BrushIDCard;  
//дID����Ϣ
extern struct RecvIDCardNo1 RecvIDCardNo;
//Զ�˵�ַ
extern struct Remote1 Remote;
extern char NullAddr[21];   //���ַ���
//���ARP
extern int ARP_Socket;
//�����������
extern int m_EthSocket;   
//UDP
extern int m_DataSocket;
extern int m_VideoSocket;
extern int LocalDataPort;   //�������UDP��?
extern int LocalVideoPort;  //����ƵUDP�˿�
extern int RemoteDataPort;
extern int RemoteVideoPort;
extern int m_VideoSendSocket;
extern char RemoteHost[20];
extern char sPath[80];
extern char wavPath[80];
extern char UdpPackageHead[15];
//״̬��ʾ��Ϣ����
extern struct PicStatBuf1 PicStatBuf;
//�����������ݷ����̣߳��ն����������������ʱһ��û�յ���Ӧ�����η���
//����UDP��Commͨ��
extern int multi_send_flag;
extern pthread_t multi_send_thread;
extern void multi_send_thread_func(void);
extern sem_t multi_send_sem;
extern struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10��UDP�������ͻ���

//watchdog
extern int watchdog_fd;  

extern int OpenLockTime[4];       //����ʱ��
extern char OpenLockTimeText[4][20];  //�ı�
extern int DelayLockTime[4];       //��ʱ����ʱ��
extern char DelayLockTimeText[4][20];       //�ı�
#endif
