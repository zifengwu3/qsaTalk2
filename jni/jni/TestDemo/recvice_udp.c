//UDP
#include <stdio.h>
#include <time.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <semaphore.h>       //sem_t
#include <dirent.h>
#include <pthread.h>       //sem_t
#include <string.h>

extern int g_Value;
extern int g_Status;

int InitUdpSocketDemo(short lPort);
void CloseUdpSocketDemo(void);
int UdpSendBuffDemo(int m_Socket, char *RemoteHost, int RemotePort,
		unsigned char *buf, int nlength);

//UDP音视频接收线程函数
void CreateUdpVideoRcvThreadDemo(void);
void UdpVideoRcvThreadDemo(void);

//UDP
short UdpRecvFlag;
pthread_t udpvideorcvid;
int m_VideoSocketDemo;

void CreateUdpVideoRcvThreadDemo(void) {
	int ret;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	ret = pthread_create(&udpvideorcvid, &attr, (void *) UdpVideoRcvThreadDemo,
			NULL);
	pthread_attr_destroy(&attr);
	printf("Create UDP video demo pthread!\n");
	if (ret != 0) {
		printf("Create video pthread error!\n");
        return;
	}
    return;
}

void UdpVideoRcvThreadDemo(void)
{
	char FromIP[20];
	struct sockaddr_in c_addr;
	socklen_t addr_len;
	int len;
	unsigned char buff[8096];
    char UdpPackageHead[15];

    printf("This is udp value demo pthread.\n");
	UdpRecvFlag = 1;

	while (UdpRecvFlag == 1) {
        switch (g_Value) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
                {
                    printf("[%s]:value = %d, g_Status = %d\n", __FUNCTION__, g_Value, g_Status);
                    g_Value = 99;
                }
                break;
            default:
                break;
        }

	}

    return;
}


int InitUdpSocketDemo(short lPort) {
	struct sockaddr_in s_addr;
	int nZero = 0;
	int m_Socket;
	int nYes;
	int ttl;
    int iLen;

	if ((m_Socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf("Create socket error\r\n");
		return (-1);
	} else {
		printf("create socket.\n\r");
	}

	memset(&s_addr, 0, sizeof(struct sockaddr_in));
	s_addr.sin_family = AF_INET;
	s_addr.sin_port = htons(lPort);
	s_addr.sin_addr.s_addr = INADDR_ANY; //inet_addr(LocalIP);//INADDR_ANY;

	iLen = sizeof((char *) &nZero); //  SO_SNDBUF
	nZero = 128*1024;
	setsockopt(m_Socket, SOL_SOCKET, SO_SNDBUF, (char*) &nZero, iLen);
	nZero = 128*1024;
	setsockopt(m_Socket, SOL_SOCKET, SO_RCVBUF, (char*) &nZero, iLen);

	ttl = 5;
	if (setsockopt(m_Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &ttl,
			sizeof(ttl)) == (-1)) {
		printf("set ip_broadcast_ttl error.\n\r");
		return (-1);
    }

	nYes = 1;
	if (setsockopt(m_Socket, SOL_SOCKET, SO_BROADCAST, (char *) &nYes,
			sizeof((char *) &nYes)) == (-1)) {
		printf("set broadcast error.\n\r");
		return (-1);
	}

	if ((bind(m_Socket, (struct sockaddr *) &s_addr, sizeof(s_addr))) == (-1)) {
		printf("bind error\n\r");
		return (-1);
	} else {
        printf("bind address to socket.\n\r");
    }

	if (lPort == 8302) {
		m_VideoSocketDemo = m_Socket;
		CreateUdpVideoRcvThreadDemo();
	}
	return 0;
}

void CloseUdpSocketDemo(void) {
	UdpRecvFlag = 0;
	close(m_VideoSocketDemo);
}
