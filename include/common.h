#include <inttypes.h>
#include <signal.h>
#include <semaphore.h>       //sem_t
#include <sys/stat.h>
#include <pthread.h>
#include "sndtools.h"

#define LIFT 110

#define _DEBUG           //����ģʽ

//#define _TESTNSSERVER        //���Է���������ģʽ
//#define _TESTTRANS           //������Ƶ��תģʽ

#define SOFTWAREVER "1.00.00"    

#define NSMULTIADDR  "238.9.9.1"//"192.168.10.255"  //NS�鲥��ַ
#define LFTMULTIADDR  "238.9.9.2"//"192.168.10.255"  //NS�鲥��ַ

#define ZOOMMAXTIME 2000   //�Ŵ���С�ӳٴ���ʱ��
#define TOUCHMAXTIME 300   //�����������ӳٴ���ʱ��

#define INTRTIME 50       //�߳�50ms
#define INTRPERSEC 20       //ÿ��20���߳�
#define BUFFER_SIZE 1024
#define FRAMEBUFFERMAX  4
#define COMMMAX 1024     //���ڻ��������ֵ

#define INFOROWLEN   32    //��Ϣÿ�г���
#define MAXROW  12          //�������
#define PAGEPERROW  3          //ҳ����
//#define PAGEPERROW  4          //ҳ����
//
#define WATCHTIMEOUT  30*(1000/INTRTIME)    //�����ʱ��
#define CALLTIMEOUT  25*(1000/INTRTIME)     //�����ʱ��
#define TALKTIMEOUT  130*(1000/INTRTIME)//30*20     //ͨ���ʱ��
#define PREPARETIMEOUT  10*(1000/INTRTIME)     //��Ӱ����Ԥ���ʱ��
#define RECORDTIMEOUT  30*(1000/INTRTIME)     //��Ӱ�����ʱ��

#define FORBIDTIMEOUT (1000/INTRTIME)
//���� ��������
#define ALARM         1    //����
#define CANCELALARM   2    //ȡ������
#define SENDMESSAGE   3   //������Ϣ
#define REPORTSTATUS  4   //�豸��ʱ����״̬
#define QUERYSTATUS   5   //�������Ĳ�ѯ�豸״̬
#define ALARMHANDLE   111
#define CHECKTIME     7
///////////paul0415
#define SENDPICINFO   6
#define PICINFONUM    8
//////////paul0812////////////////////////////
#define CHECKMAC	161
#define SETPARAM 	162
#define MAKEOLD		163
#define READMAC		164
#define MCUUPDATE	165
////////////////////
#define REMOTEDEFENCE   20   //Զ�̲���
#define RESETPASS       30   //��λ����
#define WRITEADDRESS   40   //д��ַ����
#define READADDRESS    41   //����ַ����
#define WRITEROOMSETUP     44   //д���ڻ���������
#define READROOMSETUP      45   //�����ڻ���������
#define WRITESETUP     52   //д��Ԫ�ſڻ���Χǽ��������Ϣ
#define READSETUP      53   //����Ԫ�ſڻ���Χǽ��������Ϣ
//�Խ�
#define VIDEOTALK      150 //���������ӶԽ�
#define VIDEOTALKTRANS 151 //���������ӶԽ���ת����
#define VIDEOWATCH     152 //���������
#define VIDEOWATCHTRANS   153 //�����������ת����
#define NSORDER        154 //�����������������ڹ㲥��
#define NSSERVERORDER  155 //����������(NS������)
#define FINDEQUIP      170 //�����豸

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
#define REMOTECALLLIFT   20
#define REMOTELIFT		 30
#define MAINPICNUM  24      //��ҳͼƬ����
#define MAINLABELNUM  2     //��ҳLabel����
#define DOWNLOAD  220      //����
#define ASK  1
#define REPLY  2

#define SAVEMAX  50     //FLASH�洢�������ֵ
#define UDPSENDMAX  50  //UDP��η��ͻ������ֵ
#define COMMSENDMAX  10  //COMM��η��ͻ������ֵ
#define MAXSENDNUM  6  //����ʹ���

//��ťѹ��ʱ��
#define DELAYTIME  200
//��ť����
#define InfoButtonMax  16
//����Ϣ////////////paul
#define INFOTYPENUM  4 //4    //����Ϣ����
#define INFOMAXITEM  50 //200    //����Ϣ�������
#define INFOMAXSIZE  400 //����Ϣ�����������
#define INFONUMPERPAGE 3  //һҳ��ʾ��Ϣ��

#define NMAX 512*64  //AUDIOBLK*64  //��Ƶ���λ�������С
#define G711NUM  64*512/AUDIOBLK       //��Ƶ���ջ��������� δ����   10

//#define VIDEOMAX 720*480
#define VIDEOMAX 720*576
#define VNUM  3         //��Ƶ�ɼ���������С
#define VPLAYNUM  10         //��Ƶ���Ż�������С         6
#define MP4VNUM  20         //��Ƶ���ջ��������� δ����   10
#define PACKDATALEN  1200   //���ݰ���С
#define MAXPACKNUM  100     //֡������ݰ�����

//////////////FOR UPDATE//////////////
struct downfile1
{
   char FlagText[20];     //��־�ַ���
   char FileName[20];
   unsigned int Filelen;            //�ļ���С
   unsigned short TotalPackage;      //�ܰ���
   unsigned short CurrPackage;       //��ǰ����
   unsigned short Datalen;           //���ݳ���
}__attribute__ ((packed));
//////////////////////////////////////

struct TimeStamp1
{
    unsigned int OldCurrVideo;     //��һ�ε�ǰ��Ƶʱ��
    unsigned int CurrVideo;
    unsigned int OldCurrAudio;     //��һ�ε�ǰ��Ƶʱ��
    unsigned int CurrAudio;
};
//��Ƶ�ɼ�����
struct videobuf1
{
    int iput; // ���λ������ĵ�ǰ����λ��
    int iget; // �������ĵ�ǰȡ��λ��
    int n; // ���λ������е�Ԫ��������
    uint32_t timestamp[VNUM]; //ʱ���
    uint32_t frameno[VNUM];   //֡���
    unsigned char *buffer_y[VNUM];//[VIDEOMAX];
    unsigned char *buffer_u[VNUM];//[VIDEOMAX/4];
    unsigned char *buffer_v[VNUM];//[VIDEOMAX/4];
};
//��Ƶ���ջ���  δ����
struct tempvideobuf1
{
//  int iput;                     // ���λ������ĵ�ǰ����λ��
//  int iget;                     // �������ĵ�ǰȡ��λ��
//  int n;                        // ���λ������е�Ԫ��������
    uint32_t timestamp;  //ʱ���
    uint32_t frameno;       //֡���
    short TotalPackage;     //�ܰ���
    uint8_t CurrPackage[MAXPACKNUM]; //��ǰ��   1 �ѽ���  0 δ����
    int Len;                //֡���ݳ���
    uint8_t isFull;                  //��֡�ѽ�����ȫ
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //֡��־ ��Ƶ֡ I֡ P֡
};                            //     [MP4VNUM]
//////////////paul0416

typedef struct temppicinfobuf1
{
    uint32_t frameno;       //֡���
    short TotalPackage;     //�ܰ���
    uint8_t CurrPackage[MAXPACKNUM]; //��ǰ��   1 �ѽ���  0 δ����
    int Len;                //֡���ݳ���
    uint8_t isFull;                  //��֡�ѽ�����ȫ
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //1 for private 2 for common 3 for alarm 4 for picture
} TempPicInfoBuff1 ;
////////////////////////

//��Ƶ���ջ��� ����
typedef struct node2
{
    struct tempvideobuf1 Content;
    struct node2 *llink, *rlink;
}TempVideoNode1;
//��Ƶ���Ż���
struct videoplaybuf1
{
    uint8_t isUse;     //��֡�ѽ���δ����,������������
    uint32_t timestamp; //ʱ���
    uint32_t frameno;   //֡���
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //֡��־ ��Ƶ֡ I֡ P֡
};
//ͬ�����Žṹ
struct _SYNC
{
    pthread_cond_t cond;       //ͬ���߳���������
    pthread_condattr_t cond_attr;
    pthread_mutex_t lock;      //������
    pthread_mutex_t audio_rec_lock;//[VPLAYNUM];//��Ƶ¼�ƻ�����
    pthread_mutex_t audio_play_lock;//[VPLAYNUM];//��Ƶ���Ż�����
    pthread_mutex_t video_rec_lock;//[VPLAYNUM];//��Ƶ¼�ƻ�����
    pthread_mutex_t video_play_lock;//[VPLAYNUM];//��Ƶ���Ż�����
    pthread_mutex_t lmovie_lock;////added from door///paul0326//////
    unsigned int count;        //����
    uint8_t isDecodeVideo;     //��Ƶ�ѽ���һ֡  �����߳�-->ͬ���߳�
    uint8_t isPlayVideo;       //��Ƶ�Ѳ���һ֡  �����߳�-->ͬ���߳�
    uint8_t isDecodeAudio;     //��Ƶ�ѽ���һ֡  �����߳�-->ͬ���߳�
    uint8_t isPlayAudio;       //��Ƶ�Ѳ���һ֡  �����߳�-->ͬ���߳�
};

//�ӻ�����?
struct audiobuf1
{
    int iput; // ���λ������ĵ�ǰ����λ��
    int iget; // �������ĵ�ǰȡ��λ��
    int n; // ���λ������е�Ԫ��������
    uint32_t timestamp[NMAX/AUDIOBLK]; //ʱ���
    uint32_t frameno[NMAX/AUDIOBLK];   //֡���
    unsigned char buffer[NMAX];
};

//��Ƶ���ջ���  δ����
struct tempaudiobuf1
{
    uint32_t timestamp;  //ʱ���
    uint32_t frameno;       //֡���
    short TotalPackage;     //�ܰ���
    uint8_t CurrPackage[MAXPACKNUM]; //��ǰ��   1 �ѽ���  0 δ����
    int Len;                //֡���ݳ���
    uint8_t isFull;                  //��֡�ѽ�����ȫ
    unsigned char *buffer;//[AUDIOBLK];
    unsigned char frame_flag;             //֡��־ ��Ƶ֡ I֡ P֡
};                            //     [MP4VNUM]
//��Ƶ���ջ��� ����
typedef struct node3
{
    struct tempaudiobuf1 Content;
    struct node3 *llink, *rlink;
}TempAudioNode1;

//��Ƶ���Ż���
struct audioplaybuf1
{
    uint8_t isUse;     //��֡�ѽ���δ����,������������
    uint32_t timestamp; //ʱ���
    uint32_t frameno;   //֡���
    unsigned char *buffer;//[VIDEOMAX];
};

//��ͥ���Ի���
struct wavbuf1
{
    int iput; // ���λ������ĵ�ǰ����λ��
    int iget; // �������ĵ�ǰȡ��λ��
    int n; // ���λ������е�Ԫ��������
    unsigned char *buffer;
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

////////////copy from sound/////paul0326//////////////
/*struct flcd_data
{
    unsigned int buf_len;
    unsigned int uv_offset;
    unsigned int frame_no;
    unsigned int mp4_map_dma[VIDEO_PICTURE_QUEUE_SIZE_MAX];
};*/

struct Local1 {
    int Status;
    //״̬ 0 ���� 1 ���жԽ�  2 ���жԽ�  3 ����  4 ������  5 ����ͨ��
    //6 ����ͨ��
	int RecordPic;  //����Ƭ  0 ����  1 ��������Ƭ  2 ͨ������Ƭ
    unsigned char IP_Group[4];  //�鲥��ַ

    int CallConfirmFlag; //���߱�־
    int Timer1Num;  //��ʱ��1����
    int OnlineFlag; //��������ȷ��
    int OnlineNum;  //����ȷ�����
    int TimeOut;    //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
    int TalkTimeOut; //ͨ���ʱ��

    pthread_mutex_t udp_lock;//������
};

/*
struct Local1
{
    int Status;
    //״̬ 0 ���� 1 ���жԽ�  2 ���жԽ�  3 ����  4 ������  5 ����ͨ��
    //6 ����ͨ��
    int KillStatus;
	int RecordPic;  //����Ƭ  0 ����  1 ��������Ƭ  2 ͨ������Ƭ
    int IFrameCount; //I֡����
    int IFrameNo;    //���ڼ���I֡
    unsigned char yuv[2][D1_W*D1_H*3/2];
    /////unsigned char yuv[2][CIF_W*CIF_H*3/2];
    int HavePicRecorded;  //����Ƭ��¼��
    struct tm *recpic_tm_t; //����Ƭʱ��

///////////////////////paul////////////////////////////////////////
    int again;
	char Center[2];
	char udporder;//the current order from udp
    char commorder;
    char FromIP[20];
    unsigned char udpsend_buf[1520];
    unsigned char commsend_buf[100];
    char Addr[20];
	unsigned char infoAddr[20];
	unsigned char picAddr[4];//��Ӱ
	int alarmAddr[2];
	unsigned char OldAddr[20];
	char takepic;
	unsigned char VideoAddr[20];
	unsigned char AudioAddr[20];
	unsigned char KillRemote[20];
	unsigned char KillAddr[20];
	unsigned char KillIP[4];
	char interrupted;
    char updatemcu; 
	int Floor;
	int Room;

//	char MaxFloor;
//	char MaxRoom;
/////////////////////////////////////////////////////////////
    ///////////////0326//////////from door/////////////
    int DoorNo;
    char ClockText[20];
    int InputMax;
    int OpenDoorFlag;
    int OpenDoorTime;

    ////////////
    struct tm *call_tm_t; //������ʱ��

    int CallConfirmFlag; //���߱�־
    int Timer1Num;  //��ʱ��1����
    int OnlineFlag; //��������ȷ��
    int OnlineNum;  //����ȷ�����
    int TimeOut;    //���ӳ�ʱ,  ͨ����ʱ,  ���г�ʱ�����˽���
    int TalkTimeOut; //ͨ���ʱ��
    int RecPicSize;  //��Ƶ��С  1  352*288   2  720*480
    int PlayPicSize;  //��Ƶ��С  1  352*288   2  720*480
    ///////////mj
	int ForbTimeOut;
	pthread_mutex_t save_lock;//������
    pthread_mutex_t udp_lock;//������
    pthread_mutex_t comm_lock;//������
    int PrevWindow;      //��һ�����ڱ��
    int TmpWindow;       //�ݴ洰�ڱ�� ���ڵ�������ʱ
    int CurrentWindow;   //��ǰ���ڱ��
    int DefenceDelayFlag;    //������ʱ��־
    int DefenceDelayTime;   //����
    int PassLen;            //���볤��
    int AlarmDelayFlag[2];    //������ʱ��־
    int AlarmDelayTime[2];   //����

    int ForceIFrame;    //1 ǿ��I֡
    int CalibratePos;   //У׼������ʮ��λ�� 0 1 2 3
    int CalibrateSucc;  //У׼�ɹ�
    int CurrFbPage; //��ǰFbҳ
    unsigned char IP_Group[4];  //�鲥��ַ
    unsigned char Weather[3];   //����Ԥ��

    int AddrLen;          //��ַ����  S 12  B 6 M 8 H 6

    int isHost;           //'0' ���� '1' ���� '2' ...
    int ConnToHost;       //�������������� 1 ���� 0 ������
    unsigned char HostIP[4]; //����IP
    unsigned char HostAddr[21]; //����Addr
    int DenNum;             //Ŀ������  ����
    unsigned char DenIP[10][4];    //����IP
    char DenAddr[10][21];         //����Addr

    int NetStatus;   //����״̬ 1 �Ͽ�  0 ��ͨ
    int OldNetSpeed;  //�����ٶ�
    int NoBreak;     //����״̬ 1 ����  0 ����

    int ReportSend;  //�豸��ʱ����״̬�ѷ���
    int RandReportTime; //�豸��ʱ����״̬���ʱ��
    int ReportTimeNum;  //��ʱ
    //��GPIO�߳��в�ѯ���߳��Ƿ�����
    int Key_Press_Run_Flag;
    int Save_File_Run_Flag;
    int Dispart_Send_Run_Flag;
    int Multi_Send_Run_Flag;
    int Multi_Comm_Send_Run_Flag;

    int MenuIndex;     //��ǰ��ť����
    int MaxIndex;      //�������������
    int MainMenuIndex;     //�����水ť����

    int OsdOpened;  //OSD�򿪱�־

    int LcdLightFlag; //LCD�����־
    int LcdLightTime; //ʱ��

	//int NewINfo;
    int NewInfo[FLOOR][ROOM];  //������Ϣ

    int ResetPlayRingFlag;  //��λAudio Play flag

    int nowvideoframeno;   //��ǰ��Ƶ֡���
    int nowaudioframeno;   //��ǰ��Ƶ֡���

    int ForceEndWatch;  //�к���ʱ��ǿ�ƹؼ���
    int ZoomInOutFlag;  //���ڷŴ���С��
    uint32_t newzoomtime;
    uint32_t oldzoomtime;
    uint32_t newtouchtime;
    uint32_t oldtouchtime;    //��һ�δ���������ʱ��
};
*/

struct LocalCfg1
{
    char Addr[20];             //��ַ����
    unsigned char BuildAddr[2];
	unsigned char Mac_Addr[6]; //������ַ
    unsigned char IP[4];       //IP��ַ
    unsigned char IP_Mask[4];  //��������
    unsigned char IP_Gate[4];  //���ص�ַ
    unsigned char IP_NS[4];    //NS�����ƽ�������������ַ
    unsigned char IP_Server[4];  //����������ַ����NS��������Ϊͬһ����
    unsigned char IP_Broadcast[4];  //�㲥��ַ

    int ReportTime;      //�豸��ʱ����״̬ʱ��
    unsigned char DefenceStatus;       //����״̬
    unsigned char DefenceNum;          //����ģ�����
    unsigned char DefenceInfo[32][10]; //������Ϣ

    char EngineerPass[10];             //��������
    char OpenLockPass[10];

    int In_DelayTime;                //������ʱ
    int Out_DelayTime;               //�����ʱ
    int Alarm_DelayTime;               //������ʱ

/////////////////////////0326////////////////from door/////////
    unsigned char OpenLockTime;
    unsigned char DelayLockTime;
    unsigned char PassOpenLock;
    unsigned char CardOpenLock;

    unsigned char bit_rate;
    ///////////////////////////////////////////////////
    int Ts_X0;                   //������
    int Ts_Y0;
    int Ts_deltaX;
    int Ts_deltaY;
};

struct Remote1
{
    int DenNum;             //Ŀ������  ����+����
    unsigned char DenIP[4]; //�Է�IP����Ƶ������IP
    unsigned char GroupIP[4]; //GroupIP
    unsigned char IP[10][4];    //�Է�IP
    int Added[10];                //�Ѽ�����
    char Addr[10][21];         //�Է�Addr
    int isDirect;       //�Ƿ�ֱͨ  0 ֱͨ  1 ��ת
};

struct Info1
{
    int MaxNum;   //�����Ϣ��
    int TotalNum; //��Ϣ����
    int NoReadedNum; //δ����Ϣ����
    int TotalInfoPage;   //����Ϣҳ��
    int CurrentInfoPage; //��ǰ��Ϣҳ
    int CurrNo;    //��ǰ��Ϣ���
    int CurrPlayNo;  //��ǰ�������
    int TimeNum;    //����
};

//������Ϣ���ݽṹ��
struct InfoContent1
{
    uint8_t isValid;  //��Ч��δɾ����־   1
    uint8_t isReaded; //�Ѷ���־    1
    uint8_t isLocked; //������־    1
    char Time[32];    //����ʱ��    32
    uint8_t Type;     //����        1    ��Ϣ���ͻ��¼�����
    uint32_t Sn;      //���        4
    int Length;       //����        4
    char Content[INFOMAXSIZE];//����  400  ���ݻ��¼�����
    //char Event[20];         //�¼�
    char FileName[50];
};                               //�ڴ����Ϊ444

//////////////////paul0509
struct AlarmInfoContent1
{
    uint8_t isValid;  //��Ч��δɾ����־   1
    uint8_t isReaded; //�Ѷ���־    1
    uint8_t isLocked; //������־    1
    char Time[32];    //����ʱ��    32
    uint8_t Type;     //����        1    ��Ϣ���ͻ��¼�����
//    uint32_t Sn;      //���        4
//    int Length;       //����        4
//    char Content[INFOMAXSIZE];//����  400  ���ݻ��¼�����
    //char Event[20];         //�¼�
//    char FileName[50];
//	char AlarmInfo[20]; ///paul0509������Ϣ
}; 
////////////////////////////////////////////
//��ǰ��Ϣ����״̬
struct InfoStatus1
{
    int CurrType;  //��ǰ��Ϣ����
    int CurrWin;   //��ǰ��Ϣ����  0 ��Ϣ�б�  1  ��Ϣ����
    int CurrNo;    //��ǰ��Ϣ���
};
//��Ϣ����
typedef struct node
{
    struct InfoContent1 Content;
    struct node *llink, *rlink;
}InfoNode1;
///////////////paul0509
typedef struct alarmnode
{
	struct AlarmInfoContent1 Content;
	struct alarmnode *llink,*rlink;
}AlarmInfoNode1;
////////////////////////////////////////////////////////////////
//�洢�ļ���FLASH���� ���ݽṹ ���ڴ洢FLASH�ٶȽ��� ���߳�������
struct Save_File_Buff1
{
    int isValid; //�Ƿ���Ч
    int Type;    //�洢���� 1��һ����Ϣ  2��������Ϣ  3����ҵ����  4����������
    int InfoType;   //��Ϣ����
    int InfoNo;     //��Ϣλ��
    InfoNode1 *Info_Node; //��Ϣ���
};

//UDP�����������ݷ��ͽṹ
struct Multi_Udp_Buff1
{
    int isValid; //�Ƿ���Ч
    int SendNum; //��ǰ���ʹ���
    int CurrOrder;//��ǰ����״̬,VIDEOTALK VIDEOTALKTRANS VIDEOWATCH VIDEOWATCHTRANS
    //��Ҫ���������ʱ���絥��������Ϊ0
    int m_Socket;
    char RemoteHost[20];
    unsigned char buf[1500];
    int DelayTime;  //�ȴ�ʱ��
    int SendDelayTime;
    int nlength;
};

//COMM�����������ݷ��ͽṹ
struct Multi_Comm_Buff1
{
    int isValid; //�Ƿ���Ч
    int SendNum; //��ǰ���ʹ���
    int m_Comm;
    unsigned char buf[1500];
    int nlength;
//	int udporder;
//	int commorder;
};

//ͨ�����ݽṹ
struct talkdata1
{
    char HostAddr[20];       //���з��ַ
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
//��Ϣ���ݽṹ
struct infodata1
{
    char Addr[20];       //��ַ����
    unsigned short Type; //����
//  	unsigned char Type;
	unsigned int  Sn;         //���
    unsigned short Length;   //���ݳ���
}__attribute__ ((packed));

#ifndef CommonH
#define CommonH
int DebugMode;           //����ģʽ
int DeltaLen;  //���ݰ���Ч����ƫ����
struct tm *curr_tm_t;
struct TimeStamp1 TimeStamp;  //����ʱ���벥��ʱ�䣬ͬ����
int temp_video_n;      //��Ƶ���ջ������
TempVideoNode1 *TempVideoNode_h;    //��Ƶ���ջ����б�
int temp_audio_n;      //��Ƶ���ջ������
TempAudioNode1 *TempAudioNode_h;    //��Ƶ���ջ����б�

//ϵͳ��ʼ����־
int InitSuccFlag;
//����״̬����
struct Local1 Local;
struct LocalCfg1 LocalCfg;

//Զ�˵�ַ
struct Remote1 Remote;
char null_addr[21];   //���ַ���
char null_ip[4];   //���ַ���
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
char RemoteHost[20];
char sPath[80];
char currpath[80];   //�Զ���·��
char wavPath[80];
char UdpPackageHead[15];

//�����������ݷ����̣߳��ն����������������ʱһ��û�յ���Ӧ�����η���
//����UDP��Commͨ��
int multi_send_flag;
pthread_t multi_send_thread;
void multi_send_thread_func(void);
sem_t multi_send_sem;
struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10��UDP�������ͻ���

#else
extern int DebugMode;           //����ģʽ
extern int DeltaLen;  //���ݰ���Ч����ƫ����
extern struct tm *curr_tm_t;
extern struct TimeStamp1 TimeStamp;  //����ʱ���벥��ʱ�䣬ͬ����
extern int temp_video_n;      //��Ƶ���ջ������
extern TempVideoNode1 *TempVideoNode_h;    //��Ƶ���ջ����б�
extern int temp_audio_n;      //��Ƶ���ջ������
extern TempAudioNode1 *TempAudioNode_h;    //��Ƶ���ջ����б�

//ϵͳ��ʼ����־
extern int InitSuccFlag;
//����״̬����
extern struct Local1 Local;
extern struct LocalCfg1 LocalCfg;

//Զ�˵�ַ
extern struct Remote1 Remote;
extern char null_addr[21];   //���ַ���
extern char null_ip[4];   //���ַ���
//���ARP
extern int ARP_Socket;
//�����������
extern int m_EthSocket;
//UDP
extern int m_DataSocket;
extern int m_VideoSocket;

extern int LocalDataPort;   //�������UDP�˿�
extern int LocalVideoPort;  //����ƵUDP�˿�

extern int RemoteDataPort;
extern int RemoteVideoPort;
extern char RemoteHost[20];
extern char sPath[80];
extern char currpath[80];   //�Զ���·��
extern char wavPath[80];
extern char UdpPackageHead[15];

//�����������ݷ����̣߳��ն����������������ʱһ��û�յ���Ӧ�����η���
//����UDPͨ��
extern int multi_send_flag;
extern pthread_t multi_send_thread;
extern void multi_send_thread_func(void);
extern sem_t multi_send_sem;
extern struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10��UDP�������ͻ���
#endif
