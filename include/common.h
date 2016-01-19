#include <inttypes.h>
#include <signal.h>
#include <semaphore.h>       //sem_t
#include <sys/stat.h>
#include <pthread.h>
#include "sndtools.h"

#define LIFT 110

#define _DEBUG           //调试模式

//#define _TESTNSSERVER        //测试服务器解析模式
//#define _TESTTRANS           //测试视频中转模式

#define SOFTWAREVER "1.00.00"    

#define NSMULTIADDR  "238.9.9.1"//"192.168.10.255"  //NS组播地址
#define LFTMULTIADDR  "238.9.9.2"//"192.168.10.255"  //NS组播地址

#define ZOOMMAXTIME 2000   //放大缩小延迟处理时间
#define TOUCHMAXTIME 300   //触摸屏处理延迟处理时间

#define INTRTIME 50       //线程50ms
#define INTRPERSEC 20       //每秒20次线程
#define BUFFER_SIZE 1024
#define FRAMEBUFFERMAX  4
#define COMMMAX 1024     //串口缓冲区最大值

#define INFOROWLEN   32    //信息每行长度
#define MAXROW  12          //最大行数
#define PAGEPERROW  3          //页行数
//#define PAGEPERROW  4          //页行数
//
#define WATCHTIMEOUT  30*(1000/INTRTIME)    //监视最长时间
#define CALLTIMEOUT  25*(1000/INTRTIME)     //呼叫最长时间
#define TALKTIMEOUT  130*(1000/INTRTIME)//30*20     //通话最长时间
#define PREPARETIMEOUT  10*(1000/INTRTIME)     //留影留言预备最长时间
#define RECORDTIMEOUT  30*(1000/INTRTIME)     //留影留言最长时间

#define FORBIDTIMEOUT (1000/INTRTIME)
//命令 管理中心
#define ALARM         1    //报警
#define CANCELALARM   2    //取消报警
#define SENDMESSAGE   3   //发送信息
#define REPORTSTATUS  4   //设备定时报告状态
#define QUERYSTATUS   5   //管理中心查询设备状态
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
#define REMOTEDEFENCE   20   //远程布防
#define RESETPASS       30   //复位密码
#define WRITEADDRESS   40   //写地址设置
#define READADDRESS    41   //读地址设置
#define WRITEROOMSETUP     44   //写室内机工程设置
#define READROOMSETUP      45   //读室内机工程设置
#define WRITESETUP     52   //写单元门口机、围墙机设置信息
#define READSETUP      53   //读单元门口机、围墙机设置信息
//对讲
#define VIDEOTALK      150 //局域网可视对讲
#define VIDEOTALKTRANS 151 //局域网可视对讲中转服务
#define VIDEOWATCH     152 //局域网监控
#define VIDEOWATCHTRANS   153 //局域网监控中转服务
#define NSORDER        154 //主机名解析（子网内广播）
#define NSSERVERORDER  155 //主机名解析(NS服务器)
#define FINDEQUIP      170 //查找设备

#define ASK              1     //命令类型 主叫
#define REPLY            2     //命令类型 应答

#define CALL             1     //呼叫
#define LINEUSE          2     //占线
#define QUERYFAIL        3      //通信失败
#define CALLANSWER       4     //呼叫应答
#define CALLSTART        6     //开始通话

#define CALLUP           7     //通话数据1（主叫方->被叫方）
#define CALLDOWN         8     //通话数据2（被叫方->主叫方）
#define CALLCONFIRM      9     //通话在线确认（接收方发送，以便发送方确认在线）
#define REMOTEOPENLOCK   10     //远程开锁

#define FORCEIFRAME      11     //强制I帧请求
#define ZOOMOUT          15     //放大(720*480)
#define ZOOMIN           16     //缩小(352*288)
#define OPENLOCK         17
#define CALLEND          30     //通话结束
#define REMOTECALLLIFT   20
#define REMOTELIFT		 30
#define MAINPICNUM  24      //首页图片数量
#define MAINLABELNUM  2     //首页Label数量
#define DOWNLOAD  220      //下载
#define ASK  1
#define REPLY  2

#define SAVEMAX  50     //FLASH存储缓冲最大值
#define UDPSENDMAX  50  //UDP多次发送缓冲最大值
#define COMMSENDMAX  10  //COMM多次发送缓冲最大值
#define MAXSENDNUM  6  //最大发送次数

//按钮压下时间
#define DELAYTIME  200
//按钮数量
#define InfoButtonMax  16
//短信息////////////paul
#define INFOTYPENUM  4 //4    //短信息类型
#define INFOMAXITEM  50 //200    //短信息最大条数
#define INFOMAXSIZE  400 //短信息内容最大容量
#define INFONUMPERPAGE 3  //一页显示信息数

#define NMAX 512*64  //AUDIOBLK*64  //音频环形缓冲区大小
#define G711NUM  64*512/AUDIOBLK       //音频接收缓冲区个数 未解码   10

//#define VIDEOMAX 720*480
#define VIDEOMAX 720*576
#define VNUM  3         //视频采集缓冲区大小
#define VPLAYNUM  10         //视频播放缓冲区大小         6
#define MP4VNUM  20         //视频接收缓冲区个数 未解码   10
#define PACKDATALEN  1200   //数据包大小
#define MAXPACKNUM  100     //帧最大数据包数量

//////////////FOR UPDATE//////////////
struct downfile1
{
   char FlagText[20];     //标志字符串
   char FileName[20];
   unsigned int Filelen;            //文件大小
   unsigned short TotalPackage;      //总包数
   unsigned short CurrPackage;       //当前包数
   unsigned short Datalen;           //数据长度
}__attribute__ ((packed));
//////////////////////////////////////

struct TimeStamp1
{
    unsigned int OldCurrVideo;     //上一次当前视频时间
    unsigned int CurrVideo;
    unsigned int OldCurrAudio;     //上一次当前音频时间
    unsigned int CurrAudio;
};
//视频采集缓冲
struct videobuf1
{
    int iput; // 环形缓冲区的当前放入位置
    int iget; // 缓冲区的当前取出位置
    int n; // 环形缓冲区中的元素总数量
    uint32_t timestamp[VNUM]; //时间戳
    uint32_t frameno[VNUM];   //帧序号
    unsigned char *buffer_y[VNUM];//[VIDEOMAX];
    unsigned char *buffer_u[VNUM];//[VIDEOMAX/4];
    unsigned char *buffer_v[VNUM];//[VIDEOMAX/4];
};
//视频接收缓冲  未解码
struct tempvideobuf1
{
//  int iput;                     // 环形缓冲区的当前放入位置
//  int iget;                     // 缓冲区的当前取出位置
//  int n;                        // 环形缓冲区中的元素总数量
    uint32_t timestamp;  //时间戳
    uint32_t frameno;       //帧序号
    short TotalPackage;     //总包数
    uint8_t CurrPackage[MAXPACKNUM]; //当前包   1 已接收  0 未接收
    int Len;                //帧数据长度
    uint8_t isFull;                  //该帧已接收完全
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //帧标志 音频帧 I帧 P帧
};                            //     [MP4VNUM]
//////////////paul0416

typedef struct temppicinfobuf1
{
    uint32_t frameno;       //帧序号
    short TotalPackage;     //总包数
    uint8_t CurrPackage[MAXPACKNUM]; //当前包   1 已接收  0 未接收
    int Len;                //帧数据长度
    uint8_t isFull;                  //该帧已接收完全
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //1 for private 2 for common 3 for alarm 4 for picture
} TempPicInfoBuff1 ;
////////////////////////

//视频接收缓冲 链表
typedef struct node2
{
    struct tempvideobuf1 Content;
    struct node2 *llink, *rlink;
}TempVideoNode1;
//视频播放缓冲
struct videoplaybuf1
{
    uint8_t isUse;     //该帧已解码未播放,缓冲区不可用
    uint32_t timestamp; //时间戳
    uint32_t frameno;   //帧序号
    unsigned char *buffer;//[VIDEOMAX];
    unsigned char frame_flag;             //帧标志 音频帧 I帧 P帧
};
//同步播放结构
struct _SYNC
{
    pthread_cond_t cond;       //同步线程条件变量
    pthread_condattr_t cond_attr;
    pthread_mutex_t lock;      //互斥锁
    pthread_mutex_t audio_rec_lock;//[VPLAYNUM];//音频录制互斥锁
    pthread_mutex_t audio_play_lock;//[VPLAYNUM];//音频播放互斥锁
    pthread_mutex_t video_rec_lock;//[VPLAYNUM];//视频录制互斥锁
    pthread_mutex_t video_play_lock;//[VPLAYNUM];//视频播放互斥锁
    pthread_mutex_t lmovie_lock;////added from door///paul0326//////
    unsigned int count;        //计数
    uint8_t isDecodeVideo;     //视频已解码一帧  解码线程-->同步线程
    uint8_t isPlayVideo;       //视频已播放一帧  播放线程-->同步线程
    uint8_t isDecodeAudio;     //音频已解码一帧  解码线程-->同步线程
    uint8_t isPlayAudio;       //音频已播放一帧  播放线程-->同步线程
};

//加缓冲锁?
struct audiobuf1
{
    int iput; // 环形缓冲区的当前放入位置
    int iget; // 缓冲区的当前取出位置
    int n; // 环形缓冲区中的元素总数量
    uint32_t timestamp[NMAX/AUDIOBLK]; //时间戳
    uint32_t frameno[NMAX/AUDIOBLK];   //帧序号
    unsigned char buffer[NMAX];
};

//音频接收缓冲  未解码
struct tempaudiobuf1
{
    uint32_t timestamp;  //时间戳
    uint32_t frameno;       //帧序号
    short TotalPackage;     //总包数
    uint8_t CurrPackage[MAXPACKNUM]; //当前包   1 已接收  0 未接收
    int Len;                //帧数据长度
    uint8_t isFull;                  //该帧已接收完全
    unsigned char *buffer;//[AUDIOBLK];
    unsigned char frame_flag;             //帧标志 音频帧 I帧 P帧
};                            //     [MP4VNUM]
//音频接收缓冲 链表
typedef struct node3
{
    struct tempaudiobuf1 Content;
    struct node3 *llink, *rlink;
}TempAudioNode1;

//音频播放缓冲
struct audioplaybuf1
{
    uint8_t isUse;     //该帧已解码未播放,缓冲区不可用
    uint32_t timestamp; //时间戳
    uint32_t frameno;   //帧序号
    unsigned char *buffer;//[VIDEOMAX];
};

//家庭留言缓冲
struct wavbuf1
{
    int iput; // 环形缓冲区的当前放入位置
    int iget; // 缓冲区的当前取出位置
    int n; // 环形缓冲区中的元素总数量
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
    //状态 0 空闲 1 主叫对讲  2 被叫对讲  3 监视  4 被监视  5 主叫通话
    //6 被叫通话
	int RecordPic;  //留照片  0 不留  1 呼叫留照片  2 通话留照片
    unsigned char IP_Group[4];  //组播地址

    int CallConfirmFlag; //在线标志
    int Timer1Num;  //定时器1计数
    int OnlineFlag; //需检查在线确认
    int OnlineNum;  //在线确认序号
    int TimeOut;    //监视超时,  通话超时,  呼叫超时，无人接听
    int TalkTimeOut; //通话最长时间

    pthread_mutex_t udp_lock;//互斥锁
};

/*
struct Local1
{
    int Status;
    //状态 0 空闲 1 主叫对讲  2 被叫对讲  3 监视  4 被监视  5 主叫通话
    //6 被叫通话
    int KillStatus;
	int RecordPic;  //留照片  0 不留  1 呼叫留照片  2 通话留照片
    int IFrameCount; //I帧计数
    int IFrameNo;    //留第几个I帧
    unsigned char yuv[2][D1_W*D1_H*3/2];
    /////unsigned char yuv[2][CIF_W*CIF_H*3/2];
    int HavePicRecorded;  //有照片已录制
    struct tm *recpic_tm_t; //留照片时间

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
	unsigned char picAddr[4];//留影
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
    struct tm *call_tm_t; //被呼叫时间

    int CallConfirmFlag; //在线标志
    int Timer1Num;  //定时器1计数
    int OnlineFlag; //需检查在线确认
    int OnlineNum;  //在线确认序号
    int TimeOut;    //监视超时,  通话超时,  呼叫超时，无人接听
    int TalkTimeOut; //通话最长时间
    int RecPicSize;  //视频大小  1  352*288   2  720*480
    int PlayPicSize;  //视频大小  1  352*288   2  720*480
    ///////////mj
	int ForbTimeOut;
	pthread_mutex_t save_lock;//互斥锁
    pthread_mutex_t udp_lock;//互斥锁
    pthread_mutex_t comm_lock;//互斥锁
    int PrevWindow;      //上一个窗口编号
    int TmpWindow;       //暂存窗口编号 用于弹出窗口时
    int CurrentWindow;   //当前窗口编号
    int DefenceDelayFlag;    //布防延时标志
    int DefenceDelayTime;   //计数
    int PassLen;            //密码长度
    int AlarmDelayFlag[2];    //报警延时标志
    int AlarmDelayTime[2];   //计数

    int ForceIFrame;    //1 强制I帧
    int CalibratePos;   //校准触摸屏十字位置 0 1 2 3
    int CalibrateSucc;  //校准成功
    int CurrFbPage; //当前Fb页
    unsigned char IP_Group[4];  //组播地址
    unsigned char Weather[3];   //天气预报

    int AddrLen;          //地址长度  S 12  B 6 M 8 H 6

    int isHost;           //'0' 主机 '1' 副机 '2' ...
    int ConnToHost;       //与主机连接正常 1 正常 0 不正常
    unsigned char HostIP[4]; //主机IP
    unsigned char HostAddr[21]; //主机Addr
    int DenNum;             //目标数量  副机
    unsigned char DenIP[10][4];    //副机IP
    char DenAddr[10][21];         //副机Addr

    int NetStatus;   //网络状态 1 断开  0 接通
    int OldNetSpeed;  //网络速度
    int NoBreak;     //免扰状态 1 免扰  0 正常

    int ReportSend;  //设备定时报告状态已发送
    int RandReportTime; //设备定时报告状态随机时间
    int ReportTimeNum;  //计时
    //在GPIO线程中查询各线程是否运行
    int Key_Press_Run_Flag;
    int Save_File_Run_Flag;
    int Dispart_Send_Run_Flag;
    int Multi_Send_Run_Flag;
    int Multi_Comm_Send_Run_Flag;

    int MenuIndex;     //当前按钮索引
    int MaxIndex;      //本界面最大索引
    int MainMenuIndex;     //主界面按钮索引

    int OsdOpened;  //OSD打开标志

    int LcdLightFlag; //LCD背光标志
    int LcdLightTime; //时间

	//int NewINfo;
    int NewInfo[FLOOR][ROOM];  //有新信息

    int ResetPlayRingFlag;  //复位Audio Play flag

    int nowvideoframeno;   //当前视频帧编号
    int nowaudioframeno;   //当前音频帧编号

    int ForceEndWatch;  //有呼叫时，强制关监视
    int ZoomInOutFlag;  //正在放大缩小中
    uint32_t newzoomtime;
    uint32_t oldzoomtime;
    uint32_t newtouchtime;
    uint32_t oldtouchtime;    //上一次触摸屏处理时间
};
*/

struct LocalCfg1
{
    char Addr[20];             //地址编码
    unsigned char BuildAddr[2];
	unsigned char Mac_Addr[6]; //网卡地址
    unsigned char IP[4];       //IP地址
    unsigned char IP_Mask[4];  //子网掩码
    unsigned char IP_Gate[4];  //网关地址
    unsigned char IP_NS[4];    //NS（名称解析）服务器地址
    unsigned char IP_Server[4];  //主服务器地址（与NS服务器可为同一个）
    unsigned char IP_Broadcast[4];  //广播地址

    int ReportTime;      //设备定时报告状态时间
    unsigned char DefenceStatus;       //布防状态
    unsigned char DefenceNum;          //防区模块个数
    unsigned char DefenceInfo[32][10]; //防区信息

    char EngineerPass[10];             //工程密码
    char OpenLockPass[10];

    int In_DelayTime;                //进入延时
    int Out_DelayTime;               //外出延时
    int Alarm_DelayTime;               //报警延时

/////////////////////////0326////////////////from door/////////
    unsigned char OpenLockTime;
    unsigned char DelayLockTime;
    unsigned char PassOpenLock;
    unsigned char CardOpenLock;

    unsigned char bit_rate;
    ///////////////////////////////////////////////////
    int Ts_X0;                   //触摸屏
    int Ts_Y0;
    int Ts_deltaX;
    int Ts_deltaY;
};

struct Remote1
{
    int DenNum;             //目标数量  主机+副机
    unsigned char DenIP[4]; //对方IP或视频服务器IP
    unsigned char GroupIP[4]; //GroupIP
    unsigned char IP[10][4];    //对方IP
    int Added[10];                //已加入组
    char Addr[10][21];         //对方Addr
    int isDirect;       //是否直通  0 直通  1 中转
};

struct Info1
{
    int MaxNum;   //最大信息数
    int TotalNum; //信息总数
    int NoReadedNum; //未读信息总数
    int TotalInfoPage;   //总信息页数
    int CurrentInfoPage; //当前信息页
    int CurrNo;    //当前信息序号
    int CurrPlayNo;  //当前播放序号
    int TimeNum;    //计数
};

//单条信息内容结构体
struct InfoContent1
{
    uint8_t isValid;  //有效，未删除标志   1
    uint8_t isReaded; //已读标志    1
    uint8_t isLocked; //锁定标志    1
    char Time[32];    //接收时间    32
    uint8_t Type;     //类型        1    信息类型或事件类型
    uint32_t Sn;      //序号        4
    int Length;       //长度        4
    char Content[INFOMAXSIZE];//内容  400  内容或事件对象
    //char Event[20];         //事件
    char FileName[50];
};                               //内存分配为444

//////////////////paul0509
struct AlarmInfoContent1
{
    uint8_t isValid;  //有效，未删除标志   1
    uint8_t isReaded; //已读标志    1
    uint8_t isLocked; //锁定标志    1
    char Time[32];    //接收时间    32
    uint8_t Type;     //类型        1    信息类型或事件类型
//    uint32_t Sn;      //序号        4
//    int Length;       //长度        4
//    char Content[INFOMAXSIZE];//内容  400  内容或事件对象
    //char Event[20];         //事件
//    char FileName[50];
//	char AlarmInfo[20]; ///paul0509报警信息
}; 
////////////////////////////////////////////
//当前信息窗口状态
struct InfoStatus1
{
    int CurrType;  //当前信息类型
    int CurrWin;   //当前信息窗口  0 信息列表  1  信息内容
    int CurrNo;    //当前信息序号
};
//信息链表
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
//存储文件到FLASH队列 数据结构 由于存储FLASH速度较慢 用线程来操作
struct Save_File_Buff1
{
    int isValid; //是否有效
    int Type;    //存储类型 1－一类信息  2－单个信息  3－物业服务  4－本地设置
    int InfoType;   //信息类型
    int InfoNo;     //信息位置
    InfoNode1 *Info_Node; //信息结点
};

//UDP主动命令数据发送结构
struct Multi_Udp_Buff1
{
    int isValid; //是否有效
    int SendNum; //当前发送次数
    int CurrOrder;//当前命令状态,VIDEOTALK VIDEOTALKTRANS VIDEOWATCH VIDEOWATCHTRANS
    //主要用于需解析时，如单次命令置为0
    int m_Socket;
    char RemoteHost[20];
    unsigned char buf[1500];
    int DelayTime;  //等待时间
    int SendDelayTime;
    int nlength;
};

//COMM主动命令数据发送结构
struct Multi_Comm_Buff1
{
    int isValid; //是否有效
    int SendNum; //当前发送次数
    int m_Comm;
    unsigned char buf[1500];
    int nlength;
//	int udporder;
//	int commorder;
};

//通话数据结构
struct talkdata1
{
    char HostAddr[20];       //主叫方刂�
    unsigned char HostIP[4]; //主叫方IP地址
    char AssiAddr[20];       //被叫方地址
    unsigned char AssiIP[4]; //被叫方IP地址
    unsigned int timestamp;  //时间戳
    unsigned short DataType;          //数据类型
    unsigned short Frameno;           //帧序号
    unsigned int Framelen;            //帧数据长度
    unsigned short TotalPackage;      //总包数
    unsigned short CurrPackage;       //当前包数
    unsigned short Datalen;           //数据长度
    unsigned short PackLen;       //数据包大小
}__attribute__ ((packed));
//信息数据结构
struct infodata1
{
    char Addr[20];       //地址编码
    unsigned short Type; //类型
//  	unsigned char Type;
	unsigned int  Sn;         //序号
    unsigned short Length;   //数据长度
}__attribute__ ((packed));

#ifndef CommonH
#define CommonH
int DebugMode;           //调试模式
int DeltaLen;  //数据包有效数据偏移量
struct tm *curr_tm_t;
struct TimeStamp1 TimeStamp;  //接收时间与播放时间，同步用
int temp_video_n;      //视频接收缓冲个数
TempVideoNode1 *TempVideoNode_h;    //视频接收缓冲列表
int temp_audio_n;      //音频接收缓冲个数
TempAudioNode1 *TempAudioNode_h;    //音频接收缓冲列表

//系统初始化标志
int InitSuccFlag;
//本机状态设置
struct Local1 Local;
struct LocalCfg1 LocalCfg;

//远端地址
struct Remote1 Remote;
char null_addr[21];   //空字符串
char null_ip[4];   //空字符串
//免费ARP
int ARP_Socket;
//检测网络连接
int m_EthSocket;
//UDP
int m_DataSocket;
int m_VideoSocket;

int LocalDataPort;   //命令及数据UDP端口
int LocalVideoPort;  //音视频UDP端口

int RemoteDataPort;
int RemoteVideoPort;
char RemoteHost[20];
char sPath[80];
char currpath[80];   //自定义路径
char wavPath[80];
char UdpPackageHead[15];

//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
//用于UDP和Comm通信
int multi_send_flag;
pthread_t multi_send_thread;
void multi_send_thread_func(void);
sem_t multi_send_sem;
struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10个UDP主动发送缓冲

#else
extern int DebugMode;           //调试模式
extern int DeltaLen;  //数据包有效数据偏移量
extern struct tm *curr_tm_t;
extern struct TimeStamp1 TimeStamp;  //接收时间与播放时间，同步用
extern int temp_video_n;      //视频接收缓冲个数
extern TempVideoNode1 *TempVideoNode_h;    //视频接收缓冲列表
extern int temp_audio_n;      //音频接收缓冲个数
extern TempAudioNode1 *TempAudioNode_h;    //音频接收缓冲列表

//系统初始化标志
extern int InitSuccFlag;
//本机状态设置
extern struct Local1 Local;
extern struct LocalCfg1 LocalCfg;

//远端地址
extern struct Remote1 Remote;
extern char null_addr[21];   //空字符串
extern char null_ip[4];   //空字符串
//免费ARP
extern int ARP_Socket;
//检测网络连接
extern int m_EthSocket;
//UDP
extern int m_DataSocket;
extern int m_VideoSocket;

extern int LocalDataPort;   //命令及数据UDP端口
extern int LocalVideoPort;  //音视频UDP端口

extern int RemoteDataPort;
extern int RemoteVideoPort;
extern char RemoteHost[20];
extern char sPath[80];
extern char currpath[80];   //自定义路径
extern char wavPath[80];
extern char UdpPackageHead[15];

//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
//用于UDP通信
extern int multi_send_flag;
extern pthread_t multi_send_thread;
extern void multi_send_thread_func(void);
extern sem_t multi_send_sem;
extern struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10个UDP主动发送缓冲
#endif
