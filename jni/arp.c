#include <unistd.h>
#include <arpa/inet.h> 
#include <linux/if_ether.h>

#define CommonH
#include "libqsa_common.h"

int InitArpSocket(void);
void SendFreeArp(void);
void CloseArpSocket(void);

int InitArpSocket(void)
{
	ARP_Socket = socket(AF_INET, SOCK_PACKET, htons(ETH_P_RARP));
	if (ARP_Socket < 0)
	{
		perror("Create arpsocket error\r\n");
		return 0;
	}
	return 1;   
}

void CloseArpSocket(void)
{
	if (ARP_Socket >= 0) 
    {
		close(ARP_Socket);
		ARP_Socket = (-1);
	}
}

void SendFreeArp(void)
{
    int i;
    for (i=0; i<UDPSENDMAX; i++)
    {
        if (Multi_Udp_Buff[i].isValid == 0)
        {
            //锁定互斥锁
            pthread_mutex_lock (&Local.udp_lock);
            Multi_Udp_Buff[i].SendNum = 3;
            Multi_Udp_Buff[i].m_Socket = ARP_Socket;
            Multi_Udp_Buff[i].DelayTime = 100;
            Multi_Udp_Buff[i].SendDelayTime = 0; //发送等待时间计数  20101112
            Multi_Udp_Buff[i].isValid = 1;

            pthread_mutex_unlock (&Local.udp_lock);
            sem_post(&multi_send_sem);
            break;
        }
    }
}

