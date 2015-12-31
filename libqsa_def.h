
struct dev_config {
	unsigned char address[20];			//M0101010000100000000
	unsigned char ip[4];				//192.168.10.XX
	unsigned char mask[4];				//255.255.0.0
	unsigned char route[4];				//192.168.10.1
	unsigned char center_ip[4];			//192.168.10.XX
	unsigned char server_ip[4]; 		//192.168.10.XX
	unsigned char undefined_ip[100];	//192.168.10.XX x 25
	unsigned char undefined_param[100];
};

enum {
	CB_CALL_OK = 0,     //呼叫成功回复
	CB_CALL_BUSY,       //当前正忙回复
	CB_TALK_OK,         //对讲开始命令回复
	CB_TALK_STOP,       //室内机挂机命令
	CB_OPEN_LOCK,       //开锁回复
	CB_CALL_TIMEOUT,    //呼叫30秒未回复
	CB_ACK_TIMEOUT      //发送6次未回复
} _CB_STATUS;

enum {
      CB_TYPE_AUDIO = 0
} _CB_TYPE_DATA;
