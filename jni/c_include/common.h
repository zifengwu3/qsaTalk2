#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>       //sem_t
#include <semaphore.h>       //sem_t
#include <arpa/inet.h>
#include "sndtools.h"

#define _DEBUG           //调试模式
#define HARDWAREVER "M-HW VER 1.0"    //硬件版本
#define SOFTWAREVER "VER 1.19"    //test专用版本 add: m_videosendsocket, 整合新内容,增加门磁检测和电源检测,增加无组播呼叫中心程序
#define SERIALNUM "20080318"    //产品序列号

#define LFTMULTIADDR  "238.9.9.2"//"192.168.10.255"  //NS组播地址
#define NSMULTIADDR  "238.9.9.1"  //NS组播地址

#define WAVFILEMAX 10
#define INTRTIME 20       //线程20ms
#define INTRPERSEC 50       //每秒50次线程

#define IDCARDMAXNUM  100000  //ID卡最大数量
#define CARDPERPACK  350  //每个数据包内ID卡最大数量
#define IDCARDBRUSHNUM 10000 //最多离线刷卡条数
#define COMMMAX 1024     //串口缓冲区最大值
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

#define TIPW     320     //提示条宽度
#define TIPH     24      //提示条高度
#define TIPX     320     //提示条X
#define TIPY     24      //提示条Y
#define CLOCKW     260     //时钟宽度
#define CLOCKH     24      //时钟高度
#define CLOCKX     380     //时钟X
#define CLOCKY     5       //时钟Y

#define CALLCENTERX     50     //呼叫中心X
#define CALLCENTERY     380    //呼叫中心Y
#define CC_TALKTIMEW     120    //通话计时W
#define CC_TALKTIMEH     24    //通话计时H
#define CC_TALKTIMEX     50     //通话计时X
#define CC_TALKTIMEY     410    //通话计时Y

#define R2RX     50             //户户通话X
#define R2RY     380            //户户通话Y
#define R2R_TALKTIMEW     120    //通话计时W
#define R2R_TALKTIMEH     24    //通话计时H
#define R2R_TALKTIMEX     50     //通话计时X
#define R2R_TALKTIMEY     410    //通话计时Y

#define WATCHX     50     //监视X
#define WATCHY     380    //监视Y
#define W_TALKTIMEW     120    //通话计时W
#define W_TALKTIMEH     24    //通话计时H
#define W_TALKTIMEX     50     //通话计时X
#define W_TALKTIMEY     410    //通话计时Y

#define CALLX     50     //呼叫X
#define CALLY     380    //呼叫Y
#define C_TALKTIMEW     120    //通话计时W
#define C_TALKTIMEH     24    //通话计时H
#define C_TALKTIMEX     50     //通话计时X
#define C_TALKTIMEY     410    //通话计时Y

#define COMMSRVX     50     //物业服务X
#define COMMSRVY     380    //物业服务Y

#define COMMSRV_HH   32     //物业服务高亮H
#define COMMSRV_HW   180     //物业服务高亮W
#define COMMSRV_HX   60     //物业服务高亮X
#define COMMSRV_HY   80     //物业服务高亮Y

#define SETUP1X     50     //设置窗口1X
#define SETUP1Y     380    //设置窗口1Y

#define INFOBOXW     390    //信息显示框W
#define INFOBOXH     330    //信息显示框H
#define INFOBOXX     300     //信息显示框X
#define INFOBOXY     50-4    //信息显示框Y

#define INFOBUTTONW     590    //信息按钮部分W
#define INFOBUTTONH     40    //信息按钮部分H
#define INFOBUTTONX     100     //信息按钮部分X
#define INFOBUTTONY     380    //信息按钮部分Y

#define INFONUMW     70    //信息总信息，未读信息W
#define INFONUMH     24    //信息总信息，未读信息H
#define INFONUMX     224     //信息总信息，未读信息X
#define INFONUMY     60    //信息总信息，未读信息Y

#define INFONUM2X     230     //信息总信息，未读信息X
#define INFONUM2Y     60+48    //信息总信息，未读信息Y

#define INFONUM3X     230     //信息总信息，未读信息X
#define INFONUM3Y     60+48*2    //信息总信息，未读信息Y

#define INFONUM4X     230     //信息总信息，未读信息X
#define INFONUM4Y     60+48*3    //信息总信息，未读信息Y

#define INFOROWW     370    //单条信息，正常和高亮显示
#define INFOROWH     32
#define INFOROWX     300
#define INFOROWY     60

#define LWORDNUMW     230    //信息总信息，未读信息W
#define LWORDNUMH     24    //信息总信息，未读信息H
#define LWORDNUMX     100     //信息总信息，未读信息X
#define LWORDNUMY     60    //信息总信息，未读信息Y

#define LWORDBOXW     420    //留言显示框W
#define LWORDBOXH     230    //留言显示框H
#define LWORDBOXX     140     //留言显示框X
#define LWORDBOXY     96    //留言显示框Y

#define USERW     96    //使用者设置
#define USERH     30
#define USERX     40
#define USERY     50

#define WATCHTIMEOUT  32*INTRPERSEC//30*20    //监视最长时间
#define CALLTIMEOUT  30*INTRPERSEC     //呼叫最长时间
#define TALKTIMEOUT  130*INTRPERSEC//30*20     //通话最长时间
#define PREPARETIMEOUT  10*INTRPERSEC     //留影留言预备最长时间
#define RECORDTIMEOUT  30*INTRPERSEC     //留影留言最长时间

//命令 管理中心
#define ALARM  1
#define SENDMESSAGE   3   //发送信息
#define REPORTSTATUS  4   //设备定时报告状态
#define QUERYSTATUS   5   //管理中心查询设备状态
#define SENDCOMMSRV   7   //发送物业服务
#define WRITEADDRESS   40   //写地址设置
#define READADDRESS    41   //读地址设置
#define WRITECOMMMENU   42   //写物业服务菜单
#define READCOMMMENU   43   //读物业服务菜单
#define WRITEHELP   50   //写帮助信息
#define READHELP    51   //读帮助信息
#define WRITESETUP     52   //写设置信息
#define READSETUP      53   //读设置信息
#define WRITEIDCARD    54   //写ID卡信息
#define READIDCARD     55   //读ID卡信息
#define BRUSHIDCARD     56   //刷ID卡信息，向服务器发送
//对讲
#define VIDEOTALK      150 //局域网可视对讲
#define VIDEOTALKTRANS 151 //局域网可视对讲中转服务
#define VIDEOWATCH     152 //局域网监控
#define VIDEOWATCHTRANS   153 //局域网监控中转服务
#define NSORDER        154 //主机名查找（子网内多播）
#define NSSERVERORDER  155 //主机名查找(NS服务器)
#define FINDEQUIP      170 //查找设备
/////////////paul0723
#define CHECKMAC	161
#define SETPARAM 	162
#define MAKEOLD		163
#define READMAC		164
#define MCUUPDATE	165

#define READVILLA   170
#define SETVILLA    171
/////////////////////////////
//0x80 发送通话信息给中心机
//0x81 发送开锁信息给中心机//zhou101123
#define SENDTALKINFO   0x80 //通话信息发送 由第55位
////////////////////////
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

#define SAVEMAX  10     //FLASH存储缓冲最大值
#define UDPSENDMAX  50  //UDP多次发送缓冲最大值
#define COMMSENDMAX  20  //COMM多次发送缓冲最大值
#define DISPARTMAX  10   //拆分包发送缓冲
#define MAXCOMMSENDNUM  3
#define MAXSENDNUM  6  //最大发送次数
//按钮压下时间
#define DELAYTIME  300

//视频常量
#define cWhite  1
#define cYellow 2
#define cCyan   3
#define cGreen  4
#define cMagenta  5
#define cRed      6
#define cBlue     7
#define cBlack    8
#define	FB_DEV	"/dev/fb0"
#define MAXPIXELS (1280*1024)  /* Maximum size of final image */
#define VIDEO_PICTURE_QUEUE_SIZE_MAX 20

#define CONFLICTARP  0x8950
#define FLCD_GET_DATA_SEP   0x46db
#define FLCD_GET_DATA       0x46dc
#define FLCD_SET_FB_NUM     0x46dd
#define FLCD_SWITCH_MODE    0x46de
#define FLCD_CLOSE_PANEL    0x46df
#define FLCD_BYPASS    0x46e0
#define FLCD_OPEN	0x46fa
#define FLCD_CLOSE	0x46fb

#define DEVICE_GPIO                "/dev/gpio"
#define IO_PUT                 0
#define IO_CLEAR               3
#define IO_READ         4
#define IO_SETINOUT     5
#define IO_TRIGGERMODE  6
#define IO_EDGE         7
#define IO_SETSCANVALUE     8
#define IO_SETVALUE     9

//20080401 设置PMU，关闭不用的时钟
#define CLOSE_PMU1  0x2255
#define CLOSE_PMU2  0x2256

#define NMAX 512*64  //AUDIOBLK*64  //音频环形缓冲区大小
#define G711NUM  64*512/AUDIOBLK       //音频接收缓冲区个数 未解码   10

#define VIDEOMAX 720*576
#define VNUM  3         //视频采集缓冲区大小
#define VPLAYNUM  10         //视频播放缓冲区大小         6
#define MP4VNUM  10         //视频接收缓冲区个数 未解码   10
#define PACKDATALEN  1200   //数据包大小
#define MAXPACKNUM  100     //帧最大数据包数量

//////////////////paul0518 for update//////////////////
#define DOWNLOADFILE  224    //下载应用程序
#define DOWNLOADIMAGE  225    //下载系统映像
#define STARTDOWN  1       //开始下载
#define DOWN       2       //下载
#define DOWNFINISHONE       3  //下载完成一个
#define STOPDOWN       10      //停止下载
#define DOWNFINISHALL       20 //全部完成下载
#define DOWNFAIL         21 //下载失败  设备－》管理中心

#define ERASEFLASH  31    //正在删除Flash
#define WRITEFLASH  32    //正在写Flash
#define CHECKFLASH  33    //正在校验Flash
#define ENDFLASH  34      //完成写Image
#define ERRORFLASH  35      //操作Image失败

struct downfile1
{
	char FlagText[20];     //标志字符串
	char FileName[20];
	unsigned int Filelen;            //文件大小
	unsigned short TotalPackage;      //总包数
	unsigned short CurrPackage;       //当前包数
	unsigned short Datalen;           //数据长度
}__attribute__ ((packed));
///////////////paul0902///////////////////////
struct TalkDen1{
	unsigned char IP[10][4];    //对方IP
	char Addr[10][21];         //对方Addr
	int ExistFlag[10];          //目标是否存在
};
////////////////////////////////////////////////////////////////

struct TimeStamp1{
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
//视频接收缓冲 链表
typedef struct node2{
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
	pthread_mutex_t lmovie_lock;//[VPLAYNUM];//存储留影互斥锁
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
typedef struct node3{
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

struct flcd_data
{
	unsigned int buf_len;
	unsigned int uv_offset;
	unsigned int frame_no;
	unsigned int mp4_map_dma[VIDEO_PICTURE_QUEUE_SIZE_MAX];
};


/*typedef */struct fcap_frame_buff
{
	unsigned int phyAddr;
	unsigned int mmapAddr;   //length per dma buffer
	unsigned int frame_no;
};
//////////////////////updatemcu
struct update_mcu
{
	char Type;
	int Length;
	int Pack;
	short Curr;
	int commfd;
	char FromIP[20];
	int  Socket;
	char Towho[4];
	char McuData[100*1024];
};
////////////////////////////////////

struct Local1{
	int Status;
	//状态 0 空闲 1 主叫对讲  2 被叫对讲  3 监视  4 被监视  5 主叫通话
	//6 被叫通话 
	int DoorNo;
	char FindEquip[20];
	int CallConfirmFlag; //在线标志
	int Timer1Num;  //定时器1计数
	int OnlineFlag; //需检查在线确认
	int OnlineNum;  //在线确认序号
	int TimeOut;    //监视超时,  通话超时,  呼叫超时，无人接听
	int RecPicSize;  //视频大小  1  352*288   2  720*480
	int PlayPicSize;  //视频大小  1  352*288   2  720*480
	pthread_mutex_t save_lock;//互斥锁
	pthread_mutex_t udp_lock;//互斥锁
	pthread_mutex_t comm_lock;//互斥锁
	int PassLen;            //密码长度
	int ForceIFrame;    //1 强制I帧
	char ClockText[20];  //当前时钟
	int InputMax;       //输入最大长度  围墙机  10位  单元门口机  4位
	unsigned char IP_Group[4];  //组播地址

	struct tm *call_tm_t;
	int AddrLen;          //地址长度  S 12  B 6 M 8 H 6 
	int villacount;
	int isHost;           //'0' 主机 '1' 副机 '2' ... 
	int DenNum;             //目标数量  副机
	unsigned char DenIP[10][4];    //副机IP
	char DenAddr[10][21];         //副机Addr

	int NetStatus;   //网络状态 1 断开  0 接通
	int OldNetSpeed;  //网络速度 

	int ReportSend;  //设备定时报告状态已发送
	int RandReportTime; //设备定时报告状态随机时间
	int ReportTimeNum;  //计时                   

	int OpenDoorFlag; //开锁标志 0 未开锁  1 开锁延时中  2 开锁中
	int OpenDoorTime; //时间
	//在GPIO线程中查询各线程是否运行
	int Key_Press_Run_Flag;
	int Save_File_Run_Flag;
	int Dispart_Send_Run_Flag;
	int Multi_Send_Run_Flag;
	int Multi_Comm_Send_Run_Flag;

	int nowvideoframeno;   //当前视频帧编号
	int nowaudioframeno;   //当前音频帧编号
	char updatemcu;
	char leftmessage;
	char reportinfo;
	//zhou110809
	int videozoomout;
	int OpenD1VideoFlag;//打开D1视频标记
	int OpenCIFVideoFlag;//打开CIF视频标记
	//zhou110907
	int WaitforEnd;//呼叫，监视结束状态，保持大约2S的时间，该时间内，所以其他呼叫都回忙：：
	int WaitTimeOut; //等待定时时间
#ifdef ISSETUPPACKETSIZE
	unsigned char minpacket_flag;
#endif
};
struct LocalCfg1{
	char Addr[20];             //地址编码
	unsigned char Mac_Addr[6]; //网卡地址
	unsigned char IP[4];       //IP地址
	unsigned char IP_Mask[4];  //子网掩码
	unsigned char IP_Gate[4];  //网关地址
	unsigned char IP_NS[4];    //NS（名称查找）服务器地址
	unsigned char IP_Server[4];  //主服务器地址（与NS服务器可为同一个）
	unsigned char IP_Broadcast[4];  //广播地址

	int ReportTime;      //设备定时报告状态时间

	unsigned char OpenLockTime;   //开锁时间  0 200ms  1 500ms  2 5s  3 10s
	unsigned char DelayLockTime;   //延时开锁  0 0s  1 3s  2 5s  3 10s
	unsigned char PassOpenLock;   //密码开锁
	unsigned char CardOpenLock;   //刷卡开锁

	unsigned char bit_rate;        //视频CIF时带宽    0-128k    1-256k   2-384k    3-512k   4-768  5-1024 

	char EngineerPass[10];           //工程密码
	char OpenLockPass[10];             //开锁密码
};
//ID卡信息
struct IDCardNo1{
	int Num;        //数量
	unsigned char SN[IDCARDMAXNUM*4]; //ID卡号  最大10万张
	uint32_t serialnum;     //序号
};
//写ID卡信息
struct RecvIDCardNo1{
	int isNewWriteFlag;  //新写标志
	int Num;             //卡数量
	int PackNum;         //包数量
	int PackIsRecved[IDCARDMAXNUM/CARDPERPACK + 1];  //包已接收标志
	unsigned char SN[IDCARDMAXNUM*4]; //ID卡号  最大10万张
	uint32_t serialnum;     //序号
};
//刷ID卡信息
struct BrushIDCard1{
	int Num;        //数量
	unsigned char Info[IDCARDBRUSHNUM*11]; //ID卡号  离线信息最多1万条
	//前4字节卡号，后7字节时间
};
//串口接收缓冲区
struct commbuf1
{
	int iput; // 环形缓冲区的当前放入位置
	int iget; // 缓冲区的当前取出位置
	int n; // 环形缓冲区中的元素总数量
	unsigned char buffer[COMMMAX];
};
//状态提示信息缓冲
//Type
//          11 -- 呼叫中心
//          12 -- 户户通话
//          13 -- 监视
//          16 -- 对讲图像窗口
struct PicStatBuf1{
	int Flag;                      //延时清提示信息标志
	int Type;                      //类型
	int Time;
	int MaxTime;                   //最长时间

	int KeyPressTime;
};
struct Remote1{
	int DenNum;             //目标数量  主机+副机
	unsigned char DenIP[4]; //对方IP或视频服务器IP
	unsigned char GroupIP[4]; //GroupIP
	unsigned char IP[10][4];    //对方IP
	int Added[10];                //已加入组
	char Addr[10][21];         //对方Addr
	int isDirect;       //是否直通  0 直通  1 中转
};

//存储文件到FLASH队列 数据结构 由于存储FLASH速度较慢 用线程来操作
struct Save_File_Buff1{
	int isValid; //是否有效
	int Type;    //存储类型   4－本地设置
	int InfoType;   //信息类型
	int InfoNo;     //信息位置
	char cFromIP[15];   //用于读ID卡应答
};

//UDP主动命令数据发送结构
struct Multi_Udp_Buff1{
	int isValid; //是否有效
	int SendNum; //当前发送次数
	int CurrOrder;//当前命令状态,VIDEOTALK VIDEOTALKTRANS VIDEOWATCH VIDEOWATCHTRANS
	//主要用于需查找时，如单次命令置为0
	int m_Socket;
	char RemoteHost[20];
	unsigned char buf[1500];
	int DelayTime;  //等待时间
	int nlength;
};
//COMM主动命令数据发送结构
struct Multi_Comm_Buff1{
	int isValid; //是否有效
	int SendNum; //当前发送次数
	int m_Comm;
	unsigned char buf[1500];
	int nlength;
};
//通话数据结构
struct talkdata1
{
	char HostAddr[20];       //主叫方地址
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
//读写ID卡数据结构
struct iddata1
{
	char Addr[20];       //地址编码
	unsigned int serialnum;  //序号
	unsigned int  Num;          //卡总数
	unsigned int  CurrNum;           //当前数据包卡数量
	unsigned int  TotalPackage;      //总包数
	unsigned int  CurrPackage;       //当前包数
}__attribute__ ((packed));
//信息数据结构
struct infodata1
{
	char Addr[20];       //地址编码
	unsigned short Type; //类型
	unsigned int  Sn;         //序号
	unsigned short Length;   //数据长度
}__attribute__ ((packed));
///////////////////////////////////////zhou101102
struct SendTalkInfo1
{
	int Status;//状态
	int Duration;//通话时长
	char Addr[21];//对方地址
	char StartTime[16];//开始呼叫时间
};
#ifndef CommonH
#define CommonH
struct TalkDen1 TalkDen; 
struct TimeStamp1 TimeStamp;  //接收时间与播放时间，同步用

//////////////////////////
int NeedSendPhone;//是否立即发送给电话模块//zhou101014
//////////////////////////zhou101102
struct SendTalkInfo1 SendTalkInfo;
int SendTalkInfoFlag;
int NeedOpenLock; //0为空，不上传 1为上传//zhou101102 增加主动上传门信息 由于OpenLock_Func为UDP数据线程
int DeltaLen;  //数据包有效数据偏移量

struct tm *curr_tm_t;

int temp_video_n;      //视频接收缓冲个数
TempVideoNode1 *TempVideoNode_h;    //视频接收缓冲列表  
int temp_audio_n;      //音频接收缓冲个数
TempAudioNode1 *TempAudioNode_h;    //音频接收缓冲列表

//系统初始化标志
int InitSuccFlag;
//本机状态设置
struct Local1 Local;
struct LocalCfg1 LocalCfg;
int TurnToCenter;
int Cif_Bit_rate[6] = {128, 256, 384, 512, 768, 1024};
//ID卡信息
struct IDCardNo1 IDCardNo;
//刷ID卡信息
struct BrushIDCard1 BrushIDCard;
//写ID卡信息
struct RecvIDCardNo1 RecvIDCardNo;

//远端地址
struct Remote1 Remote;
char NullAddr[21];   //空字符串
//COMM
int Comm2fd;  //串口2句柄
int Comm3fd;  //串口3句柄
int Comm4fd;  //串口4句柄
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
/////////////////////////////
int m_ForbidSocket;
int LocalForbidPort;
int RemoteForbidPort;
//////////////////////////////
int m_VideoSendSocket;
//////////////////////////////
char RemoteHost[20];
char sPath[80];
char wavPath[80];
char UdpPackageHead[15];
//状态提示信息缓冲
struct PicStatBuf1 PicStatBuf;

//FLASH存储线程
int save_file_flag;
pthread_t save_file_thread;
void save_file_thread_func(void);
sem_t save_file_sem;
struct Save_File_Buff1 Save_File_Buff[SAVEMAX]; //FLASH存储缓冲最大值

//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
//用于UDP和Comm通信
int multi_send_flag;
pthread_t multi_send_thread;
void multi_send_thread_func(void);
sem_t multi_send_sem;
struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10个UDP主动发送缓冲

//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
//用于UDP和Comm通信
int multi_comm_send_flag;
pthread_t multi_comm_send_thread;
void multi_comm_send_thread_func(void);
sem_t multi_comm_send_sem;
struct Multi_Comm_Buff1 Multi_Comm_Buff[COMMSENDMAX]; //10个COMM主动发送缓冲

//watchdog
int watchdog_fd;

//gpio 按键
int gpio_fd;
int gpio_rcv_flag;
pthread_t gpio_rcv_thread;
void gpio_rcv_thread_func(void);

int OpenLockTime[4];       //开锁时间
char OpenLockTimeText[4][20];  //文本
int DelayLockTime[4];       //延时开锁时间
char DelayLockTimeText[4][20];       //文本
#else
//////////////////////////
extern int NeedSendPhone;//是否立即发送给电话模块//zhou101014
//////////////////////////zhou101102
extern struct SendTalkInfo1 SendTalkInfo;
extern int SendTalkInfoFlag;
extern int NeedOpenLock; //0为空，不上传 1为上传//zhou101102 增加主动上传门信息 由于OpenLock_Func为UDP数据线程
extern struct TalkDen1 TalkDen;
extern struct TimeStamp1 TimeStamp;  //接收时间与播放时间，同步用

extern int DeltaLen;  //数据包有效数据偏移量

extern struct tm *curr_tm_t;

extern int temp_video_n;      //视频接收缓冲个数
extern TempVideoNode1 *TempVideoNode_h;    //视频接收缓冲列表  
extern int temp_audio_n;      //音频接收缓冲个数
extern TempAudioNode1 *TempAudioNode_h;    //音频接收缓冲列表

//系统初始化标志
extern int InitSuccFlag;
//本机状态设置
extern struct Local1 Local;
extern struct LocalCfg1 LocalCfg;
extern int TurnToCenter;
extern int Cif_Bit_rate[6];
//ID卡信息
extern struct IDCardNo1 IDCardNo;
//刷ID卡信息
extern struct BrushIDCard1 BrushIDCard;  
//写ID卡信息
extern struct RecvIDCardNo1 RecvIDCardNo;
//远端地址
extern struct Remote1 Remote;
extern char NullAddr[21];   //空字符串
//COMM
extern int Comm2fd;  //串口2句柄
extern int Comm3fd;  //串口3句柄
extern int Comm4fd;  //串口4句柄
//免费ARP
extern int ARP_Socket;
//检测网络连接
extern int m_EthSocket;   
//UDP
extern int m_DataSocket;
extern int m_VideoSocket;
extern int LocalDataPort;   //命令及数据UDP端?
extern int LocalVideoPort;  //音视频UDP端口
extern int RemoteDataPort;
extern int RemoteVideoPort;
////////////
extern int m_VideoSendSocket;
//////////////////////
extern int m_ForbidSocket;
extern int LocalForbidPort;
extern int RemoteForbidPort; 
/////////////////
extern char RemoteHost[20];
extern char sPath[80];
extern char wavPath[80];
extern char UdpPackageHead[15];
//状态提示信息缓冲
extern struct PicStatBuf1 PicStatBuf;
//FLASH存储线程
extern int save_file_flag;
extern pthread_t save_file_thread;
extern void save_file_thread_func(void);
extern sem_t save_file_sem;
extern struct Save_File_Buff1 Save_File_Buff[SAVEMAX]; //FLASH存储缓冲最大值

//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
//用于UDP和Comm通信
extern int multi_send_flag;
extern pthread_t multi_send_thread;
extern void multi_send_thread_func(void);
extern sem_t multi_send_sem;
extern struct Multi_Udp_Buff1 Multi_Udp_Buff[UDPSENDMAX]; //10个UDP主动发送缓冲
//主动命令数据发送线程：终端主动发送命令，如延时一段没收到回应，则多次发送
//用于UDP和Comm通信
extern int multi_comm_send_flag;
extern pthread_t multi_comm_send_thread;
extern void multi_comm_send_thread_func(void);
extern sem_t multi_comm_send_sem;
extern struct Multi_Comm_Buff1 Multi_Comm_Buff[COMMSENDMAX]; //10个COMM主动发送缓冲

//watchdog
extern int watchdog_fd;  

//gpio 按键
extern int gpio_fd;
extern int gpio_rcv_flag;
extern pthread_t gpio_rcv_thread;
extern void gpio_rcv_thread_func(void);

extern int OpenLockTime[4];       //开锁时间
extern char OpenLockTimeText[4][20];  //文本
extern int DelayLockTime[4];       //延时开锁时间
extern char DelayLockTimeText[4][20];       //文本
#endif
