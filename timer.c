/************************************************************
 *  File:
 *  Author:
 *  Version :
 *  Date:
 *  Description:
 *  History:
 *            <Zhou Xiaomin> <2016-01-20>
 ***********************************************************/

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#define _LIB_QSA_DEF_H
#include "libqsa_common.h"

void OnlineCheckFunc(void);
void TalkCtrlFunc(void);

int timer_fd;
int timer_thread_flag;
pthread_t timer_thread;
void timer_thread_task(void);
extern int UdpSendBuff(int m_Socket, char * RemoteHost, int RemotePort,
		unsigned char * buf, int nlength);

int Init_Timer(void);
int Uninit_Timer(void);

int Init_Timer(void) {
	pthread_attr_t attr;
	timer_thread_flag = 1;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&timer_thread, &attr, (void *) timer_thread_task, NULL);
	pthread_attr_destroy(&attr);
	if (timer_thread == 0) {
		printf("don't create timer thread \n");
		return (-1);
	}
	return 0;
}

int Uninit_Timer(void) {
	//timer
	timer_thread_flag = 0;
	usleep(40 * 1000);
	return 0;
}

void timer_thread_task(void) {

	int timenum;

	printf("create timer thread :0.001\n");

	timenum = 0;
	while (timer_thread_flag) {

		if (Local.OnlineFlag == 1) {
			OnlineCheckFunc();
            TalkCtrlFunc();
		}

		timenum++;

		if (timenum > 0xFFFFFF) {
			timenum = 0;
		}
		usleep((TIMERTIME-10)*1000);
	}

}

//在线检查
void OnlineCheckFunc(void)
{
    int sendlength;
    int RemotePort;
	unsigned char send_b[1520];
    int Status = 0;
    Status = get_device_status(CALL_MIXER);

	if (Local.Timer1Num > INTRPERSEC * 20) {
		if (Local.CallConfirmFlag == 0) {
			if ((Status == CB_ST_CALLING) || (Status == CB_ST_CALLED)
					|| (Status == CB_ST_TALKING) || (Status == CB_ST_TALKED)) {
				stop_talk();
                cb_opt_function.cb_curr_opt(CB_TALK_STOP);
			}
			Local.OnlineFlag = 0;
		} else {
			Local.CallConfirmFlag = 0;
		}
		Local.Timer1Num = 0;
	} else if ((Local.Timer1Num % INTRPERSEC) == 0) {
		if ((Status == CB_ST_CALLED) || (Status == CB_ST_TALKED)) {
			memcpy(send_b, UdpPackageHead, 6);
			send_b[6] = VIDEOTALK;

			send_b[7] = ASK;
			send_b[8] = CALLCONFIRM;
			memcpy(send_b + 9, local_config.address, 20);
			memcpy(send_b + 29, local_config.ip, 4);
			memcpy(send_b + 33, remote_info.Addr[0], 20);
			memcpy(send_b + 53, remote_info.IP[0], 4);
			send_b[57] = (Local.OnlineNum & 0xFF000000) >> 24;
			send_b[58] = (Local.OnlineNum & 0x00FF0000) >> 16;
			send_b[59] = (Local.OnlineNum & 0x0000FF00) >> 8;
			send_b[60] = Local.OnlineNum & 0x000000FF;
			Local.OnlineNum++;
			sendlength = 62;
			sprintf(RemoteHost, "%d.%d.%d.%d", remote_info.DenIP[0], remote_info.DenIP[1],
					remote_info.DenIP[2], remote_info.DenIP[3]);
			RemotePort = RemoteVideoPort;
			UdpSendBuff(m_VideoSocket, RemoteHost, RemotePort, send_b, sendlength);
		}
	}
    Local.Timer1Num++;
}

void TalkCtrlFunc(void)
{
	int CallTimeOut;
    int Status;

	//1S
	if ((Local.TimeOut % TIMERPERSEC)==0) {
        Status = get_device_status(CALL_MIXER);
        
		switch(Status)
		{
			case 1:  //主叫对讲
			case 2:  //被叫对讲
				CallTimeOut = CALLTIMEOUT;
                //呼叫超时
				if (Local.TimeOut > CallTimeOut) {
                    printf("呼叫超时\n");
                    stop_talk();
                    cb_opt_function.cb_curr_opt(CB_CALL_TIMEOUT);
                    Local.OnlineFlag = 0;
				}
				break;
			case 5:  //主叫通话
			case 6:  //被叫通话
				if (Local.TimeOut > Local.TalkTimeOut) {
					printf("通话超时\n");
                    stop_talk();
                    cb_opt_function.cb_curr_opt(CB_TALK_TIMEOUT);
					Local.OnlineFlag = 0;
				}
				break;
			default:
				break;
		}
	}

	Local.TimeOut ++;       //监视超时,  通话超时,  呼叫超时，无人接听
}
