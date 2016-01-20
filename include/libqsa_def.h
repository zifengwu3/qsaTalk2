struct dev_config {
	unsigned char mac[12];			    //00:00:00:36:25:9A
	unsigned char address[20];			//M0101010000100000000
	unsigned char ip[4];				//192.168.10.XX
	unsigned char mask[4];				//255.255.0.0
	unsigned char route[4];				//192.168.10.1
	unsigned char center_ip[4];			//192.168.10.XX
	unsigned char server_ip[4]; 		//192.168.10.XX
	unsigned char undefined_ip[100];	//192.168.10.XX x 25
	unsigned char undefined_param[100];
};

struct _send_info {
    char * ip;		     //发送信息的设备IP地址，需要广告机或上位服务器提供（搜网）
    char * addr;		 //发送信息的设备房号地址，4位房号地址,需要广告机或上位服务器提供（搜网）
    char * title;		 //发送的信息的标题，需要上位服务器提供
    int length;		     //发送的信息的长度，需要上位服务器提供
    int port;		     //发送信息的设备端口号，由默认值或上位服务器提供
    int uFlag;		     //信息类型，1为私人信息，2为公共信息，3为私人图片信息，4为公共图片信息
};

enum {
    CB_CALL_OK = 0,      //呼叫成功回复
    CB_CALL_BUSY,        //当前正忙回复
    CB_TALK_OK,          //对讲开始命令回复
    CB_TALK_STOP,        //室内机挂机命令
    CB_OPEN_LOCK,        //开锁回复
    CB_FORCE_IFRAME,	 //强制I帧
    CB_INFO_OK,		     //发送信息回复
    CB_CALL_LIFT,		 //呼梯回调命令
    CB_CALL_FAIL,		 //呼叫状态异常
    CB_TALK_TIMEOUT,     //对讲180秒超时
    CB_CALL_TIMEOUT,     //呼叫30秒超时
    CB_ACK_TIMEOUT       //发送6次超时
} _CB_OPT;

enum {
	CALL_MIXER = 0,      //混合呼叫模式
	CALL_SIP,            //SIP呼叫
	CALL_DEVICE          //本地呼叫
} _CALL_MODE;

enum {
    CB_ST_NULL = 0,       //状态空闲
    CB_ST_CALLING,        //当前设备正在呼叫其他设备中
    CB_ST_CALLED,         //当前设备正在被叫
    CB_ST_WATCHED,        //当前设备处于被监视状态中
    CB_ST_WATCHING,	      //当前设备正在监视其他设备中
    CB_ST_TALKING,	      //当前设备正在对讲状态中
    CB_ST_TALKED, 		  //当前设备正在被叫对讲中
    CB_ST_FINDING = 99	  //当前设备正在搜网
}_CB_CALL_STATUS;

enum {
    CB_TYPE_AUDIO = 0
} _CB_TYPE_DATA;

#define _FALSE 0
#define _TRUE  1

#define DIRECTCALLTIME  600 

#define INTRTIME 50       //线程50ms
#define INTRPERSEC 20       //每秒20次线程
#define UDPSENDMAX  50  //UDP多次发送缓冲最大值
#define MAXSENDNUM  6  //最大发送次数
#define TIMERTIME 500						//线程500ms
#define TIMERPERSEC 2						//每秒2次线程

#define WATCHTIMEOUT  30*(1000/INTRTIME)    //监视最长时间
#define CALLTIMEOUT  25*(1000/INTRTIME)     //呼叫最长时间
#define TALKTIMEOUT  130*(1000/INTRTIME)//30*20     //通话最长时间
#define PREPARETIMEOUT  10*(1000/INTRTIME)     //留影留言预备最长时间
#define RECORDTIMEOUT  30*(1000/INTRTIME)     //留影留言最长时间

#define NSMULTIADDR  "238.9.9.1"//"192.168.10.255"  //NS组播地址
#define LFTMULTIADDR  "238.9.9.2"//"192.168.10.255"  //NS组播地址
#define MULTITTL   5      //组播TTL值

#define PACKDATALEN  1200   //数据包大小
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
