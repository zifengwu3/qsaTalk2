#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

#include <assert.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
//#include <linux/videodev.h>
#include <arpa/inet.h>

#include "common.h"

int DebugMode = 0;
pthread_mutex_t audio_open_lock;
pthread_mutex_t audio_close_lock;
pthread_mutex_t audio_lock;

extern int InitArpSocket(void);
extern int CloseArpSocket(void);
extern void SendFreeArp(void);

extern int Init_Timer(void);
extern int Uninit_Timer(void);

extern void AddMultiGroup(int m_Socket, char *McastAddr);
extern void InitRecVideo(void);

extern int InitUdpSocket(short lPort);
extern void CloseUdpSocket(void);
extern int Init_Udp_Send_Task(void);
extern int Uninit_Udp_Send_Task(void);

int qsa_init_main_task(void);
void qsa_init_audio_task(void);
void qsa_init_udp_task(void);
void GetCfg(void);

//int init_param_task(struct dev_config * _config);
//int init_param_task(struct dev_config * _config)

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

void qsa_init_audio_task(void) {

    /*
    if (TempAudioNode_h == NULL) {
		TempAudioNode_h = (TempAudioNode1 *) init_audionode();
	}

	InitAudioParam();
	InitRecVideo();
     */
}

void qsa_uninit_task(void) {

	Uninit_Timer();

    Uninit_Udp_Send_Task();

	CloseArpSocket();
	CloseUdpSocket();
}

void qsa_init_udp_task(void) {

	if (InitUdpSocket(LocalVideoPort) == 0) {
		printf("can't create video socket.\n\r");
	}

	Init_Udp_Send_Task();
	SendFreeArp();
}

void GetCfg(void) {
	memcpy(LocalCfg.Addr, NullAddr, 20);
	memcpy(LocalCfg.Addr, "M00010100000", 12);
	LocalCfg.ReportTime = 10;
}

