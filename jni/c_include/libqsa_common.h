#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>       //sem_t
#include <pthread.h>       //sem_t
#include <string.h>
#include <arpa/inet.h>

#include "libqsa_def.h"
#include "libqsa_callback.h"

#define LOGD printf

#define _FALSE 0
#define _TRUE  1

#define _MEPG2 0
#define _H264  1

#define DIRECTCALLTIME  600 

#define INTRTIME 50       //线程50ms
#define INTRPERSEC 20       //每秒20次线程
#define UDPSENDMAX  50  //UDP多次发送缓冲最大值
#define MAXSENDNUM  6  //最大发送次数
#define TIMERTIME 500						//线程500ms
#define TIMERPERSEC 2						//每秒2次线程

#define WATCHTIMEOUT  30*(1000/INTRTIME)    //监视最长时间
#define CALLTIMEOUT  25*(1000/INTRTIME)     //呼叫最长时间
#define TALKTIMEOUT  180*(1000/INTRTIME)//30*20     //通话最长时间
#define PREPARETIMEOUT  10*(1000/INTRTIME)     //留影留言预备最长时间
#define RECORDTIMEOUT  30*(1000/INTRTIME)     //留影留言最长时间

#define NSMULTIADDR  "238.9.9.1"//"192.168.10.255"  //NS组播地址
#define LFTMULTIADDR  "238.9.9.2"//"192.168.10.255"  //NS组播地址
#define MULTITTL   10      //组播TTL值

#define AUDIOPACKDATALEN  1200   //数据包大小
#define VIDEOPACKDATALEN  4096   //数据包大小
#ifdef _AUDIO_BUF_48MS
#define AUDIOBLK (128*6)   //每帧48ms
#else
#define AUDIOBLK 128   //每帧8ms
#endif

#define ALARM         1    //报警
#define CANCELALARM   2    //取消报警
#define SENDMESSAGE   3   //发送信息
#define REPORTSTATUS  4   //设备定时报告状态
#define QUERYSTATUS   5   //管理中心查询设备状态

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
#define LIFT		     110

struct Remote1 {
    int DenNum;             //目标数量  主机+副机
    unsigned char DenIP[4]; //对方IP或视频服务器IP
    unsigned char GroupIP[4]; //GroupIP
    unsigned char IP[10][4];    //对方IP
    int Added[10];                //已加入组
    char Addr[10][21];         //对方Addr
    int isDirect;       //是否直通  0 直通  1 中转
};

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

//UDP主动命令数据发送结构
struct Multi_Udp_Buff1 {
    int isValid; //是否有效
    int SendNum; //当前发送次数
    int CurrOrder;//当前命令状态,VIDEOTALK VIDEOTALKTRANS VIDEOWATCH VIDEOWATCHTRANS
    //主要用于需解析时，如单次命令置为0
    int m_Socket;
    char RemoteHost[20];
    char RemoteIP[20];               
    char DenIP[20];
    int RemotePort;
    unsigned char buf[1500];
    int DelayTime;  //等待时间
    int SendDelayTime;
    int nlength;
};

//通话数据结构
struct talkdata1 {
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

#ifndef _LIB_QSA_DEF_H
#define _LIB_QSA_DEF_H
struct _cb_function cb_opt_function;

struct dev_config local_config;                      //本地通用配置
int configlen = sizeof(struct dev_config);            //结构长度

struct Local1 Local;
//远端地址
struct Remote1 remote_info;
char null_addr[21];   //空字符串
char null_ip[4];   //空字符串

unsigned char locate_ip[4];

//免费ARP
int ARP_Socket;
//UDP
int m_VideoSocket;
int m_DataSocket;

int LocalDataPort;   //命令及数据UDP端口
int LocalVideoPort;  //音视频UDP端口

int RemoteDataPort;
int RemoteVideoPort;
int RemoteAudioPort;

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
extern struct _cb_function cb_opt_function;
extern struct dev_config local_config;               
extern int configlen;

extern struct Local1 Local;
extern struct Remote1 remote_info;
extern char null_addr[21];   //空字符串
extern char null_ip[4];   //空字符串
extern unsigned char locate_ip[4];

//免费ARP
extern int ARP_Socket;
//UDP
extern int m_VideoSocket;
extern int m_DataSocket;

extern int LocalDataPort;   //命令及数据UDP端口
extern int LocalVideoPort;  //音视频UDP端口

extern int RemoteDataPort;
extern int RemoteVideoPort;
extern int RemoteAudioPort;

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

extern void set_device_status(int uStatus);
#endif

