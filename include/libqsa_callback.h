
//CALLBACK Function
typedef void ( * _cb_audio_data)(void *, int, int);
typedef void ( * _cb_info)(const char *, const char *, int, int);
typedef int ( * _cb_status)(int);
typedef void ( * _cb_devip)(const char *, const char *, int);
typedef void ( * _cb_opt)(int);

struct _cb_function{
    _cb_audio_data cb_audio_data;
    _cb_status cb_curr_st;
    _cb_opt cb_curr_opt;
    _cb_devip cb_devip;
    _cb_info cb_info;
};

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

/* init callback function */ 
extern void set_cb_function(struct _cb_function * p);
/* find ip of the device */
extern void find_ip(const char * addr, int uFlag);
/* start call a user device */
extern void start_call(const char * ip, const char * addr, int uFlag);
/* stop a call or a talk */
extern void stop_talk(void);
/* send the public message or the private message */
extern void send_info(const char * data, struct _send_info * info);
/* send audio or video data to the user device */
extern void send_audio(const char * data, int length, int frame_num, const char * ip);
extern void send_video(const char * data, int length, int frame_num, int frame_type, const char * ip);


