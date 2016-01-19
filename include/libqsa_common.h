#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>       //sem_t
#include <pthread.h>       //sem_t
#include <string.h>
#include <arpa/inet.h>

#include "libqsa_def.h"

#ifndef _LIB_QSA_DEF_H
#define _LIB_QSA_DEF_H
struct dev_config local_config;                      //本地通用配置
int configlen = sizeof(struct dev_config);            //结构长度

struct Local1 Local;
//远端地址
struct Remote1 remote_info;
char null_addr[21];   //空字符串
char null_ip[4];   //空字符串

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
extern struct dev_config local_config;               
extern int configlen;

extern struct Local1 Local;
extern struct Remote1 remote_info;
extern char null_addr[21];   //空字符串
extern char null_ip[4];   //空字符串

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

#endif

