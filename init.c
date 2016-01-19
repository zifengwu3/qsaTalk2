#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "libqsa_common.h"

extern void Init_Timer();
extern int init_udp_task(void);

int init_param_task(struct dev_config * _config);
void init_config_null_task(void);
void init_local_param_task(void);
void init_default_ip_task(void);
void init_default_remote_task(void);
void get_device_config_task(struct dev_config * _config);

int init_param_task(struct dev_config * _config) {
    
    //初始化设备参数数据
    init_config_null_task();
    get_device_config_task(_config);

    //初始化本地数据
    init_local_param_task();
    init_default_ip_task();
    init_default_remote_task();

    //初始化线程
	Init_Timer();
	init_udp_task();

	return (0);
}

void init_config_null_task(void) {
    memset(&local_config, 0x00, configlen);
    return;
}

void get_device_config_task(struct dev_config * _config) {
    memcpy(&local_config, _config, configlen);
    return;
}

void init_local_param_task(void) {

    RemoteDataPort = 8300;
	LocalDataPort = 8300;

	RemoteVideoPort = 8302;
	LocalVideoPort = 8302;

    strcpy(UdpPackageHead, "QIUSHI");
    strcpy(RemoteHost, "192.168.0.88");

    memset(null_addr, 0x30, 20);
    null_addr[20] = '\0';

    memset(null_ip, 0, 4);

	memcpy(remote_info.Addr[0], null_addr, 20);
	memcpy(remote_info.Addr[0], "S00010101010", 12);

    return;
}

void init_default_ip_task(void) {

    return;
}

void init_default_remote_task(void) {

    memset(&remote_info, 0x00, sizeof(struct Remote1));
    remote_info.DenNum = 0;             //表示使用默认参数
    remote_info.isDirect = 1;
}


/*
int qsa_init_main_task(void) {
	int i;
	uint32_t Ip_Int;

	DebugMode = 1;

	TimeStamp.OldCurrVideo = 0;
	TimeStamp.CurrVideo = 0;
	TimeStamp.OldCurrAudio = 0;
	TimeStamp.CurrAudio = 0;

	RemoteVideoPort = 8302;
	strcpy(RemoteHost, "192.168.0.88");
	LocalVideoPort = 8302;

	//ReadCfgFile();
	GetCfg();
	DeltaLen = 9 + sizeof(struct talkdata1);

	strcpy(UdpPackageHead, "QIUSHI");
	for (i = 0; i < 20; i++) {
		NullAddr[i] = '0';
	}
	NullAddr[20] = '\0';

	Local.Status = 0;
	Local.RecPicSize = 1;
	Local.PlayPicSize = 1;

	Ip_Int = inet_addr("192.168.0.5");
	memcpy(Remote.IP, &Ip_Int, 4);
	memcpy(Remote.Addr[0], NullAddr, 20);
	memcpy(Remote.Addr[0], "S00010101010", 12);

	InitArpSocket();

	qsa_init_audio_task();
	qsa_init_udp_task();

	Init_Timer();

	return (0);
}
*/

