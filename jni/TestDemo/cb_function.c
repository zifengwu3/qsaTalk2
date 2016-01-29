#include <stdio.h>
#include <stdlib.h>

#include "../include/libqsa_callback.h"
extern int g_Status;
extern unsigned char g_addr[30];
extern unsigned char g_ip[20];

/* default defined */
void default_cb_audio_data1(void * data, int length, int type) {
    printf("default_cb_audio_data1\n");
    return;
}

void default_cb_info1(const void * data, const void * ip, int length, int port) {
    printf("default_cb_info1\n");
    return;
}

int default_cb_status1(int type) {
    printf("%s : type = %d, g_Status = %d\n", __FUNCTION__, type, g_Status);
    return g_Status;
}

void default_cb_devip1(const void * address, const void * ip, int uflag) {
    int rtnval = 0;
    printf("default_cb_devip1\n");
    
    rtnval = memcmp(g_addr, address, 4);
    if (rtnval == 0) {
        memcpy(g_ip, ip, 4);
    }
    printf("ip:%d.%d.%d.%d\n", g_ip[0], g_ip[1], g_ip[2], g_ip[3]);
    return;
}

void default_cb_opt1(int value, int status) {

    g_Status = status;

    printf(" %s:value = %d, g_Status = %d\n", __FUNCTION__, value, g_Status);

    return;
}
