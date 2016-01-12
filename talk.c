#include <stdio.h>
#include <stdlib.h>
//#include <semaphore.h>       //sem_t
//#include <arpa/inet.h>

#define _LIB_QSA_DEF_H
#include "libqsa_common.h"

void start_call(const char * ip, const char * addr, int uFlag);
void Talk_Call_End_Task(void);
void Talk_Call_TimeOut_Task(void);
//---------------------------------------------------------------------------
void start_call(const char * ip, const char * addr, int uFlag)
{
	int i;
	int j;
	uint32_t Ip_Int;

	j = 0;

	if ( 2 == uFlag ) {
		/* get remote device information */
		Ip_Int = inet_addr(ip);
		memcpy(&Remote.IP[j], &Ip_Int, 4);
		printf("%d.%d.%d.%d\n", Remote.IP[j][0], Remote.IP[j][1], Remote.IP[j][2], Remote.IP[j][3]);

		memcpy(&Remote.Addr[j], device_config.address, 20);
		Remote.Addr[j][0] = 'S';
		Remote.Addr[j][7] = addr[0];
		Remote.Addr[j][8] = addr[1];
		Remote.Addr[j][9] = addr[2];
		Remote.Addr[j][10] = addr[3];

		pthread_mutex_lock(&Local.udp_lock);

		for (i = 0; i < UDPSENDMAX; i++) {
			if (Multi_Udp_Buff[i].isValid == 0) {
				Multi_Udp_Buff[i].SendNum = 0;
				Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
				Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
				sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",
						Remote.IP[j][0], Remote.IP[j][1], Remote.IP[j][2],
						Remote.IP[j][3]);
				memcpy(&Multi_Udp_Buff[i].RemoteIP,
						&Multi_Udp_Buff[i].RemoteHost, 20);
				memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
				Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
				Multi_Udp_Buff[i].buf[7] = ASK;
				Multi_Udp_Buff[i].buf[8] = CALL;
				memcpy(Multi_Udp_Buff[i].buf + 9, device_config.address, 20);
				memcpy(Multi_Udp_Buff[i].buf + 29, device_config.ip, 4);
				memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[j], 20);
				memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j], 4);
				Multi_Udp_Buff[i].buf[57] = 0;
				memcpy(Multi_Udp_Buff[i].buf + 58, Remote.IP, 4);

				Multi_Udp_Buff[i].nlength = 62;
				Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
				Multi_Udp_Buff[i].SendDelayTime = 0;
				Multi_Udp_Buff[i].isValid = 1;
				sem_post(&multi_send_sem);
				break;
			}
		}
		pthread_mutex_unlock(&Local.udp_lock);
	}
}

void stop_talk(void) {

	int i, j;
    j = 0;
	pthread_mutex_lock(&Local.udp_lock);
	for (i = 0; i < UDPSENDMAX; i++) {
		if (Multi_Udp_Buff[i].isValid == 0) {
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
			Multi_Udp_Buff[i].buf[7] = ASK;
			Multi_Udp_Buff[i].buf[8] = CALLEND;
			Multi_Udp_Buff[i].SendNum = 0;
			Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
			Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
			Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;

			memcpy(Multi_Udp_Buff[i].buf + 9, device_config.address, 20);
			memcpy(Multi_Udp_Buff[i].buf + 29, device_config.ip, 4);
			memcpy(Multi_Udp_Buff[i].buf + 33, Remote.Addr[j], 20);
			memcpy(Multi_Udp_Buff[i].buf + 53, Remote.IP[j], 4);

			Multi_Udp_Buff[i].nlength = 57;
			Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
			Multi_Udp_Buff[i].SendDelayTime = 0;
			Multi_Udp_Buff[i].isValid = 1;
			sem_post(&multi_send_sem);
			Local.Status = 0;

			break;
		}
	}
	pthread_mutex_unlock(&Local.udp_lock);
}
//---------------------------------------------------------------------------
void Talk_Call_TimeOut_Task(void) {

    stop_talk();
    Local.OnlineFlag = 0;
    printf("call timeout\n");
}

