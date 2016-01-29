#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>

#include "libqsa_common.h"

extern void Init_Timer();
extern void Uninit_Timer();
extern int init_udp_task(void);
extern int uninit_udp_task(void);
extern void set_cb_function_default(struct _cb_function * p);

int init_param_task(struct dev_config * _config);

void init_config_null_task(void);
void init_local_param_task(void);
void init_default_remote_task(void);
void get_device_config_task(struct dev_config * _config);
void send_info(const char * data, struct _send_info * info);

int uninit_task(void);

int init_param_task(struct dev_config * _config) {
    
    //初始化设备参数数据
    init_config_null_task();
    get_device_config_task(_config);

    //初始化本地数据
    init_local_param_task();
    init_default_remote_task();

    //初始化回调函数
    set_cb_function_default(&cb_opt_function);

    //初始化线程
	Init_Timer();
	init_udp_task();

	return (1);
}

int uninit_task(void) {

    Uninit_Timer();
    uninit_udp_task();

	return (1);
}

void init_config_null_task(void) {
    memset(&local_config, 0x00, configlen);
    return;
}

void get_device_config_task(struct dev_config * _config) {
    if (_config) {
        memcpy(&local_config, _config, configlen);
    }
    return;
}

void init_local_param_task(void) {

    uint32_t Ip_Int;
    
    RemoteDataPort = 8300;
	LocalDataPort = 8300;

	RemoteVideoPort = 8302;
	LocalVideoPort = 8302;

    strcpy(UdpPackageHead, "QIUSHI");
    memset(null_addr, 0x30, 20);
    null_addr[20] = '\0';

    memset(null_ip, 0, 4);

    Ip_Int = inet_addr((char *)local_config.ip);
    memcpy(locate_ip, &Ip_Int, 4);

    return;
}

void init_default_remote_task(void) {

    memset(&remote_info, 0x00, sizeof(struct Remote1));
    remote_info.DenNum = 0;             //表示使用默认参数
    remote_info.isDirect = 1;
}

void send_info(const char * data, struct _send_info * info) {
    int i;
    int j;
    uint32_t Ip_Int;

    if (info != NULL) {
        j = 0;
        /* get remote device information */
        Ip_Int = inet_addr((char *)info->ip);
        memcpy(&remote_info.IP[j], &Ip_Int, 4);
        printf("%d.%d.%d.%d\n", remote_info.IP[j][0], remote_info.IP[j][1], 
                remote_info.IP[j][2], remote_info.IP[j][3]);

        memcpy(&remote_info.Addr[j], local_config.address, 20);
        remote_info.Addr[j][0] = 'S';
        memcpy(&remote_info.Addr[j]+7, info->addr, 4);

        for (i = 0; i < UDPSENDMAX; i++) {
            if (Multi_Udp_Buff[i].isValid == 0) {
                Multi_Udp_Buff[i].SendNum = 0;
                Multi_Udp_Buff[i].m_Socket = m_DataSocket;
                Multi_Udp_Buff[i].RemotePort = info->port;

                sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",
                        remote_info.IP[j][0], remote_info.IP[j][1], 
                        remote_info.IP[j][2], remote_info.IP[j][3]);
                memcpy(&Multi_Udp_Buff[i].RemoteIP,
                        &Multi_Udp_Buff[i].RemoteHost, 20);

                memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
                Multi_Udp_Buff[i].buf[6] = SENDMESSAGE;
                Multi_Udp_Buff[i].buf[7] = ASK;

                memcpy(Multi_Udp_Buff[i].buf + 8, local_config.address, 20);
                memcpy(Multi_Udp_Buff[i].buf + 28, remote_info.Addr[j], 20);
                memcpy(Multi_Udp_Buff[i].buf + 48, local_config.mac, 6);
                Multi_Udp_Buff[i].buf[54] = info->uFlag;
                memcpy(Multi_Udp_Buff[i].buf + 55, info->title, 16);
                memset(Multi_Udp_Buff[i].buf + 71, 0x00, 14);
                Multi_Udp_Buff[i].buf[72] = 0x01;
                Multi_Udp_Buff[i].buf[73] = ((info->length) >> 8);
                Multi_Udp_Buff[i].buf[74] = ((info->length) & 0xFF);
                Multi_Udp_Buff[i].buf[78] = 0x01;
                Multi_Udp_Buff[i].buf[80] = 0x01;
                Multi_Udp_Buff[i].buf[81] = ((info->length) >> 8);
                Multi_Udp_Buff[i].buf[82] = ((info->length) & 0xFF);
                Multi_Udp_Buff[i].buf[83] = (1200 >> 8);
                Multi_Udp_Buff[i].buf[84] = (1200 & 0xFF);
                memcpy(Multi_Udp_Buff[i].buf + 85, data, info->length);

                Multi_Udp_Buff[i].nlength = 84 + (info->length);
                Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
                Multi_Udp_Buff[i].SendDelayTime = 0;
                Multi_Udp_Buff[i].isValid = 1;
                printf("<%s>   准备向<%d.%d.%d.%d>送信息\n", __FUNCTION__,
                        remote_info.IP[j][0], remote_info.IP[j][1], 
                        remote_info.IP[j][2], remote_info.IP[j][3]);
                sem_post(&multi_send_sem);
                break;
            }
        }
    }
}



