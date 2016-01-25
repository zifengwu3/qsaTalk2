#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <semaphore.h>       //sem_t
#include <pthread.h>       //sem_t

#define _LIB_QSA_DEF_H
#include "libqsa_common.h"

extern int UdpSendBuff(int m_Socket, char *RemoteHost, int RemotePort,
		unsigned char *buf, int nlength);

//---------------------------------------------------------------------------
void start_call(const char * ip, const char * addr, int uFlag) {
	int i;
	int j;
	uint32_t Ip_Int;
    int Status = 0;

    j = remote_info.DenNum;

    if ( 2 == uFlag ) {
        Status = get_device_status(CALL_MIXER);
        if (Status == CB_ST_NULL) {
            /* get remote device information */
            Ip_Int = inet_addr(ip);
            memcpy(&remote_info.IP[j], &Ip_Int, 4);
            printf("<%s>   %d.%d.%d.%d\n", __FUNCTION__,
                    remote_info.IP[j][0], remote_info.IP[j][1], 
                    remote_info.IP[j][2], remote_info.IP[j][3]);

            memcpy(&remote_info.Addr[j], local_config.address, 20);
            remote_info.Addr[j][0] = 'S';
            remote_info.Addr[j][7] = addr[0];
            remote_info.Addr[j][8] = addr[1];
            remote_info.Addr[j][9] = addr[2];
            remote_info.Addr[j][10] = addr[3];

            for (i = 0; i < UDPSENDMAX; i++) {
                if (Multi_Udp_Buff[i].isValid == 0) {
                    Multi_Udp_Buff[i].SendNum = 0;
                    Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
                    Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;

                    sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",
                            remote_info.IP[j][0], remote_info.IP[j][1], 
                            remote_info.IP[j][2], remote_info.IP[j][3]);
                    memcpy(&Multi_Udp_Buff[i].RemoteIP,
                            &Multi_Udp_Buff[i].RemoteHost, 20);

                    memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
                    Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
                    Multi_Udp_Buff[i].buf[7] = ASK;
                    Multi_Udp_Buff[i].buf[8] = CALL;

                    memcpy(Multi_Udp_Buff[i].buf + 9, local_config.address, 20);
                    memcpy(Multi_Udp_Buff[i].buf + 29, local_config.ip, 4);
                    memcpy(Multi_Udp_Buff[i].buf + 33, remote_info.Addr[j], 20);
                    memcpy(Multi_Udp_Buff[i].buf + 53, remote_info.IP[j], 4);
                    Multi_Udp_Buff[i].buf[57] = 0;
                    memcpy(Multi_Udp_Buff[i].buf + 58, remote_info.IP, 4);

                    Multi_Udp_Buff[i].nlength = 62;
                    Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
                    Multi_Udp_Buff[i].SendDelayTime = 0;
                    Multi_Udp_Buff[i].isValid = 1;
                    printf("<%s>   开始呼叫命令\n", __FUNCTION__);
                    sem_post(&multi_send_sem);
                    break;
                }
            }
        } else {
            printf("I'm is Busy!\n");
        }
    }
}

void stop_talk(void) {

	int i;

	for (i = 0; i < UDPSENDMAX; i++) {
		if (Multi_Udp_Buff[i].isValid == 0) {
			memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
			Multi_Udp_Buff[i].buf[6] = VIDEOTALK;
			Multi_Udp_Buff[i].buf[7] = ASK;
			Multi_Udp_Buff[i].buf[8] = CALLEND;
			Multi_Udp_Buff[i].SendNum = 0;
			Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
			Multi_Udp_Buff[i].RemotePort = RemoteVideoPort;
            sprintf(Multi_Udp_Buff[i].RemoteHost, "%d.%d.%d.%d",
                    remote_info.IP[0][0], remote_info.IP[0][1], 
                    remote_info.IP[0][2], remote_info.IP[0][3]);
            memcpy(&Multi_Udp_Buff[i].RemoteIP,
                    &Multi_Udp_Buff[i].RemoteHost, 20);
            Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;

			memcpy(Multi_Udp_Buff[i].buf + 9, local_config.address, 20);
			memcpy(Multi_Udp_Buff[i].buf + 29, local_config.ip, 4);
			memcpy(Multi_Udp_Buff[i].buf + 33, remote_info.Addr[0], 20);
			memcpy(Multi_Udp_Buff[i].buf + 53, remote_info.IP[0], 4);

			Multi_Udp_Buff[i].nlength = 57;
			Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
			Multi_Udp_Buff[i].SendDelayTime = 0;
			Multi_Udp_Buff[i].isValid = 1;

            printf("<%s>   通知对方关闭对讲呼叫\n", __FUNCTION__);
			sem_post(&multi_send_sem);
			break;
		}
	}
}
//-----------------------------------------------------------------------
//呼叫   0 住户 1  中心  
void find_ip(const char * addr, int uFlag) {
    int i;
    int Status;
    char remoteAddr[20];

    if (addr != NULL) {
        Status = get_device_status(CALL_MIXER);
        if ((Status == CB_ST_NULL) && (uFlag == 0)) {
            memcpy(remoteAddr, local_config.address, 20);
            remoteAddr[0] = 'S';
            memcpy(remoteAddr + 7, addr, 4);
            //查找可用发送缓冲并填空
            for (i=0; i<UDPSENDMAX; i++) {
                if (Multi_Udp_Buff[i].isValid == 0) {
                    Multi_Udp_Buff[i].SendNum = 3;
                    Multi_Udp_Buff[i].m_Socket = m_VideoSocket;
                    Multi_Udp_Buff[i].CurrOrder = VIDEOTALK;
                    strcpy(Multi_Udp_Buff[i].RemoteHost, NSMULTIADDR);
                    Multi_Udp_Buff[i].RemotePort = LocalVideoPort;
                    //头部
                    memcpy(Multi_Udp_Buff[i].buf, UdpPackageHead, 6);
                    //命令  ,子网多播查找
                    Multi_Udp_Buff[i].buf[6] = NSORDER;
                    Multi_Udp_Buff[i].buf[7] = ASK;    //主叫
                    memcpy(Multi_Udp_Buff[i].buf + 8, local_config.address, 20);
                    memcpy(Multi_Udp_Buff[i].buf + 28, local_config.ip, 4);

                    memcpy(Multi_Udp_Buff[i].buf+32, remoteAddr, 20);

                    memcpy(Multi_Udp_Buff[i].buf+52, null_ip, 4);

                    Multi_Udp_Buff[i].nlength = 56;
                    Multi_Udp_Buff[i].DelayTime = DIRECTCALLTIME;
                    Multi_Udp_Buff[i].SendDelayTime = 0;
                    Multi_Udp_Buff[i].isValid = 1;

                    memcpy(remote_info.Addr[0], null_addr, 20);
                    memcpy(remote_info.Addr[0], addr, 20);
                    memcpy(remote_info.IP[0], null_ip, 4);
                    printf("<%s>   正在查找地址 : %s\n", __FUNCTION__, addr);
                    sem_post(&multi_send_sem);
                    break;
                }
            }
        } else {
            printf("I'm BUSY\n");
        }
    } else {
        printf("Addr is NULL\n");
    }
}
//---------------------------------------------------------------------------
void send_video(const char * data, int length, int frame_num, int frame_type, const char * ip) {
    int j;
    int TotalPackage; //总包数
    unsigned char mpeg4_out[1600];
    char RemoteHost[20];
    //通话数据结构
    struct talkdata1 talkdata;

    struct timeval tv;
    uint32_t nowtime;
    int Status;

#ifdef _DEBUG
    printf("发送VIDEO数据：\n" );
#endif

    gettimeofday(&tv, NULL);
    nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;

    Status = get_device_status(CALL_MIXER);
    if (Status > 0) {
        //头部
        memcpy(mpeg4_out, UdpPackageHead, 6);
        //命令
        mpeg4_out[6] = VIDEOTALK;
        mpeg4_out[7] = 1;
        //子命令
        mpeg4_out[8] = CALLUP;
        //IP
        memcpy(talkdata.HostAddr, local_config.address, 20);
        memcpy(talkdata.HostIP, local_config.ip, 4);
        memcpy(talkdata.AssiAddr, remote_info.Addr[0], 20);
        memcpy(talkdata.AssiIP, remote_info.IP[0], 4);
        //时间戳
        talkdata.timestamp = nowtime;
        //帧序号
        talkdata.Frameno = frame_num;
        //帧数据长度
        talkdata.Framelen = length;
        //帧类型
        talkdata.DataType = frame_type;
        //单包长度
        talkdata.PackLen = PACKDATALEN;
        //总包数
        if ((length%talkdata.PackLen) == 0) {
            TotalPackage = length/talkdata.PackLen;
        } else {
            TotalPackage = (length/talkdata.PackLen) + 1;
        }
        talkdata.TotalPackage = TotalPackage;

        //对方IP
        sprintf(RemoteHost, "%d.%d.%d.%d", 
                remote_info.DenIP[0], remote_info.DenIP[1],
                remote_info.DenIP[2], remote_info.DenIP[3]);

        for (j=1; j<=TotalPackage; j++) {        
            //包的顺序从大到小
            if (j == TotalPackage) {
                talkdata.CurrPackage = j;      //当前包
                talkdata.Datalen = length - (j - 1)*talkdata.PackLen;     //数据长度
                memcpy(mpeg4_out + 9, &talkdata, sizeof(talkdata));
                memcpy(mpeg4_out + 9 + sizeof(struct talkdata1), 
                        data + (j - 1)*talkdata.PackLen, (length - (j - 1)*talkdata.PackLen));
                //UDP发送
                UdpSendBuff(m_VideoSocket, RemoteHost, RemoteVideoPort, 
                        mpeg4_out, (9 + sizeof(struct talkdata1) + (length - (j - 1)*talkdata.PackLen)));
            } else {
                talkdata.CurrPackage = j;           //当前包
                talkdata.Datalen = talkdata.PackLen;       //数据长度
                memcpy(mpeg4_out + 9, &talkdata, sizeof(talkdata));
                memcpy(mpeg4_out + 9 + sizeof(struct talkdata1), 
                        data + (j - 1)*talkdata.PackLen, talkdata.PackLen);
                //UDP发送
                UdpSendBuff(m_VideoSocket, RemoteHost, RemoteVideoPort, 
                        mpeg4_out, (9 + sizeof(struct talkdata1) + talkdata.PackLen));
            }
        }
    }
}

void send_audio(const char * data, int length, int frame_num, const char * ip) {

    unsigned char adpcm_out[1600];
    char RemoteHost[20];
    //通话数据结构
    struct talkdata1 talkdata;

    struct timeval tv;
    uint32_t nowtime;
    int Status;

#ifdef _DEBUG
    printf("创建采集数据处理线程：\n" );
#endif

    Status = get_device_status(CALL_MIXER);
    if (Status > 0) {
        gettimeofday(&tv, NULL);
        nowtime = tv.tv_sec *1000 + tv.tv_usec/1000;

        //头部
        memcpy(adpcm_out, UdpPackageHead, 6);
        //命令
        adpcm_out[6] = VIDEOTALK;
        adpcm_out[7] = 1;
        //子命令
        if (Status == 5) { //本机为主叫方
            adpcm_out[8] = CALLUP;
            memcpy(talkdata.HostAddr, local_config.address, 20);
            memcpy(talkdata.HostIP, local_config.ip, 4);
            memcpy(talkdata.AssiAddr, remote_info.Addr[0], 20);
            memcpy(talkdata.AssiIP, remote_info.IP[0], 4);
        }

        //时间戳
        talkdata.timestamp = nowtime;
        //数据类型
        talkdata.DataType = 1;
        //帧序号
        talkdata.Frameno = frame_num;
        //帧数据长度
        talkdata.Framelen = AUDIOBLK/2;
        //总包数
        talkdata.TotalPackage = 1;
        //当前包
        talkdata.CurrPackage = 1;
        //数据长度
        talkdata.Datalen = AUDIOBLK/2;
        talkdata.PackLen = PACKDATALEN;
        memcpy(adpcm_out + 9, &talkdata, sizeof(talkdata));
        memcpy((adpcm_out + 9 + sizeof(struct talkdata1)), data,  AUDIOBLK/2);

        //UDP发送
        sprintf(RemoteHost, "%d.%d.%d.%d", 
                remote_info.DenIP[0], remote_info.DenIP[1],
                remote_info.DenIP[2], remote_info.DenIP[3]);

        UdpSendBuff(m_VideoSocket, RemoteHost, RemoteAudioPort, 
                adpcm_out, 9 + sizeof(struct talkdata1) + AUDIOBLK/2);
    }
}
